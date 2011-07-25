#include  <stdio.h>
#include  <linux/types.h>
#include  <linux/perf_event.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <string.h>
#include  "types.h"
#include  "evsample.h"
#include  "evtypes.h"
#include  "session.h"

#define try( func, msg )  if( func < 0 ){ perror( msg ); return -1; }

static int count[PERF_RECORD_MAX];

static int decodeSample( union perf_event *evt ) {
    struct perf_sample  sample;
    memset( &sample, 0, sizeof(sample) );
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        fprintf( stderr, "failed: perf_event__parse_sample" );
        return -1;
    }
    
    struct event_type_entry* entry = get_entry( sample.id );
    if( entry == NULL ){
        fprintf( stderr, "id not found: %llu\n", sample.id );
        return -1;
    }
    entry->count++;
    
    return 0;
}

int readEvents() {
    while( 1 ){
        union perf_event evt;
        switch( next_event_header( &evt.header ) ){
            case -1: return -1;
            case 0: return 0;
            case 1: break;
        }
        if( (evt.header.type > 0) && (evt.header.type < PERF_RECORD_MAX) ){
            count[evt.header.type]++;
        } else {
            count[0]++;
        }
        if( evt.header.type == PERF_RECORD_SAMPLE ){
            if( read_event_data( &evt ) < 0 ){
                return -1;
            }
            decodeSample( &evt );
        } else {
            if( skip_event_data( &evt.header ) < 0 ){
                return -1;
            }
        }
    }
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
    
    start_session( fd );
    readEvents( fd );
    
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


