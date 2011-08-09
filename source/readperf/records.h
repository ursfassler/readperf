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
    
    typedef enum {
        OE_START,
        OE_END
    } order_entry_t;
    
    struct record_t {
        TREE_ENTRY(record_t)      RECORD_TREE_LINK;
        order_entry_t type;
        u32 pid;
        u32 tid;
        u64 time;
        u64 nr;
    };
    
    typedef TREE_HEAD(_OrderTree, record_t) record_order_tree_t;
    
    record_order_tree_t init_record_order();
    void add_record_order( record_order_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr, order_entry_t type );
    void iterate_order( record_order_tree_t *tree, void (*callback)(struct record_t *proc, void *data), void *data );
    
#ifdef	__cplusplus
}
#endif

#endif	/* _RECORDS_H */

