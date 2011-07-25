
#include    <stdlib.h>
#include    <string.h>
#include    "session.h"


struct event_id_link {
    u64    id;
    struct event_type_entry   *entry;
};

static struct event_id_link *idlink = NULL;
static unsigned int linkmemory = 0;
static unsigned int linkcount = 0;

static unsigned int eventAttrCount = 0;
static struct event_type_entry *eventAttr = NULL;

static u64 samplingType = 0;

static struct perf_file_header fheader;
static int i_fd = -1;

/*static void printSampleFormat( __u64 type ) {
    static const char *names[] = { "IP", "TID", "TIME", "ADDR", "READ", "CALLCHAIN", "ID", "CPU", "PERIOD", "STREAM_ID", "RAW" };
    unsigned int i;
    
    for( i = 0; i < sizeof(names)/sizeof(names[0]); i++ ){
        if( (type & 1) == 1 ){
            printf( " %s", names[i] );
        }
        type = type >> 1;
    }
}*/

int readn( int fd, void *buf, size_t count )
{
  while( count > 0 ){
    int ret = read( fd, buf, count );
    if( ret <= 0 ){
      return -1;
    }
    count = count - ret;
    buf   = buf + ret;
  }
  return 0;
}

static int add_link( u64 id, struct event_type_entry *entry ){
    if( linkmemory == 0 ){
        linkmemory  = 4;
        idlink  = malloc( sizeof(*idlink) * linkmemory );
        if( idlink == NULL ){
            return -1;
        }
    } else if( linkmemory == linkcount ) {
        struct event_id_link *old = idlink;
        linkmemory  *= 2;
        idlink  = malloc( sizeof(*idlink) * linkmemory );
        if( idlink == NULL ){
            return -1;
        }
        memcpy( idlink, old, sizeof(*idlink) * linkcount );
    }
    idlink[linkcount].id = id;
    idlink[linkcount].entry = entry;
    linkcount++;
    return 0;
};

static void* readAttr() {
    struct perf_file_attr f_attr;
    
    if( (fheader.attrs.size % sizeof( struct perf_file_attr )) != 0 ){
        fprintf( stderr, "problem with attrs size: %llu %lu\n", fheader.attrs.size, sizeof( struct perf_file_attr ) );
        return NULL;
    }
    eventAttrCount = fheader.attrs.size / sizeof( struct perf_file_attr );
    eventAttr = malloc( sizeof( *eventAttr ) * eventAttrCount );
    
    if( lseek( i_fd, fheader.attrs.offset, SEEK_SET ) < 0 ){
        return NULL;
    }
    
    /* special case if there is only one event
     * events don't have an ID tag, use default value
     */
    if( eventAttrCount == 1 ){
        add_link( -1ULL, &eventAttr[0] );
    }
    
    unsigned int pos = lseek( i_fd, 0, SEEK_CUR );
    unsigned int i;
    for( i = 0; i < eventAttrCount; i++ ){
        lseek( i_fd, pos, SEEK_SET );
        if( readn( i_fd, &f_attr, sizeof(f_attr) ) < 0 ){
            return NULL;
        }
        pos = lseek( i_fd, 0, SEEK_CUR );
        
//        eventAttr[i].attr = f_attr.attr;
        eventAttr[i].config = f_attr.attr.config;
        eventAttr[i].count = 0;
        
        if( i == 0 ){
            samplingType = f_attr.attr.sample_type;
        } else {
            if( samplingType != f_attr.attr.sample_type ){
                fprintf( stderr, "samples differ in type\n" );
                exit( -1 );
            }
        }
        
        u64   f_id;
        unsigned int idcount = f_attr.ids.size / sizeof(f_id);
        
        if( idcount > 0 ){
            lseek( i_fd, f_attr.ids.offset, SEEK_SET );
            unsigned int k;
            for( k = 0; k < idcount; k++ ){
                readn( i_fd, &f_id, sizeof( f_id ) );
                add_link( f_id, &eventAttr[i] );
            }
        }
        
/*        printf( "idcount: %u\n", idcount );
        printf( "idoffset: %llu\n", f_attr.ids.offset );
        
        printf( "type: %u\n", f_attr.attr.type );
        printf( "size: %u\n", f_attr.attr.size );
        printf( "config: %llu\n", f_attr.attr.config );
        printf( "sample_type: %llu:", f_attr.attr.sample_type );
        printSampleFormat( f_attr.attr.sample_type );
        printf( "\n" );
        printf( "read_format: %llu\n", f_attr.attr.read_format );
        printf( "bp_type: %u\n", f_attr.attr.bp_type );*/
    }
    
    return 0;
}

