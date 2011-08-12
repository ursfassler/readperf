/*
 * File:   session.h
 * Author: urs
 *
 * Created on July 25, 2011, 2:34 PM
 */

#ifndef _SESSION_H
#define	_SESSION_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    <linux/perf_event.h>
#include    "../util/origperf.h"
    
#define HEADER_FEAT_BITS			256
    
    struct perf_file_section {
        u64 offset;
        u64 size;
    };
    
    struct perf_file_attr {
        struct perf_event_attr	attr;
        struct perf_file_section	ids;
    };
    
    struct perf_file_header {
        u64				magic;
        u64				size;
        u64				attr_size;
        struct perf_file_section	attrs;
        struct perf_file_section	data;
        struct perf_file_section	event_types;
        u8        bits[HEADER_FEAT_BITS/8];
    };
    
    struct event_type_entry{
//        struct perf_event_attr attr;
        u64   config;
        char  name[MAX_EVENT_NAME];
        u32   count;
    };
    
    u64 get_sampling_type();
    unsigned int get_entry_count();
    struct event_type_entry* get_entry_by_index( unsigned int index );
    struct event_type_entry* get_entry( u64 id );
    bool start_session( int fd );
    bool has_more_events();
    bool next_event_header( struct perf_event_header* header );
    bool read_event_data( union perf_event *evt );
    bool skip_event_data( struct perf_event_header* header );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _SESSION_H */

