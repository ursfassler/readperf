#include    "records.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"

TREE_DEFINE(records, RECORD_TREE_LINK);
TREE_DEFINE(time_order, RECORD_TREE_LINK);

void add_time_order( time_order_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr, bool isStart ){
    struct time_order *proc    = (struct time_order *)malloc( sizeof(*proc) );
    memset( proc, 0, sizeof(*proc) );
    proc->pid = pid;
    proc->tid = tid;
    proc->time = time;
    proc->isStart = isStart;
    proc->nr = nr;
    TREE_INSERT( tree, time_order, RECORD_TREE_LINK, proc);
}

static int compare_order(struct time_order *lhs, struct time_order *rhs) {
    if( lhs->time > rhs->time ){
        return  1;
    } else if( lhs->time < rhs->time ){
        return  -1;
    } else {
        return 0;
    }
};

time_order_tree_t init_time_order(){
    time_order_tree_t res = TREE_INITIALIZER(compare_order);
    return  res;
}

void iterate_order( time_order_tree_t *tree, void (*callback)(struct time_order *proc, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, time_order, RECORD_TREE_LINK, callback, data);
};



static int compare(struct records *lhs, struct records *rhs) {
    if( lhs->pid > rhs->pid ){
        return  1;
    } else if( lhs->pid < rhs->pid ){
        return  -1;
    } else {
        if( lhs->tid > rhs->tid ){
            return  1;
        } else if( lhs->tid < rhs->tid ){
            return  -1;
        } else {
            return 0;
        }
    }
}

record_tree_t init_records(){
    record_tree_t res = TREE_INITIALIZER(compare);
    return  res;
}

struct records* create_record( record_tree_t *tree, u32 pid, u32 tid ){
    struct records *proc    = (struct records *)malloc( sizeof(*proc) );
    memset( proc, 0, sizeof(*proc) );
    proc->pid = pid;
    proc->tid = tid;
    TREE_INSERT( tree, records, RECORD_TREE_LINK, proc);
    return proc;
};

static void printer(struct records *proc, void *stream) {
    print_record( proc, (FILE *)stream );
}

void print_record_header( FILE* fid ){
    fprintf( fid, "\"%s\",\"%s\"\n", "pid", "tid" );
};

void print_record( struct records *proc, FILE* fid ){
    fprintf( fid, "%i,%i\n", proc->pid, proc->tid );
};

void print_records( record_tree_t *tree, FILE* fid ){
    TREE_FORWARD_APPLY( tree, records, RECORD_TREE_LINK, printer, fid);
}

void iterate_records( record_tree_t *tree, void (*callback)(struct records *proc, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, records, RECORD_TREE_LINK, callback, data);
};

struct records* find_record( record_tree_t *tree, u32 pid, u32 tid ){
    struct records test;
    memset( &test, 0, sizeof(test) );
    test.pid = pid;
    test.tid = tid;
    struct records *proc = TREE_FIND( tree, records, RECORD_TREE_LINK, &test);
    return proc;
};

bool is_running( struct records *proc ){
    if( proc->runs == NULL ){
        return false;
    } else {
        return proc->runs->exit_time == 0;
    }
};

static void update_nr( struct run* run, u64 nr ){
    if( nr < run->first_nr ){
        run->first_nr = nr;
    }
    if( nr > run->last_nr ){
        run->last_nr = nr;
    }
};

static bool add_run( struct records *proc, u64 time, u64 nr ){
    struct run  *old    = proc->runs;
    proc->runs = (struct run*)malloc(sizeof(*proc->runs));
    trysys( proc->runs != NULL );
    memset( proc->runs, 0, sizeof(*proc->runs) );
    proc->runs->first_nr = (u64)-1;
    proc->runs->last_nr = 0;
    proc->runs->next = old;
    proc->runs->fork_time = time;
    update_nr( proc->runs, nr );
    return true;
};

bool start_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct records *proc    = find_record( tree, pid, tid );
    if( proc == NULL ){
        proc = create_record( tree, pid, tid );
        trysys( proc != NULL );
    }
    if( !is_running(proc) ){
        return add_run( proc, time, nr );
    } else {
        if( proc->runs != NULL ){
            update_nr( proc->runs, nr );
        }
    }
    return true;
};

static bool finish_run( struct records *proc, u64 time, u64 nr ){
    trymsg( proc->runs != NULL, ERR_ENTRY_NOT_FOUND, "finish_run" );
    proc->runs->exit_time = time;
    update_nr( proc->runs, nr );
    return true;
};

bool stop_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct records *proc    = find_record( tree, pid, tid );
    trymsg( proc != 0, ERR_ENTRY_NOT_FOUND, "stop_run" );
    finish_run( proc, time, nr );
    return true;
};

#define NR_MAGIC_HYSTERESE    5

struct run* find_run( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct records *proc    = find_record( tree, pid, tid );
    trymsg( proc != 0, ERR_ENTRY_NOT_FOUND, "find_run:1" );
    
    struct run* itr;
    
    for( itr = proc->runs; itr != NULL; itr = itr->next ){
        if( time != (u64)-1 ){
            if( ((itr->fork_time == (u64)-1) || (itr->fork_time <= time)) && ((itr->exit_time == 0) || (itr->exit_time >= time)) ){
                return itr;
            }
        }
        if( (nr < itr->last_nr + NR_MAGIC_HYSTERESE) && (nr + NR_MAGIC_HYSTERESE > itr->first_nr) ){
            return itr;
        }
    }
    
    set_last_error( ERR_ENTRY_NOT_FOUND, "find_run:2" );
    return NULL;
};

static void run_callback( struct records *rec, void *data ){
    void (*callback)(struct run *run, struct records *rec) = (void (*)(struct run *run, struct records *rec))data;
    struct run *itr;
    for( itr = rec->runs; itr != NULL; itr = itr->next ){
        callback( itr, rec );
    }
};

void iterate_runs( record_tree_t *tree, void (*callback)(struct run *run, struct records *rec) ){
    TREE_FORWARD_APPLY( tree, records, RECORD_TREE_LINK, run_callback, (void*)callback);
};
