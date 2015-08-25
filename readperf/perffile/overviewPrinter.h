/*
 * @file
 * Functions to log records to an file.
 *
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
 */

#ifndef _OVERVIEWPRINTER_H
#define	_OVERVIEWPRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    <linux/perf_event.h>
#include    "../util/types.h"
#include    <stdio.h>
    
    bool overviewInit( FILE* overview );
    bool overviewClose();
    
    void log_overview_s( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info );
    void log_overview_siii( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info1, u64 info2, u64 info3, u64 info4 );
    void log_overview_i( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info );
    void log_overview_ii( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info1, u64 info2 );
    void log_overview( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time );
    
    bool log_type( __u32 type );
    void print_type_summary( FILE *stat );
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* _OVERVIEWPRINTER_H */

