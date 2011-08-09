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
        struct process *next;
        u64 fork_time;
        u64 exit_time;
        u64 first_nr;
        u64 last_nr;
        u64 samples;
        u64 period;
	char comm[16];
        struct rmmap *mmaps;
    };
    
    struct pid_record {
        TREE_ENTRY(pid_record)      RECORD_TREE_LINK;
        u32 pid;
        u32 tid;
        struct process  *procs;
    };
    
    typedef TREE_HEAD(_RecordTree, pid_record) record_tree_t;
    
    bool start_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    bool stop_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    struct process* find_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    void iterate_process( record_tree_t *tree, void (*callback)(struct process *run, struct pid_record *rec) );
    
    record_tree_t init_records();
    struct pid_record* create_record( record_tree_t *tree, u32 pid, u32 tid );
    struct pid_record* find_record( record_tree_t *tree, u32 pid, u32 tid );
    void print_record_header( FILE* fid );
    void print_record( struct pid_record *proc, FILE* fid );
    void print_records( record_tree_t *tree, FILE* fid );
    void iterate_records( record_tree_t *tree, void (*callback)(struct pid_record *proc, void *data), void *data );
    

#ifdef	__cplusplus
}
#endif

#endif	/* _PROCESSES_H */

