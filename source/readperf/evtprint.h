/*
 * File:   evtprint.h
 * Author: urs
 *
 * Created on July 28, 2011, 10:05 AM
 */

#ifndef _EVTPRINT_H
#define	_EVTPRINT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    <stdio.h>
#include    "types.h"
    
    void printSampleFormat( __u64 type );
    void printSample( FILE* fid, u64 ev_nr, struct perf_sample *sample, u64 type, const char *sep );
    void printSampleHeader( FILE* fid, u64 type, const char *sep );
    void printMmap( FILE* fid, u64 ev_nr, struct mmap_event *evt, const char *sep );
    void printMmapHeader( FILE* fid, const char *sep );
    void printComm( FILE* fid, u64 ev_nr, struct comm_event *evt, const char *sep );
    void printCommHeader( FILE* fid, const char *sep );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _EVTPRINT_H */