static void readTypes() {
    unsigned int traceInfoCount = 0;
    struct perf_trace_event_type *traceInfo = NULL;
    
    if( (fheader.event_types.size % sizeof( struct perf_trace_event_type )) != 0 ){
        fprintf( stderr, "problem with event_types size: %llu %lu\n", fheader.event_types.size, sizeof( struct perf_file_header ) );
        return;
    }
    traceInfoCount = fheader.event_types.size / sizeof( struct perf_trace_event_type );
    
    if( lseek( i_fd, fheader.event_types.offset, SEEK_SET ) < 0 ){
        return;
    };
    
    traceInfo = malloc( fheader.event_types.size );
    
//    printf( "event offset: %llu size: %llu\n", fheader.event_types.offset, fheader.event_types.size );
//    printf( "found %i events\n", traceInfoCount );
    
    readn( i_fd, traceInfo, fheader.event_types.size );
    
    unsigned int i, k;
    for( i = 0; i < eventAttrCount; i++ ){
        for( k = 0; k < traceInfoCount; k++ ){
            if( eventAttr[i].config == traceInfo[k].event_id ){
                memcpy( &eventAttr[i].name, traceInfo[k].name, sizeof(eventAttr[i].name) );
                break;
            }
        }
        if( k == traceInfoCount ){
            fprintf( stderr, "trace info not found for config: %llu\n", eventAttr[i].config );
            exit( -1 );
        }
    }
    return;
}

// ---- public ----

u64 get_sampling_type(){
    return  samplingType;
}

int next_event_header( struct perf_event_header* header ){
    if( lseek( i_fd, 0, SEEK_CUR ) >= (int)(fheader.data.offset+fheader.data.size) ){
        return 0;
    }
    if( readn( i_fd, header, sizeof(*header) ) < 0 ){
        return -1;
    }
    return 1;
};

int read_event_data( union perf_event *evt ){
    if( evt->header.size > sizeof(*evt) ){
        return -1;
    }
    if( readn( i_fd,  &(evt->sample.array[0]), evt->header.size-sizeof(evt->header) ) < 0 ){
        return -1;
    }
    return 0;
};

int skip_event_data( struct perf_event_header* header ){
    if( lseek( i_fd, header->size-sizeof(*header), SEEK_CUR ) < 0 ){
        return -1;
    }
    return 0;
};

int start_session( int fd ){
    i_fd = fd;
    if( readn( i_fd, &fheader, sizeof(fheader) ) < 0 ){
        printf( "problem during reading\n" );
        return -1;
    }
    
    if (fheader.attr_size != sizeof(struct perf_file_attr)) {
        fprintf( stderr, "data corrupt (maybe needed swap but not implemented)\n" );
    }
    
    readAttr();
    readTypes();
    
    return lseek( i_fd, fheader.data.offset, SEEK_SET );
}

unsigned int get_entry_count(){
    return eventAttrCount;
}

struct event_type_entry* get_entry_by_index( unsigned int index ){
    return &eventAttr[index];
};

struct event_type_entry* get_entry( u64 id ){
    unsigned int i;
    for( i = 0; i < linkcount; i++ ){
        if( idlink[i].id == id ){
            return idlink[i].entry;
        }
    }
    return NULL;
};

