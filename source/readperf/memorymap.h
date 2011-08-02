/*
 * @file
 * Contains functions to map instruction pointers to library / application
 * names.
 * ATTENTION: the algorithms and datastructures are highly innefficient. The
 * code is only a proof of concept!
 */

#ifndef _MEMORYMAP_H
#define	_MEMORYMAP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    
#include    <stdio.h>
#include    "types.h"
#include    "origperf.h"
    
    struct mmap_entry {
        struct mmap_entry   *next;
        u32 pid, tid;
        u64 start;
        u64 end;
        u64 period;
        u64 samples;
        char filename[PATH_MAX];
    };
    
    struct mmap_entry* mm_get_entry( u32 pid, u32 tid, u64 addr );
    struct mmap_entry* mm_add_entry();
    struct mmap_entry* mm_get_error();
    void mm_print( FILE *fid, bool include_zero_entries );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _MEMORYMAP_H */

