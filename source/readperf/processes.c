#include    "processes.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"

#define NR_MAGIC_HYSTERESE    5

TREE_DEFINE(pid_record, RECORD_TREE_LINK);

static int compare(struct pid_record *lhs, struct pid_record *rhs) {
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

struct pid_record* create_record( record_tree_t *tree, u32 pid, u32 tid ){
    struct pid_record *proc    = (struct pid_record *)malloc( sizeof(*proc) );
    memset( proc, 0, sizeof(*proc) );
    proc->pid = pid;
    proc->tid = tid;
    TREE_INSERT( tree, pid_record, RECORD_TREE_LINK, proc);
    return proc;
};

static void printer(struct pid_record *proc, void *stream) {
    print_record( proc, (FILE *)stream );
}

void print_record_header( FILE* fid ){
    fprintf( fid, "\"%s\",\"%s\"\n", "pid", "tid" );
};

void print_record( struct pid_record *proc, FILE* fid ){
    fprintf( fid, "%i,%i\n", proc->pid, proc->tid );
};

void print_records( record_tree_t *tree, FILE* fid ){
    TREE_FORWARD_APPLY( tree, pid_record, RECORD_TREE_LINK, printer, fid);
}

void iterate_records( record_tree_t *tree, void (*callback)(struct pid_record *proc, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, pid_record, RECORD_TREE_LINK, callback, data);
};

struct pid_record* find_record( record_tree_t *tree, u32 pid, u32 tid ){
    struct pid_record test;
    memset( &test, 0, sizeof(test) );
    test.pid = pid;
    test.tid = tid;
    struct pid_record *proc = TREE_FIND( tree, pid_record, RECORD_TREE_LINK, &test);
    return proc;
};

bool is_running( struct pid_record *proc ){
    if( proc->procs == NULL ){
        return false;
    } else {
        return proc->procs->exit_time == 0;
    }
};

static void update_nr( struct process* run, u64 nr ){
    if( nr < run->first_nr ){
        run->first_nr = nr;
    }
    if( nr > run->last_nr ){
        run->last_nr = nr;
    }
};

static bool add_process( struct pid_record *proc, u64 time, u64 nr ){
    struct process  *old    = proc->procs;
    proc->procs = (struct process*)malloc(sizeof(*proc->procs));
    trysys( proc->procs != NULL );
    memset( proc->procs, 0, sizeof(*proc->procs) );
    proc->procs->first_nr = (u64)-1;
    proc->procs->last_nr = 0;
    proc->procs->next = old;
    proc->procs->fork_time = time;
    update_nr( proc->procs, nr );
    return true;
};

bool start_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct pid_record *proc    = find_record( tree, pid, tid );
    if( proc == NULL ){
        proc = create_record( tree, pid, tid );
        trysys( proc != NULL );
    }
    if( !is_running(proc) ){
        return add_process( proc, time, nr );
    } else {
        if( proc->procs != NULL ){
            update_nr( proc->procs, nr );
        }
    }
    return true;
};

static bool finish_process( struct pid_record *proc, u64 time, u64 nr ){
    trymsg( proc->procs != NULL, ERR_ENTRY_NOT_FOUND, "finish_run" );
    proc->procs->exit_time = time;
    update_nr( proc->procs, nr );
    return true;
};

bool stop_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct pid_record *proc    = find_record( tree, pid, tid );
    trymsg( proc != 0, ERR_ENTRY_NOT_FOUND, "stop_run" );
    finish_process( proc, time, nr );
    return true;
};

struct process* find_process( record_tree_t *tree, u32 pid, u32 tid, u64 time, u64 nr ){
    struct pid_record *proc    = find_record( tree, pid, tid );
    trymsg( proc != 0, ERR_ENTRY_NOT_FOUND, "find_run:1" );
    
    struct process* itr;
    
    for( itr = proc->procs; itr != NULL; itr = itr->next ){
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

static void process_callback( struct pid_record *rec, void *data ){
    void (*callback)(struct process *run, struct pid_record *rec) = (void (*)(struct process *run, struct pid_record *rec))data;
    struct process *itr;
    for( itr = rec->procs; itr != NULL; itr = itr->next ){
        callback( itr, rec );
    }
};

void iterate_process( record_tree_t *tree, void (*callback)(struct process *run, struct pid_record *rec) ){
    TREE_FORWARD_APPLY( tree, pid_record, RECORD_TREE_LINK, process_callback, (void*)callback);
};
