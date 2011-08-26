/**
 * @file
 * Stores processes and related information like memory maps.
 */

#ifndef _PROCESSES_H
#define	_PROCESSES_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include    "../util/origperf.h"
#include    "../util/tree.h"
    
    #define COMMON_PROC     ((u32)-1)
    
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
    
    typedef TREE_HEAD(_RecordTree, process) process_tree_t;
    
    process_tree_t init_processes();
    struct process* create_process( process_tree_t *tree, u32 pid );
    bool remove_process( process_tree_t *tree, struct process* proc );
    
    struct process* find_process( process_tree_t *tree, u32 pid );
    struct rmmap* find_mmap( struct process* proc, u64 addr );
    
    void iterate_processes( process_tree_t *tree, void (*callback)(struct process *proc, void *data), void *data );
    

#ifdef	__cplusplus
}
#endif

#endif	/* _PROCESSES_H */

