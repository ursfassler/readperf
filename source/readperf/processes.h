/*
 * File:   processes.h
 * Author: urs
 *
 * Created on August 5, 2011, 9:11 AM
 */

#ifndef _PROCESSES_H
#define	_PROCESSES_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    <stdio.h>
#include    "origperf.h"
#include    "tree.h"
    
    struct rmmap {
        struct rmmap *next;
	u64 start;
	u64 end;
	u64 pgoff;
	char filename[PATH_MAX];
    };
    
    struct process {
        TREE_ENTRY(process)      RECORD_TREE_LINK;
        u32 pid;
        u64 fork_time;
        u64 exit_time;
        u64 samples;
        u64 period;
	char comm[16];
        struct rmmap *mmaps;
    };
    
    typedef TREE_HEAD(_RecordTree, process) record_tree_t;
    
    record_tree_t init_records();
    struct process* create_record( record_tree_t *tree, u32 pid );
    bool remove_record( record_tree_t *tree, struct process* proc );
    
    struct process* find_record( record_tree_t *tree, u32 pid );
    struct rmmap* find_mmap( struct process* proc, u64 addr );
    
    void print_record_header( FILE* fid );
    void print_record( struct process *proc, FILE* fid );
    void print_records( record_tree_t *tree, FILE* fid );
    void iterate_records( record_tree_t *tree, void (*callback)(struct process *proc, void *data), void *data );
    

#ifdef	__cplusplus
}
#endif

#endif	/* _PROCESSES_H */

