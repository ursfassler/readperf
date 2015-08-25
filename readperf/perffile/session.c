/**
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
 */

#include    <stdlib.h>
#include    <string.h>
#include    <stdio.h>
#include    "session.h"
#include    "../util/errhandler.h"

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

/**
 * To read the attributes into memory we first have to get the number of 
 * attribute instances of the structure @link perf_file_attr @endlink. To 
 * achieve this, .attrs.size is divided by the size of the containing structure
 * @link perf_file_attr @endlink. Then we can read the array of instances from 
 * the file offset .attrs.offset. For every instance we have to read the 
 * corresponding IDs. As for the whole structure, there can be several ID's. 
 * .ids.size is used to determine the number of IDs. If only one event source 
 * was used, there is no ID entry since all records belong to the single one 
 * @link perf_file_attr @endlink instance.
 *
 * We check that .attr.sample_id_all is set for all instances. This ensures that
 * all records have an timestamp and an identification entry. All instances are
 * checked that they have the same value for .attr.sample\_type.
 */
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
        
        trymsg( f_attr.attr.size == sizeof( f_attr.attr ), ERR_SIZE_MISMATCH, "f_attr.config.size" );
        
        eventAttr[i].attr = f_attr.attr;
        
        if( !f_attr.attr.sample_id_all ){
            set_last_error( ERR_NOT_YET_DEFINED, "need attr.sample_id_all" );
            return false;
        }
        if( i == 0 ){
            samplingType = f_attr.attr.sample_type;
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
    }
    
    return true;
}

/**
 * There can also be several instances of the @link perf_trace_event_type
 * @endlink in the file. As before, the .event_types.size is used to determine
 * the number of instances. By comparing .config of the @link perf_file_attr
 * @endlink instances with .event_id of the @link perf_trace_event_type @endlink
 * instances the corresponding pairs are searched. .name from the latter is 
 * assigned to the @link perf_file_attr @endlink instance.
 */
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
    
    try( readn( i_fd, traceInfo, fheader.event_types.size ) );
    
    unsigned int i, k;
    for( i = 0; i < eventAttrCount; i++ ){
        for( k = 0; k < traceInfoCount; k++ ){
            if( eventAttr[i].attr.config == traceInfo[k].event_id ){
                memcpy( &eventAttr[i].name, traceInfo[k].name, sizeof(eventAttr[i].name) );
                break;
            }
        }
        if( k == traceInfoCount ){
            char buf[256];
            snprintf( buf, sizeof(buf), "%llu", eventAttr[i].attr.config );
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

/**
 * Reads the perf file header.
 * Testing .magic for the content "PERFFILE" to ensure that we are really 
 * reading a perf file. Comparing the .attr_size with the size of the structure
 * @link perf_file_attr @endlink gives information whether the values 
 * have to be swapped. For readperf, we assume this is not the case.
 */
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
    
    trysys( lseek( i_fd, fheader.data.offset, SEEK_SET ) >= 0 );
    return true;
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

