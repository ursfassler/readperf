#include    "processes.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"

TREE_DEFINE(process, PROC_TREE_LINK);

static int compare(struct process *lhs, struct process *rhs) {
    s64 res = (s64)lhs->pid - (s64)rhs->pid;
    if( res == 0 ){
        res = lhs->tid - rhs->tid;
    }
    return res;
}

proc_tree_t init_processes(){
    proc_tree_t res = TREE_INITIALIZER(compare);
    return  res;
}

struct process *find_process( proc_tree_t *tree, u32 pid, u32 tid ){
    struct process test;
    memset( &test, 0, sizeof(test) );
    test.pid = pid;
    test.tid = tid;
    struct process *proc = TREE_FIND( tree, process, PROC_TREE_LINK, &test);
    return proc;
};

/*struct process *force_process( proc_tree_t *tree, u32 pid, u32 tid ){
    struct process *proc;
    
    proc = find_process( tree, pid, tid );
    if( proc == NULL ){
        proc = create_process( tree, pid, tid );
    }
    return proc;
};*/

struct process *create_process( proc_tree_t *tree, u32 pid, u32 tid ){
    struct process *proc    = (struct process *)malloc( sizeof(*proc) );
    memset( proc, 0, sizeof(*proc) );
    proc->pid = pid;
    proc->tid = tid;
    TREE_INSERT( tree, process, PROC_TREE_LINK, proc);
    return proc;
};

bool add_process( proc_tree_t *tree, struct process *proc ){
    trymsg( find_process( tree, proc->pid, proc->tid ) == NULL, ERR_PROC_ALREADY_EXISTS, NULL );
    TREE_INSERT( tree, process, PROC_TREE_LINK, proc);
    return true;
};

void remove_process( proc_tree_t *tree, struct process *proc ){
    TREE_REMOVE( tree, process, PROC_TREE_LINK, proc );
};

static void printer(struct process *proc, void *stream) {
    print_process( proc, (FILE *)stream );
}

void print_process_header( FILE* fid ){
    fprintf( fid, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", "pid", "tid", "fork time", "exit time", "samples", "period", "comm" );
};

void print_process( struct process *proc, FILE* fid ){
    fprintf( fid, "%i,%i,%llu,%llu,%llu,%llu,\"%s\"\n", proc->pid, proc->tid, proc->fork_time, proc->exit_time, proc->samples, proc->period, proc->comm );
};

void print_processes( proc_tree_t *tree, FILE* fid ){
    TREE_FORWARD_APPLY( tree, process, PROC_TREE_LINK, printer, fid);
}

