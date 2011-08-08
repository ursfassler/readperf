/*
 * File:   records.h
 * Author: urs
 *
 * Created on August 8, 2011, 9:48 AM
 */

#ifndef _RECORDS_H
#define	_RECORDS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    
#include    <stdio.h>
#include    "origperf.h"
#include    "tree.h"
    
#define RECORD_TREE_LINK  ptl
    
    struct rmmap {
        struct rmmap *next;
	u64 start;
	u64 end;
	u64 pgoff;
	char filename[PATH_MAX];
    };
    
    struct run {
        struct run *next;
        u64 fork_time;
        u64 exit_time;
        u64 first_nr;
        u64 last_nr;
        u64 samples;
        u64 period;
	char comm[16];
        struct rmmap *mmaps;
    };
    
    struct records {
        TREE_ENTRY(records)      RECORD_TREE_LINK;
        u32 pid;
        u32 tid;
        struct run  *runs;
    };
    
    typedef TREE_HEAD(_RecordTree, records) record_tree_t;
    
    struct time_order {
        TREE_ENTRY(time_order)      RECORD_TREE_LINK;
        bool isStart;
        u32 pid;
        u32 tid;
        u64 time;
        u64 nr;
    };
    
    typedef TREE_HEAD(_OrderTree, time_order) time_order_tree_t;

    time_order_tree_t init_time_order();
    void add_time_order( time_order_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr, bool isStart );
    void iterate_order( time_order_tree_t *tree, void (*callback)(struct time_order *proc, void *data), void *data );
    
    record_tree_t init_records();
    struct records* create_record( record_tree_t *tree, u32 pid, u32 tid );
    struct records* find_record( record_tree_t *tree, u32 pid, u32 tid );
    bool start_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    bool stop_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    struct run* find_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr );
    void iterate_runs( record_tree_t *tree, void (*callback)(struct run *run, struct records *rec) );
    
    void print_record_header( FILE* fid );
    void print_record( struct records *proc, FILE* fid );
    void print_records( record_tree_t *tree, FILE* fid );
    void iterate_records( record_tree_t *tree, void (*callback)(struct records *proc, void *data), void *data );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _RECORDS_H */

