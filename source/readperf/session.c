
#include    <stdlib.h>
#include    <string.h>
#include    <stdio.h>
#include    "session.h"
#include    "errhandler.h"

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

bool readn( int fd, void *buf, size_t count )
{
  while( count > 0 ){
    int ret = read( fd, buf, count );
    trysys( ret >= 0 );
    if( ret == 0 ){
        set_last_error( ERR_FILE_END_TO_EARLY, NULL );
        return false;
    } else {
        count = count - ret;
        buf   = ((char*)buf) + ret;
    }
  }
  return true;
}

static bool add_link( u64 id, struct event_type_entry *entry ){
    if( linkmemory == 0 ){
        linkmemory  = 4;
        idlink  = (struct event_id_link *)malloc( sizeof(*idlink) * linkmemory );
        trysys( idlink != NULL );
    } else if( linkmemory == linkcount ) {
        struct event_id_link *old = idlink;
        linkmemory  *= 2;
        idlink  = (struct event_id_link *)malloc( sizeof(*idlink) * linkmemory );
        trysys( idlink != NULL );
        memcpy( idlink, old, sizeof(*idlink) * linkcount );
    }
    idlink[linkcount].id = id;
    idlink[linkcount].entry = entry;
    linkcount++;
    return true;
};

static bool readAttr() {
    struct perf_file_attr f_attr;
    
    if( (fheader.attrs.size % sizeof( struct perf_file_attr )) != 0 ){
        char buf[256];
        snprintf( buf, sizeof(buf), "problem with attrs size: %llu %lu", fheader.attrs.size, sizeof( struct perf_file_attr ) );
        set_last_error( ERR_SIZE_MISMATCH, strdup(buf) );
        return false;
    }
    eventAttrCount = fheader.attrs.size / sizeof( struct perf_file_attr );
    eventAttr = (struct event_type_entry *)malloc( sizeof( *eventAttr ) * eventAttrCount );
    
    trysys( lseek( i_fd, fheader.attrs.offset, SEEK_SET ) >= 0 );
    
    /* special case if there is only one event
     * events don't have an ID tag, use default value
     */
    if( eventAttrCount == 1 ){
        try( add_link( -1ULL, &eventAttr[0] ) );
    }
    
    off_t pos = lseek( i_fd, 0, SEEK_CUR );
    unsigned int i;
    for( i = 0; i < eventAttrCount; i++ ){
        trysys( lseek( i_fd, pos, SEEK_SET ) >= 0 );
        try( readn( i_fd, &f_attr, sizeof(f_attr) ) );
        pos = lseek( i_fd, 0, SEEK_CUR );
        trysys( pos >= 0 );
        
//        eventAttr[i].attr = f_attr.attr;
        eventAttr[i].config = f_attr.attr.config;
        eventAttr[i].count = 0;
        
        if( i == 0 ){
            samplingType = f_attr.attr.sample_type;
            if( !f_attr.attr.sample_id_all ){
                set_last_error( ERR_NOT_YET_DEFINED, "need attr.sample_id_all" );
                return false;
            }
        } else {
            if( samplingType != f_attr.attr.sample_type ){
                set_last_error( ERR_SAMPLES_DIFFER_IN_TYPE, NULL );
                return false;
            }
        }
        
        u64   f_id;
        unsigned int idcount = f_attr.ids.size / sizeof(f_id);
        
        if( idcount > 0 ){
            trysys( lseek( i_fd, f_attr.ids.offset, SEEK_SET ) >= 0 );
            unsigned int k;
            for( k = 0; k < idcount; k++ ){
                try( readn( i_fd, &f_id, sizeof( f_id ) ) );
                try( add_link( f_id, &eventAttr[i] ) );
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
    
    return true;
}

static bool readTypes() {
    unsigned int traceInfoCount = 0;
    struct perf_trace_event_type *traceInfo = NULL;
    
    if( (fheader.event_types.size % sizeof( struct perf_trace_event_type )) != 0 ){
        char buf[256];
        snprintf( buf, sizeof(buf), "problem with event_types size: %llu %lu\n", fheader.event_types.size, sizeof( struct perf_file_header ) );
        set_last_error( ERR_SIZE_MISMATCH, strdup(buf) );
        return false;
    }
    traceInfoCount = fheader.event_types.size / sizeof( struct perf_trace_event_type );
    
    trysys( lseek( i_fd, fheader.event_types.offset, SEEK_SET ) >= 0 );
    
    traceInfo = (struct perf_trace_event_type *)malloc( fheader.event_types.size );
    trysys( traceInfo != NULL );
    
//    printf( "event offset: %llu size: %llu\n", fheader.event_types.offset, fheader.event_types.size );
//    printf( "found %i events\n", traceInfoCount );
    
    try( readn( i_fd, traceInfo, fheader.event_types.size ) );
    
    unsigned int i, k;
    for( i = 0; i < eventAttrCount; i++ ){
        for( k = 0; k < traceInfoCount; k++ ){
            if( eventAttr[i].config == traceInfo[k].event_id ){
                memcpy( &eventAttr[i].name, traceInfo[k].name, sizeof(eventAttr[i].name) );
                break;
            }
        }
        if( k == traceInfoCount ){
            char buf[256];
            snprintf( buf, sizeof(buf), "%llu", eventAttr[i].config );
            set_last_error( ERR_TRACE_INFO_NOT_FOUND_FOR_CONFIG, strdup(buf) );
            return false;
        }
    }
    return true;
}

// ---- public ----

u64 get_sampling_type(){
    return  samplingType;
}

bool has_more_events(){
    off_t off = lseek( i_fd, 0, SEEK_CUR );
    trysys( off >= 0 ); //TODO: throw exception
    return off < (int)(fheader.data.offset+fheader.data.size);
}

bool next_event_header( struct perf_event_header* header ){
    try( readn( i_fd, header, sizeof(*header) ) );
    return true;
};

bool read_event_data( union perf_event *evt ){
    try( evt->header.size <= sizeof(*evt) );
    try( readn( i_fd,  &(evt->sample.array[0]), evt->header.size-sizeof(evt->header) ) );
    return true;
};

bool skip_event_data( struct perf_event_header* header ){
    try( lseek( i_fd, header->size-sizeof(*header), SEEK_CUR ) >= 0 );
    return true;
};

bool goto_start_data(){
    trysys( lseek( i_fd, fheader.data.offset, SEEK_SET ) >= 0 );
    return true;
}

bool start_session( int fd ){
    i_fd = fd;
    try( readn( i_fd, &fheader, sizeof(fheader) ) );
    
    const char MAGIC[8] = { 'P', 'E', 'R', 'F', 'F', 'I', 'L', 'E' };
    
    if( fheader.magic != *((u64 *)MAGIC) ){
        set_last_error( ERR_NO_PERF_FILE, NULL );
        return false;
    }

    if( fheader.attr_size != sizeof(struct perf_file_attr) ){
        set_last_error( ERR_SIZE_MISMATCH, "maybe needed swap but not implemented" );
        return false;
    };
    
    try( readAttr() );
    try( readTypes() );
    
    return goto_start_data();
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
    set_last_error( ERR_ENTRY_NOT_FOUND, "session:get_entry" );
    return NULL;
};

