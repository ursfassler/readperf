#include  <stdio.h>
#include  <linux/types.h>
#include  <linux/perf_event.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <string.h>
#include    <stdlib.h>
#include  "types.h"
#include  "origperf.h"
#include  "evtypes.h"
#include  "session.h"
#include    "errhandler.h"
#include    "evtprint.h"
#include    "funcstat.h"
#include    "records.h"
#include    "processes.h"

#define SEP ","
static int count[PERF_RECORD_MAX];

#define SAMPLE_LOG_NAME "sample.csv"
static FILE *sample_fid = NULL;
#define MMAP_LOG_NAME   "mmap.csv"
static FILE *mmap_file = NULL;
#define COMM_LOG_NAME   "comm.csv"
static FILE *comm_file = NULL;
#define FORK_LOG_NAME   "fork.csv"
static FILE *fork_file = NULL;
#define EXIT_LOG_NAME   "exit.csv"
static FILE *exit_file = NULL;
#define OVERVIEW_LOG_NAME   "overview.csv"
static FILE *overview_file = NULL;

#define SUMMARY_LOG_NAME   "summary.csv"
static FILE *summary_file = NULL;


#define STAT_LOG_NAME   "stat.csv"
#define FUNC_LOG_NAME   "func.csv"

record_order_tree_t orderTree;
record_tree_t recTree;

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
    struct process* proc = find_record( &recTree, evt->header.pid );
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
    struct process* proc = find_record( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_record( &recTree, evt->header.pid );
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
    struct process* proc = find_record( &recTree, evt->header.pid );
    if( proc == NULL ){
        trymsg( evt->header.time == 0, ERR_NOT_YET_DEFINED, __func__ );
        proc = create_record( &recTree, evt->header.pid );
        try( proc != NULL );
    }
    
    memcpy( proc->comm, evt->comm, sizeof(proc->comm) );
    
    log_overview_s( evt->header.nr, PERF_RECORD_COMM, evt->header.pid, evt->header.tid, 0, evt->comm );
    return true;
}

static bool decodeFork( struct record_fork *evt ) {
    struct process* proc = find_record( &recTree, evt->header.pid );
    if( proc == NULL ){
        // if it is a thread, throw an error
        trymsg( evt->header.pid == evt->header.tid, ERR_PROC_NOT_FOUND, __func__ );
        proc = create_record( &recTree, evt->header.pid );
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
    struct process* proc = find_record( &recTree, evt->header.pid );
    trymsg( proc != NULL, ERR_ENTRY_NOT_FOUND, __func__ );
    
    proc->exit_time = evt->header.time;
    
    try( remove_record( &recTree, proc ) );
    
    print_record( proc, summary_file );
    free( proc );
    
    log_overview( evt->header.nr, PERF_RECORD_EXIT, evt->header.pid, evt->header.tid, evt->header.time );
    return true;
}

static struct record_t* create_mmap_msg( struct mmap_event *evt ){
    struct record_mmap* rec = (struct record_mmap*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->start = evt->start;
        rec->len = evt->len;
        rec->pgoff = evt->pgoff;
        memcpy( rec->filename, evt->filename, sizeof(rec->filename) );
    }
    return &rec->header;
};

static struct record_t* create_comm_msg( struct comm_event *evt ){
    struct record_comm* rec = (struct record_comm*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        memcpy( rec->comm, evt->comm, sizeof(rec->comm) );
    }
    return &rec->header;
};

static struct record_t* create_fork_msg( struct fork_event *evt ){
    struct record_fork* rec = (struct record_fork*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->ppid = evt->ppid;
        rec->ptid = evt->ptid;
    }
    return &rec->header;
};

static struct record_t* create_sample_msg( struct perf_sample *evt ){
    struct record_sample* rec = (struct record_sample*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->header.cpu = evt->cpu;
        rec->header.id = evt->id;
        rec->ip = evt->ip;
        rec->period = evt->period;
    }
    return &rec->header;
};

static bool readEvents() {
    u64 ev_nr = 0;
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        if( (evt.header.type > 0) && (evt.header.type < PERF_RECORD_MAX) ){
            count[evt.header.type]++;
        } else {
            count[0]++;
        }
        switch( evt.header.type ){
            case PERF_RECORD_MMAP:
            case PERF_RECORD_COMM:
            case PERF_RECORD_FORK:
            case PERF_RECORD_EXIT:
            case PERF_RECORD_SAMPLE: {
                try( read_event_data( &evt ) );
                struct perf_sample  sample;
                trymsg( perf_event__parse_sample( &evt, get_sampling_type(), true, &sample ) >= 0, ERR_NOT_YET_DEFINED, "readEvents:perf_event__parse_sample" );
                
                struct record_t *rec;
                
                switch( evt.header.type ){
                    case PERF_RECORD_MMAP: rec = create_mmap_msg( &evt.mmap ); break;
                    case PERF_RECORD_COMM: rec = create_comm_msg( &evt.comm ); break;
                    case PERF_RECORD_FORK: // fall throught
                    case PERF_RECORD_EXIT: rec = create_fork_msg( &evt.fork ); break;
                    case PERF_RECORD_SAMPLE: rec = create_sample_msg( &sample ); break;
                }
                
                rec->type = (enum perf_event_type)evt.header.type;
                rec->nr = ev_nr;
                rec->time = sample.time;
                rec->cpu = sample.cpu;
                rec->id = sample.id;
                
                add_record_order( &orderTree, rec );
                break;
            }
            default:{
                try( skip_event_data( &evt.header ) );
                break;
            }
        }
        ev_nr++;
    }
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
    
    orderTree  = init_record_order();
    
    memset( count, 0, sizeof(count) );
    
    if( !start_session( fd ) ){
        print_last_error();
        return -1;
    }
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) != (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
        set_last_error( ERR_NOT_YET_DEFINED, "not enough information in file" );
        print_last_error();
        return -1;
    }
    
    printf( "Sampling types: " );
    printSampleFormat( get_sampling_type() );
    printf( "\n" );
    
    overview_file  = fopen( OVERVIEW_LOG_NAME, "w+" );
    fprintf( overview_file, "nr%stype%spid%stid%stime%sinfo\n", SEP, SEP, SEP, SEP, SEP );
    
    mmap_file  = fopen( MMAP_LOG_NAME, "w+" );
    printMmapHeader( mmap_file, SEP );
    
    comm_file  = fopen( COMM_LOG_NAME, "w+" );
    printCommHeader( comm_file, SEP );
    
    fork_file  = fopen( FORK_LOG_NAME, "w+" );
    printForkHeader( fork_file, SEP );
    
    exit_file  = fopen( EXIT_LOG_NAME, "w+" );
    printExitHeader( exit_file, SEP );
    
    sample_fid  = fopen( SAMPLE_LOG_NAME, "w+" );
    printSampleHeader( sample_fid, get_sampling_type(), SEP );
    
    if( !readEvents() ){
        print_last_error();
        return -1;
    }
    
    summary_file  = fopen( SUMMARY_LOG_NAME, "w+" );
    print_record_header( summary_file );
    recTree = init_records();
    iterate_order( &orderTree, handleRecord, NULL );
    print_records( &recTree, summary_file );
    fclose( summary_file );
    
    fclose( sample_fid );
    fclose( exit_file );
    fclose( fork_file );
    fclose( comm_file );
    fclose( mmap_file );
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
