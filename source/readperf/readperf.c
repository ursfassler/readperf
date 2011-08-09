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
#include    "memorymap.h"
#include    "funcstat.h"
#include    "records.h"
#include    "processes.h"

static u32 sample_fail = 0;
static u32 mmap_fail = 0;
static u32 comm_fail = 0;


#define     MAGIC_ACCESS_DISTANCE   100

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

record_tree_t procOrder;
record_order_tree_t orderTree;

static const char *NAMES[] = { NULL, "MMAP", "LOST", "COMM", "EXIT", "THROTTLE", "UNTHROTTLE", "FORK", "READ", "SAMPLE" };

void checkOrder(struct record_t *proc, void *data){
    (void)data;
    
    switch( proc->type ){
        case OE_START: start_process( &procOrder, proc->pid, proc->tid, proc->time, proc->nr ); break;
        case OE_END:   stop_process( &procOrder, proc->pid, proc->tid, proc->time, proc->nr );  break;
    }
};

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

static bool decodeSample( u64 ev_nr, union perf_event *evt ) {
    struct perf_sample  sample;
    memset( &sample, 0, sizeof(sample) );
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        set_last_error( ERR_DECODE_SAMPLE, NULL );
        return false;
    }
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_TIME)) == (PERF_SAMPLE_TID | PERF_SAMPLE_TIME) ){
        struct process* run = find_process( &procOrder, sample.pid, sample.tid, sample.time, ev_nr );
        
        if( run != NULL ){
            run->samples++;
            run->period += sample.period;
        } else {
            sample_fail++;
        }
    }
    
    struct event_type_entry* entry = get_entry( sample.id );
    try( entry != NULL );
    entry->count++;
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
        struct mmap_entry *entry = mm_get_entry(sample.pid, sample.tid, sample.ip);
        entry->samples++;
        entry->period += sample.period;
    }
    
    printSample( sample_fid, ev_nr, &sample, get_sampling_type(), SEP );
    log_overview_i( ev_nr, PERF_RECORD_SAMPLE, sample.pid, sample.tid, sample.time, sample.period );
    
    return true;
}

static bool decodeMmap( u64 ev_nr, struct mmap_event *evt ) {
    add_record_order( &orderTree, evt->pid == (u32)-1 ? 0 : evt->pid, evt->tid, 0, ev_nr, OE_START );
    
    printMmap( mmap_file, ev_nr, evt, SEP );
    log_overview_s( ev_nr, PERF_RECORD_MMAP, evt->pid, evt->tid, 0, evt->filename );
    return true;
}

static bool decodeComm( u64 ev_nr, struct comm_event *evt ) {
    add_record_order( &orderTree, evt->pid, evt->tid, 0, ev_nr, OE_START );
    
    printComm( comm_file, ev_nr, evt, SEP );
    log_overview_s( ev_nr, PERF_RECORD_COMM, evt->pid, evt->tid, 0, evt->comm );
    return true;
}

static bool decodeFork( u64 ev_nr, struct fork_event *evt ) {
    add_record_order( &orderTree, evt->pid, evt->tid, evt->time, ev_nr, OE_START );
    
    printFork( fork_file, ev_nr, evt, SEP );
    log_overview_i( ev_nr, PERF_RECORD_FORK, evt->pid, evt->tid, evt->time, evt->ppid );
    return true;
}

static bool decodeExit( u64 ev_nr, struct fork_event *evt ) {
    add_record_order( &orderTree, evt->pid, evt->tid, evt->time, ev_nr, OE_END );
    
    printExit( exit_file, ev_nr, evt, SEP );
    log_overview( ev_nr, PERF_RECORD_EXIT, evt->pid, evt->tid, evt->time );
    return true;
}

bool readEvents() {
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
            case PERF_RECORD_COMM:{
                try( read_event_data( &evt ) );
                try( decodeComm( ev_nr, &evt.comm ) );
                break;
            }
            case PERF_RECORD_MMAP:{
                try( read_event_data( &evt ) );
                try( decodeMmap( ev_nr, &evt.mmap ) );
                break;
            }
            case PERF_RECORD_FORK:{
                try( read_event_data( &evt ) );
                try( decodeFork( ev_nr, &evt.fork ) );
                break;
            }
            case PERF_RECORD_EXIT:{
                try( read_event_data( &evt ) );
                try( decodeExit( ev_nr, &evt.fork ) );
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

bool applySamples() {
    u64 ev_nr = 0;
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        switch( evt.header.type ){
            case PERF_RECORD_COMM:{
                try( read_event_data( &evt ) );
                struct process* run = find_process( &procOrder, evt.comm.pid, evt.comm.tid, (u64)-1, ev_nr );
                if( run != NULL ){
                    memcpy( run->comm, evt.comm.comm, sizeof(run->comm) );
                } else {
                    comm_fail++;
                }
                break;
            }
            case PERF_RECORD_MMAP:{
                try( read_event_data( &evt ) );
                struct process* run = find_process( &procOrder, evt.mmap.pid == (u32)-1 ? 0 : evt.mmap.pid, evt.mmap.tid, (u64)-1, ev_nr );
                if( run != NULL ){
                    struct rmmap *old = run->mmaps;
                    run->mmaps = (struct rmmap *)malloc( sizeof(*run->mmaps) );
                    run->mmaps->next = old;
                    run->mmaps->start = evt.mmap.start;
                    run->mmaps->end = evt.mmap.start + evt.mmap.len - 1;
                    run->mmaps->pgoff = evt.mmap.pgoff;
                    memcpy( run->mmaps->filename, evt.mmap.filename, sizeof(run->mmaps->filename) );
                } else {
                    mmap_fail++;
                }
                break;
            }
            case PERF_RECORD_SAMPLE:{
                try( read_event_data( &evt ) );
                try( decodeSample( ev_nr, &evt ) );
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

static void printSummaryHeader(){
    fprintf( summary_file, "%s,%s,%s,%s,%s,%s,%s,%s,%s\n", "pid", "tid", "comm", "fork_time", "exit_time", "first_nr", "last_nr", "samples", "period" );
};

static void run_cb(struct process *run, struct pid_record *rec){
    fprintf( summary_file, "%u,%u,\"%s\",%llu,%llu,%llu,%llu,%llu,%llu\n", rec->pid, rec->tid, run->comm, run->fork_time, run->exit_time, run->first_nr, run->last_nr, run->samples, run->period );
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
    
    procOrder   = init_records();
    iterate_order( &orderTree, checkOrder, NULL );
    
    goto_start_data();
    if( !applySamples() ){
        print_last_error();
        return -1;
    }
    
    fclose( sample_fid );
    fclose( exit_file );
    fclose( fork_file );
    fclose( comm_file );
    fclose( mmap_file );
    fclose( overview_file );
    
    summary_file  = fopen( SUMMARY_LOG_NAME, "w+" );
    printSummaryHeader();
    iterate_process( &procOrder, run_cb );
    fclose( summary_file );
    
    printf( "found events\n" );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        printf( "  %s: %i\n", NAMES[i], count[i] );
    }
    printf( "  unknown: %i\n", count[0] );
    printf( "\n" );
    
//    print_records( &procOrder, stdout );
    
    printf( "sample_fail: %u\n", sample_fail );
    printf( "mmap_fail: %u\n", mmap_fail );
    printf( "comm_fail: %u\n", comm_fail );
    
    /*    fprintf( stdout, "-- activeTree tree --\n" );
     * print_processes( &activeTree, stdout );
     * fprintf( stdout, "-- dead tree --\n" );
     * print_processes( &deadTree, stdout );*/
    
    return 0;
}
