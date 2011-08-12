#include    <stdio.h>
#include    <linux/types.h>
#include    <linux/perf_event.h>
#include    <unistd.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#include    <string.h>
#include    <stdlib.h>
#include    "util/types.h"
#include    "util/origperf.h"
#include    "util/errhandler.h"
#include    "decode/funcstat.h"
#include    "perffile/processPrinter.h"
#include    "perffile/perffile.h"
#include    "perffile/records.h"

#define SEP ","

#define OVERVIEW_LOG_NAME   "overview.csv"
static FILE *overview_file = NULL;

#define SUMMARY_LOG_NAME   "summary.csv"
static FILE *summary_file = NULL;


#define STAT_LOG_NAME   "stat.csv"
#define FUNC_LOG_NAME   "func.csv"

process_tree_t recTree;

static const char *NAMES[] = { NULL, "MMAP", "LOST", "COMM", "EXIT", "THROTTLE", "UNTHROTTLE", "FORK", "READ", "SAMPLE" };

static void log_overview_head( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    fprintf( overview_file, "%llu%s%s%s%u%s%u%s%llu%s", ev_nr, SEP, NAMES[type], SEP, pid, SEP, tid, SEP, time, SEP );
}

static void log_overview_s( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "%s\n", info );
}

static void log_overview_i( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "%llu\n", info );
}

static void log_overview( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "\n" );
}

static bool decodeSample( struct record_sample *evt ) {
    struct process* proc = find_process( &recTree, evt->header.pid );
    trymsg( proc != NULL, ERR_ENTRY_NOT_FOUND, __func__ );
    
    proc->samples++;
    proc->period += evt->period;
    
    struct rmmap* mmap = find_mmap( proc, evt->ip );
    trymsg( mmap != NULL, ERR_ENTRY_NOT_FOUND, __func__ );
    
    struct func_dir *entry = force_entry( evt->ip, mmap->filename );
    trymsg( entry != NULL, ERR_NOT_YET_DEFINED, __func__ );
    
    entry->samples++;
    entry->period += evt->period;
    
    log_overview_i( evt->header.nr, PERF_RECORD_SAMPLE, evt->header.pid, evt->header.tid, evt->header.time, evt->period );
    
    return true;
}

static bool decodeMmap( struct record_mmap *evt ) {
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_process( &recTree, evt->header.pid );
        try( proc != NULL );
    }
    
    struct rmmap *old = proc->mmaps;
    proc->mmaps = (struct rmmap *)malloc( sizeof(*proc->mmaps) );
    trysys( proc->mmaps != NULL );
    proc->mmaps->next = old;
    proc->mmaps->start = evt->start;
    proc->mmaps->end = evt->start + evt->len - 1;
    proc->mmaps->pgoff = evt->pgoff;
    memcpy( proc->mmaps->filename, evt->filename, sizeof(proc->mmaps->filename) );
    
    log_overview_s( evt->header.nr, PERF_RECORD_MMAP, evt->header.pid, evt->header.tid, 0, evt->filename );
    return true;
}

static bool decodeComm( struct record_comm* evt ) {
    struct process* proc = find_process( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_process( &recTree, evt->header.pid );
        try( proc != NULL );
    }
    
    memcpy( proc->comm, evt->comm, sizeof(proc->comm) );
    
    log_overview_s( evt->header.nr, PERF_RECORD_COMM, evt->header.pid, evt->header.tid, 0, evt->comm );
    return true;
}

static bool decodeFork( struct record_fork *evt ) {
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
    
    log_overview_i( evt->header.nr, PERF_RECORD_FORK, evt->header.pid, evt->header.tid, evt->header.time, evt->ppid );
    return true;
}

static bool decodeExit( struct record_fork *evt ) {
    struct process* proc = find_process( &recTree, evt->header.pid );
    trymsg( proc != NULL, ERR_ENTRY_NOT_FOUND, __func__ );
    
    proc->exit_time = evt->header.time;
    
    try( remove_process( &recTree, proc ) );
    
    print_process( proc, summary_file );
    free( proc );
    
    log_overview( evt->header.nr, PERF_RECORD_EXIT, evt->header.pid, evt->header.tid, evt->header.time );
    return true;
}

void handleRecord(struct record_t *proc, void *data){
    (void)data;
    switch( proc->type ){
        case PERF_RECORD_COMM:{
            decodeComm( (struct record_comm*)proc );
            break;
        }
        case PERF_RECORD_MMAP:{
            decodeMmap( (struct record_mmap*)proc );
            break;
        }
        case PERF_RECORD_FORK:{
            decodeFork( (struct record_fork*)proc );
            break;
        }
        case PERF_RECORD_EXIT:{
            decodeExit( (struct record_fork*)proc );
            break;
        }
        case PERF_RECORD_SAMPLE:{
            decodeSample( (struct record_sample*)proc );
            break;
        }
        default:{
            log_overview( proc->nr, proc->type, 0, 0, proc->time );
            break;
        }
    }
};

int main( int argc, char **argv ) {
    if( argc != 2 ){
        printf( "expect a file name\n" );
        return -1;
    }
    
    int fd = open( argv[1], O_RDONLY );
    if( fd < 0 ){
        perror( "open data file\n" );
        return -1;
    }
    
    
    memset( count, 0, sizeof(count) );

    overview_file  = fopen( OVERVIEW_LOG_NAME, "w+" );
    fprintf( overview_file, "nr%stype%spid%stid%stime%sinfo\n", SEP, SEP, SEP, SEP, SEP );
    
    
    if( !read_perf_file( fd ) ){
        print_last_error();
        return -1;
    }
    
    summary_file  = fopen( SUMMARY_LOG_NAME, "w+" );
    print_process_header( summary_file );
    recTree = init_processes();
    iterate_order( &orderTree, handleRecord, NULL );
    print_processes( &recTree, summary_file );
    fclose( summary_file );
    
    fclose( overview_file );
    
    printf( "found events\n" );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        printf( "  %s: %i\n", NAMES[i], count[i] );
    }
    printf( "  unknown: %i\n", count[0] );
    printf( "\n" );
    
    FILE *res  = fopen( "results.csv", "w+" );
    func_print_detailed( res );
    fclose( res );
    
    func_print_overview( stdout );
    
    return 0;
}
