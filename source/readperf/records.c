#include    "records.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"

TREE_DEFINE(record_t, RECORD_TREE_LINK);

void add_record_order( record_order_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr, order_entry_t type ){
    struct record_t *proc    = (struct record_t *)malloc( sizeof(*proc) );
    memset( proc, 0, sizeof(*proc) );
    proc->pid = pid;
    proc->tid = tid;
    proc->time = time;
    proc->type = type;
    proc->nr = nr;
    TREE_INSERT( tree, record_t, RECORD_TREE_LINK, proc);
}

static int compare(struct record_t *lhs, struct record_t *rhs) {
    if( lhs->time > rhs->time ){
        return  1;
    } else if( lhs->time < rhs->time ){
        return  -1;
    } else {
        return 0;
    }
};

record_order_tree_t init_record_order(){
    record_order_tree_t res = TREE_INITIALIZER(compare);
    return  res;
}

void iterate_order( record_order_tree_t *tree, void (*callback)(struct record_t *proc, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, record_t, RECORD_TREE_LINK, callback, data);
};




