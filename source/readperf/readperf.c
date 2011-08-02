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

#define SEP ","
static int count[PERF_RECORD_MAX];

#define SAMPLE_LOG_NAME "sample.csv"
static FILE *sample_fid = NULL;
#define MMAP_LOG_NAME   "mmap.csv"
static FILE *mmap_file = NULL;
#define COMM_LOG_NAME   "comm.csv"
static FILE *comm_file = NULL;
#define STAT_LOG_NAME   "stat.csv"
#define FUNC_LOG_NAME   "func.csv"


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
        
        struct func_dir* func;
        
        if( entry == mm_get_error() ){
            func = func_error();
        } else {
            func = force_entry( sample.ip, entry->filename );
        }
        
        trysys( func != NULL );
        
        func->period += sample.period;
        func->samples++;
    }
    /*    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
     * struct log_entry *entry = forceLogEntry( sample.pid, sample.tid, sample.ip, NULL, NULL );
     * entry->period += sample.period;
     * entry->samples++;
     * }*/
    printSample( sample_fid, ev_nr, &sample, get_sampling_type(), SEP );
    
    return true;
}

static bool decodeMmap( u64 ev_nr, union perf_event *evt ) {
    (void)evt;
    struct mmap_entry *entry = mm_add_entry();
    if( entry == NULL ){
        return false;
    }
    entry->pid = evt->mmap.pid;
    entry->tid = evt->mmap.tid;
    entry->start = evt->mmap.start;
    entry->end   = evt->mmap.start+evt->mmap.len-1;
    memcpy( entry->filename, evt->mmap.filename, sizeof(entry->filename) );
    
    /*    struct log_entry *entry = forceLogEntry( evt->mmap.pid, evt->mmap.tid, 0, NULL, evt->mmap.filename );
     * if( entry->filename == NULL ){
     * entry->filename = strndup( evt->mmap.filename, sizeof( evt->mmap.filename) );
     * }*/
    printMmap( mmap_file, ev_nr, &evt->mmap, SEP );
    return true;
}

static bool decodeComm( u64 ev_nr, union perf_event *evt ) {
    (void)evt;
    /*    struct log_entry *entry = forceLogEntry( evt->comm.pid, evt->comm.tid, 0, evt->comm.comm, NULL );
     * if( entry->comm == NULL ){
     * entry->comm = strndup( evt->comm.comm, sizeof( evt->comm.comm) );
     * }*/
    printComm( comm_file, ev_nr, &evt->comm, SEP );
    return true;
}

bool buildMap() {
    u64 ev_nr = 0;
    printMmapHeader( mmap_file, SEP );
    printCommHeader( comm_file, SEP );
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
    
    mmap_file  = fopen( MMAP_LOG_NAME, "w+" );
    comm_file  = fopen( COMM_LOG_NAME, "w+" );
    if( !buildMap() ){
        print_last_error();
        return -1;
    }
    fclose( comm_file );
    fclose( mmap_file );
    
    goto_start_data();

    sample_fid  = fopen( SAMPLE_LOG_NAME, "w+" );
    if( !readEvents() ){
        print_last_error();
        return -1;
    }
    fclose( sample_fid );
    
    static const char *REC_NAME[] = { NULL, "MMAP", "LOST", "COMM", "EXIT", "THROTTLE", "UNTHROTTLE", "FORK", "READ", "SAMPLE" };
    
    printf( "found events\n" );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        printf( "  %s: %i\n", REC_NAME[i], count[i] );
    }
    printf( "  unknown: %i\n", count[0] );
    printf( "\n" );
    
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
    
    return 0;
}


