#include    "buildstat.h"
#include    "../util/errhandler.h"
#include    "funcstat.h"
#include    "processPrinter.h"
#include    "../perffile/overviewPrinter.h"
#include    <stdlib.h>
#include    <string.h>

static process_tree_t recTree;

/**
 * A new sample has been produced. The corresponding process is searched for, if
 * not found, we assume it belongs to a common process with the pid ffffffff. 
 * The number of samples of this process is increased by one and the period of 
 * the record is added to the period of the process.
 *
 * In addition, the application or library where the .ip of the sample points to
 * is searched within the mmap entries of the process. If it is a library we 
 * subtract the start address of the library from the instruction pointer to get
 * the address. For an application, we just use the instruction pointer. This 
 * address together with the binary name is used to search for or create the 
 * source function name where this event occurred.
 * As for the process, the sample count and period of the function is updated.
 */
static bool decodeSample( struct record_sample *evt ) {
    log_overview_ii( evt->header.nr, PERF_RECORD_SAMPLE, evt->header.pid, evt->header.tid, evt->header.time, evt->ip, evt->period );
    
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        proc = find_process( &recTree, COMMON_PROC );
    }
    if( proc == NULL ){
        char txt[30];
        snprintf( txt, sizeof(txt), "%i", evt->header.pid );
        set_last_error(ERR_PROC_NOT_FOUND, strdup(txt));
        return false;
    }
    
    proc->samples++;
    proc->period += evt->period;
    
    struct rmmap* mmap = find_mmap( proc, evt->ip );
    if( mmap == NULL ){
        proc = find_process( &recTree, COMMON_PROC );
        try( proc != NULL );
        mmap = find_mmap( proc, evt->ip );
        if( mmap == NULL ){
            char txt[60];
            snprintf( txt, sizeof(txt), "%llu in %s", evt->ip, proc->comm );
            set_last_error(ERR_MEMORY_RANGE_NOT_FOUND, strdup(txt));
            return false;
        }
    }
    
    u64 addr;
    
    // this is a bit hacky; I'm not sure if it is correct but it seems to work
    if( evt->ip >= proc->vdso ){
        addr = evt->ip - mmap->start;
    } else {
        addr = evt->ip;
    }
    
    struct func_dir *entry = force_entry( addr, mmap->filename );
    if( entry == NULL ){
        entry = func_error();
    }
    try( entry != NULL );
    
    entry->samples++;
    entry->period += evt->period;
    
    return true;
}

/**
 * A library module was loaded. As for "COMM" records, it is possible that a 
 * process does not yet exist. For that case we create one as in the function 
 * @link decodeComm @endlink.
 * The information of the record is added to the process. If the .filename is
 * "[vdso]" we assume that this record contains the begin of the address space 
 * of the libraries. In this case, the .pgoff information is stored as .vdso 
 * for the process.
 */
static bool decodeMmap( struct record_mmap *evt ) {
    log_overview_siii( evt->header.nr, PERF_RECORD_MMAP, evt->header.pid, evt->header.tid, evt->header.time, evt->filename, evt->start, evt->len, evt->pgoff );
    
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_process( &recTree, evt->header.pid );
        try( proc != NULL );
    }
    
    if( memcmp( evt->filename, "[vdso]", 6 ) == 0 ){
        trymsg( proc->vdso == 0, ERR_NOT_YET_DEFINED, "2 vdso mmap records found" );
        proc->vdso = evt->pgoff;
    } else {
        struct rmmap *old = proc->mmaps;
        proc->mmaps = (struct rmmap *)malloc( sizeof(*proc->mmaps) );
        trysys( proc->mmaps != NULL );
        proc->mmaps->next = old;
        proc->mmaps->start = evt->start;
        proc->mmaps->end = evt->start + evt->len - 1;
        proc->mmaps->pgoff = evt->pgoff;
        memcpy( proc->mmaps->filename, evt->filename, sizeof(proc->mmaps->filename) );
    }
    
    return true;
}

