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
    
    struct proc_mmap {
        struct proc_mmap    *next;
        u64 first;  // memory address
        u64 last;   // memory address
        u64 pgoff;
        char filename[PATH_MAX];
    };
    
#define PROC_TREE_LINK  ptl
    
    struct process {
        TREE_ENTRY(process)      PROC_TREE_LINK;
        u32 pid;
        u32 tid;
        u64 fork_time;
        u64 exit_time;
        u64 samples;
        u64 period;
        u64 last_access;
        char comm[16];
        struct proc_mmap    *mmap;
    };
    
    typedef TREE_HEAD(_ProcTree, process) proc_tree_t;
    
    proc_tree_t init_processes();
    struct process *find_process( proc_tree_t *tree, u32 pid, u32 tid );
//    struct process *force_process( proc_tree_t *tree, u32 pid, u32 tid );
    struct process *create_process( proc_tree_t *tree, u32 pid, u32 tid, u64 last_access );
//    bool add_process( proc_tree_t *tree, struct process *proc );
    void remove_process( proc_tree_t *tree, struct process *proc );
    void print_process_header( FILE* fid );
    void print_process( struct process *proc, FILE* fid );
    void print_processes( proc_tree_t *tree, FILE* fid );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _PROCESSES_H */

