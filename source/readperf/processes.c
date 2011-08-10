#include    "processes.h"
#include    <stdlib.h>
#include    <string.h>
#include    "errhandler.h"

#define NR_MAGIC_HYSTERESE    5

TREE_DEFINE(process, RECORD_TREE_LINK);

static int compare(struct process *lhs, struct process *rhs) {
    if( lhs->pid > rhs->pid ){
        return  1;
    } else if( lhs->pid < rhs->pid ){
        return  -1;
    } else {
/*        if( lhs->tid > rhs->tid ){
            return  1;
        } else if( lhs->tid < rhs->tid ){
            return  -1;
        } else {*/
            return 0;
//        }
    }
}

record_tree_t init_records(){
    record_tree_t res = TREE_INITIALIZER(compare);
    return  res;
}

struct process* create_record( record_tree_t *tree, u32 pid ){
    struct process *proc    = (struct process *)malloc( sizeof(*proc) );
    if( proc != NULL ){
        memset( proc, 0, sizeof(*proc) );
        proc->exit_time = -1;
        proc->pid = pid;
        TREE_INSERT( tree, process, RECORD_TREE_LINK, proc);
    }
    return proc;
};

bool remove_record( record_tree_t *tree, struct process* proc ){
    TREE_REMOVE( tree, process, RECORD_TREE_LINK, proc);
    return true;
};


static void printer(struct process *proc, void *stream) {
    print_record( proc, (FILE *)stream );
}

void print_record_header( FILE* fid ){
    fprintf( fid, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", "pid", "comm", "#mmap", "fork", "exit", "samples", "period" );
};

void print_record( struct process *proc, FILE* fid ){
    unsigned int n = 0;
    struct rmmap *itr;
    for( itr = proc->mmaps; itr != NULL; itr = itr->next ){
        n++;
    }
    fprintf( fid, "%i,\"%s\",%u,%llu,%llu,%llu,%llu\n", proc->pid, proc->comm, n, proc->fork_time, proc->exit_time, proc->samples, proc->period );
};

void print_records( record_tree_t *tree, FILE* fid ){
    TREE_FORWARD_APPLY( tree, process, RECORD_TREE_LINK, printer, fid);
}

void iterate_records( record_tree_t *tree, void (*callback)(struct process *proc, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, process, RECORD_TREE_LINK, callback, data);
};

struct process* find_record( record_tree_t *tree, u32 pid ){
    struct process test;
    memset( &test, 0, sizeof(test) );
    test.pid = pid;
    struct process *proc = TREE_FIND( tree, process, RECORD_TREE_LINK, &test);
    return proc;
};

void iterate_process( record_tree_t *tree, void (*callback)(struct process *run, void *data), void *data ){
    TREE_FORWARD_APPLY( tree, process, RECORD_TREE_LINK, callback, data );
};
