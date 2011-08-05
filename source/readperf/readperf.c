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

static bool produce_statistic = false;

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

#define STAT_LOG_NAME   "stat.csv"
#define FUNC_LOG_NAME   "func.csv"

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

static bool decodeSample( u64 ev_nr, union perf_event *evt ) {
    struct perf_sample  sample;
    memset( &sample, 0, sizeof(sample) );
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        set_last_error( ERR_DECODE_SAMPLE, NULL );
        return false;
    }
    
    struct event_type_entry* entry = get_entry( sample.id );
    try( entry != NULL );
    entry->count++;
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
        struct mmap_entry *entry = mm_get_entry(sample.pid, sample.tid, sample.ip);
        entry->samples++;
        entry->period += sample.period;
        
        if( produce_statistic ){
            struct func_dir* func;
            
            if( entry == mm_get_error() ){
                func = func_error();
            } else {
                func = force_entry( sample.ip, entry->filename );
                if( func == NULL ){
                    func = func_error();
                }
            }
            
            try( func != NULL );
            
            func->period += sample.period;
            func->samples++;
        }
    }
    /*    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
     * struct log_entry *entry = forceLogEntry( sample.pid, sample.tid, sample.ip, NULL, NULL );
     * entry->period += sample.period;
     * entry->samples++;
     * }*/
    printSample( sample_fid, ev_nr, &sample, get_sampling_type(), SEP );
    log_overview_i( ev_nr, PERF_RECORD_SAMPLE, sample.pid, sample.tid, sample.time, sample.period );
    
    return true;
}

static bool decodeMmap( u64 ev_nr, union perf_event *evt ) {
    if( produce_statistic ){
        struct mmap_entry *entry = mm_add_entry();
        if( entry == NULL ){
            return false;
        }
        entry->pid = evt->mmap.pid;
        entry->tid = evt->mmap.tid;
        entry->start = evt->mmap.start;
        entry->end   = evt->mmap.start+evt->mmap.len-1;
        memcpy( entry->filename, evt->mmap.filename, sizeof(entry->filename) );
    }
    
    /*    struct log_entry *entry = forceLogEntry( evt->mmap.pid, evt->mmap.tid, 0, NULL, evt->mmap.filename );
     * if( entry->filename == NULL ){
     * entry->filename = strndup( evt->mmap.filename, sizeof( evt->mmap.filename) );
     * }*/
    printMmap( mmap_file, ev_nr, &evt->mmap, SEP );
    log_overview_s( ev_nr, PERF_RECORD_MMAP, evt->mmap.pid, evt->mmap.tid, 0, evt->mmap.filename );
    return true;
}

static bool decodeComm( u64 ev_nr, union perf_event *evt ) {
    (void)evt;
    /*    struct log_entry *entry = forceLogEntry( evt->comm.pid, evt->comm.tid, 0, evt->comm.comm, NULL );
     * if( entry->comm == NULL ){
     * entry->comm = strndup( evt->comm.comm, sizeof( evt->comm.comm) );
     * }*/
    printComm( comm_file, ev_nr, &evt->comm, SEP );
    log_overview_s( ev_nr, PERF_RECORD_COMM, evt->comm.pid, evt->comm.tid, 0, evt->comm.comm );
    return true;
}

static bool decodeFork( u64 ev_nr, union perf_event *evt ) {
    printFork( fork_file, ev_nr, &evt->fork, SEP );
    log_overview_i( ev_nr, PERF_RECORD_FORK, evt->fork.pid, evt->fork.tid, evt->fork.time, evt->fork.ppid );
    return true;
}

static bool decodeExit( u64 ev_nr, union perf_event *evt ) {
    printExit( exit_file, ev_nr, &evt->fork, SEP );
    log_overview( ev_nr, PERF_RECORD_EXIT, evt->fork.pid, evt->fork.tid, evt->fork.time );
    return true;
}

bool buildMap() {
    u64 ev_nr = 0;
    printMmapHeader( mmap_file, SEP );
    printCommHeader( comm_file, SEP );
    printForkHeader( fork_file, SEP );
    printExitHeader( exit_file, SEP );
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        switch( evt.header.type ){
            case PERF_RECORD_COMM:{
                try( read_event_data( &evt ) );
                try( decodeComm( ev_nr, &evt ) );
                break;
            }
            case PERF_RECORD_MMAP:{
                try( read_event_data( &evt ) );
                try( decodeMmap( ev_nr, &evt ) );
                break;
            }
            case PERF_RECORD_FORK:{
                try( read_event_data( &evt ) );
                try( decodeFork( ev_nr, &evt ) );
                break;
            }
            case PERF_RECORD_EXIT:{
                try( read_event_data( &evt ) );
                try( decodeExit( ev_nr, &evt ) );
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

bool readEvents() {
    u64 ev_nr = 0;
    printSampleHeader( sample_fid, get_sampling_type(), SEP );
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        if( (evt.header.type > 0) && (evt.header.type < PERF_RECORD_MAX) ){
            count[evt.header.type]++;
        } else {
            count[0]++;
        }
        switch( evt.header.type ){
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
    comm_file  = fopen( COMM_LOG_NAME, "w+" );
    fork_file  = fopen( FORK_LOG_NAME, "w+" );
    exit_file  = fopen( EXIT_LOG_NAME, "w+" );
    if( !buildMap() ){
        print_last_error();
        return -1;
    }
    fclose( exit_file );
    fclose( fork_file );
    fclose( comm_file );
    fclose( mmap_file );
    
    goto_start_data();
    
    sample_fid  = fopen( SAMPLE_LOG_NAME, "w+" );
    if( !readEvents() ){
        print_last_error();
        return -1;
    }
    fclose( sample_fid );
    fclose( overview_file );
    
    printf( "found events\n" );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        printf( "  %s: %i\n", NAMES[i], count[i] );
    }
    printf( "  unknown: %i\n", count[0] );
    printf( "\n" );
    
    if( produce_statistic ){
        printf( "Detailed:\n" );
        for( i = 0; i < get_entry_count(); i++ ){
            struct event_type_entry* entry = get_entry_by_index( i );
            printf( "  %s: %i\n", entry->name, entry->count );
        }
        
        FILE *stat_fid  = fopen( STAT_LOG_NAME, "w+" );
        mm_print( stat_fid, false );
        fclose( stat_fid );
        
        FILE *func_fid = fopen( FUNC_LOG_NAME, "w+" );
        func_print( func_fid );
        fclose( func_fid );
        
        fprintf( stdout, "\nStatistics per function:\n" );
        func_stat_print( stdout );
    }
    
    return 0;
}