/**
 * Provides the application name for an process. If the corresponding process is
 * not found we assume that it was not yet created. This is the case for 
 * processes running at the time perf record was started. If so, we expect the 
 * timestamp to be zero and create the process. The name, provided by the 
 * record, is assigned to the process.
 */
static bool decodeComm( struct record_comm* evt ) {
    log_overview_s( evt->header.nr, PERF_RECORD_COMM, evt->header.pid, evt->header.tid, evt->header.time, evt->comm );
    
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_process( &recTree, evt->header.pid );
        try( proc != NULL );
    }
    
    memcpy( proc->comm, evt->comm, sizeof(proc->comm) );
    
    return true;
}

/**
 * A new process or thread is created. We check if we already have a process 
 * with this pid stored. If yes and the fork created a new process we throw an 
 * error because we cannot have two running processes with the same pid. If no 
 * process is found and the fork created a thread we also throw an error, since 
 * a thread cannot be created without a corresponding process. 
 * If a new process is created by the fork, we also create a new process in 
 * memory and assign the corresponding pid and timestamp.
 */
static bool decodeFork( struct record_fork *evt ) {
    log_overview_i( evt->header.nr, PERF_RECORD_FORK, evt->header.pid, evt->header.tid, evt->header.time, evt->ppid );
    
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        // if it is a thread, throw an error
        trymsg( evt->header.pid == evt->header.tid, ERR_PROC_NOT_FOUND, __func__ );
        proc = create_process( &recTree, evt->header.pid );
        try( proc != NULL );
    } else {
        // if it is not a thread, throw an error
        trymsg( evt->header.pid != evt->header.tid, ERR_PROC_ALREADY_EXISTS, __func__ );
    }
    
    proc->fork_time = evt->header.time;
    
    return true;
}

/**
 * A process or thread is terminated. If it was a process, it is removed from 
 * the internal list of processed and the information is written to a file.
 */
static bool decodeExit( struct record_fork *evt ) {
    log_overview( evt->header.nr, PERF_RECORD_EXIT, evt->header.pid, evt->header.tid, evt->header.time );
    
    if( evt->header.pid == evt->header.tid ){       // only remove if application is terminated
        struct process* proc = find_process( &recTree, evt->header.pid );
        if( proc != NULL ){     // exit occurs 2 times, don't throw an error
            proc->exit_time = evt->header.time;
            
            try( remove_process( &recTree, proc ) );
            
            print_process( proc );
            free( proc );
        }
    }
    
    return true;
}

static void handleRecord(struct record_t *proc, void *data){
    (void)data;
    bool success = false;
    switch( proc->type ){
        case PERF_RECORD_COMM:{
            success = decodeComm( (struct record_comm*)proc );
            break;
        }
        case PERF_RECORD_MMAP:{
            success = decodeMmap( (struct record_mmap*)proc );
            break;
        }
        case PERF_RECORD_FORK:{
            success = decodeFork( (struct record_fork*)proc );
            break;
        }
        case PERF_RECORD_EXIT:{
            success = decodeExit( (struct record_fork*)proc );
            break;
        }
        case PERF_RECORD_SAMPLE:{
            success = decodeSample( (struct record_sample*)proc );
            break;
        }
        default:{
            success = true;
            log_overview( proc->nr, proc->type, 0, 0, proc->time );
            break;
        }
    }
    if( !success ){
        print_last_error();
    };
};

/**
 * Since all records are now sorted in the memory, we can process them. For 
 * every record, the corresponding callback function is called. Two new data 
 * structures are kept in memory: one to keep track of the actual processes 
 * together with memory maps of it and used libraries, and the other to gather 
 * the period and sample number for each source function.
 */
bool buildstat( record_order_tree_t *tree ){
    recTree = init_processes();
    iterate_order( tree, handleRecord, NULL );
    print_processes( &recTree );
    return true;
}
