#include  <stdio.h>
#include  <linux/types.h>
#include  <linux/perf_event.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <string.h>
#include  "types.h"
#include  "origperf.h"
#include  "evtypes.h"
#include  "session.h"
#include    "errhandler.h"

#define try( func )  if( !(func) ){ return false; }

static int count[PERF_RECORD_MAX];

static bool decodeSample( union perf_event *evt ) {
    struct perf_sample  sample;
    memset( &sample, 0, sizeof(sample) );
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        set_last_error( ERR_DECODE_SAMPLE, NULL );
        return false;
    }
    
    struct event_type_entry* entry = get_entry( sample.id );
    try( entry != NULL );
    entry->count++;
    
    return true;
}

bool readEvents() {
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        if( (evt.header.type > 0) && (evt.header.type < PERF_RECORD_MAX) ){
            count[evt.header.type]++;
        } else {
            count[0]++;
        }
        if( evt.header.type == PERF_RECORD_SAMPLE ){
            try( read_event_data( &evt ) );
            try( decodeSample( &evt ) );
        } else {
            try( skip_event_data( &evt.header ) );
        }
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
    if( !readEvents() ){
        print_last_error();
        return -1;
    }
    
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
    
    return 0;
}


