#include    "records.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"
#include    "session.h"

TREE_DEFINE(record_t, RECORD_TREE_LINK);

    struct record_t* create_order_record( size_t size );

/*bool add_event( record_order_tree_t *tree, u64 ev_nr, const union perf_event *evt ){
    struct perf_sample  sample;
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        set_last_error( ERR_NOT_YET_DEFINED, "add_event:perf_event__parse_sample" );
        return false;
    }
    
    struct record_t *rec = (struct record_t *)malloc( sizeof(*rec) );
    trysys( rec != NULL );
    memset( rec, 0, sizeof(*rec) );
    rec->nr = ev_nr;
    rec->time = sample.time;
    rec->event = *evt;
    return add_record_order( tree, rec );
};*/

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




