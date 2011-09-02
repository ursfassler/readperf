#include    "perffile.h"

#include    <stdlib.h>
#include    <string.h>
#include    "session.h"
#include    "../util/origperf.h"
#include    "../util/errhandler.h"
#include    "overviewPrinter.h"

record_order_tree_t orderTree;

static struct record_t* create_mmap_msg( struct mmap_event *evt ){
    struct record_mmap* rec = (struct record_mmap*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->start = evt->start;
        rec->len = evt->len;
        rec->pgoff = evt->pgoff;
        memcpy( rec->filename, evt->filename, sizeof(rec->filename) );
    }
    return &rec->header;
};

static struct record_t* create_comm_msg( struct comm_event *evt ){
    struct record_comm* rec = (struct record_comm*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        memcpy( rec->comm, evt->comm, sizeof(rec->comm) );
    }
    return &rec->header;
};

static struct record_t* create_fork_msg( struct fork_event *evt ){
    struct record_fork* rec = (struct record_fork*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->ppid = evt->ppid;
        rec->ptid = evt->ptid;
    }
    return &rec->header;
};

static struct record_t* create_sample_msg( struct perf_sample *evt ){
    struct record_sample* rec = (struct record_sample*)malloc( sizeof(*rec) );
    if( rec != NULL ){
        rec->header.pid = evt->pid;
        rec->header.tid = evt->tid;
        rec->header.cpu = evt->cpu;
        rec->header.id = evt->id;
        rec->ip = evt->ip;
        rec->period = evt->period;
    }
    return &rec->header;
};

/**
 * After the file header is read, the records can be read. We iterate through
 * all records in the file. The ID, timestamp and more are decoded for every
 * record by the function @link perf_event__parse_sample @endlink. Specific
 * information for the different types of the record are also decoded and 
 * written to a new record. This new record is then stored, sorted by the 
 * timestamp.
 */
static bool readEvents() {
    u64 ev_nr = 0;
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        log_type( evt.header.type );
        switch( evt.header.type ){
            case PERF_RECORD_MMAP:
            case PERF_RECORD_COMM:
            case PERF_RECORD_FORK:
            case PERF_RECORD_EXIT:
            case PERF_RECORD_SAMPLE: {
                try( read_event_data( &evt ) );
                struct perf_sample  sample;
                trymsg( perf_event__parse_sample( &evt, get_sampling_type(), true, &sample ) >= 0, ERR_NOT_YET_DEFINED, "readEvents:perf_event__parse_sample" );
                
                struct record_t *rec;
                
                switch( evt.header.type ){
                    case PERF_RECORD_MMAP: rec = create_mmap_msg( &evt.mmap ); break;
                    case PERF_RECORD_COMM: rec = create_comm_msg( &evt.comm ); break;
                    case PERF_RECORD_FORK: // fall throught
                    case PERF_RECORD_EXIT: rec = create_fork_msg( &evt.fork ); break;
                    case PERF_RECORD_SAMPLE: rec = create_sample_msg( &sample ); break;
                }
                
                rec->type = (enum perf_event_type)evt.header.type;
                rec->nr = ev_nr;
                rec->time = sample.time;
                rec->cpu = sample.cpu;
                rec->id = sample.id;
                
                add_record_order( &orderTree, rec );
                break;
            }
            default:{
                try( skip_event_data( &evt.header ) );
                break;
            }
        }
        ev_nr++;
    }
    return true;
}

bool read_perf_file( int fd ){
    orderTree  = init_record_order();

    try( start_session( fd ) );

    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) != (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
        set_last_error( ERR_NOT_YET_DEFINED, "not enough information in file" );
        return false;
    }
    
    try( readEvents() );
    
    return true;
}

record_order_tree_t* get_record_order_tree(){
    return  &orderTree;
}
