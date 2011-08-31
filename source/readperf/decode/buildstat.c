#include    "buildstat.h"
#include    "../util/errhandler.h"
#include    "funcstat.h"
#include    "processPrinter.h"
#include    "../perffile/overviewPrinter.h"
#include    <stdlib.h>
#include    <string.h>

static process_tree_t recTree;


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

bool buildstat( record_order_tree_t *tree ){
    recTree = init_processes();
    iterate_order( tree, handleRecord, NULL );
    print_processes( &recTree );
    return true;
}
