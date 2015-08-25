/**
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
 */
#include    "records.h"
#include    <stdlib.h>
#include    <string.h>
#include    "../util/errhandler.h"
#include    "session.h"

TREE_DEFINE(record_t, RECORD_TREE_LINK);

bool add_record_order( record_order_tree_t *tree, struct record_t *rec ){
    TREE_INSERT( tree, record_t, RECORD_TREE_LINK, rec);
    return true;
};

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




