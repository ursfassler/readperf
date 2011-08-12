#include    "processPrinter.h"

static void printer(struct process *proc, void *stream) {
    print_process( proc, (FILE *)stream );
}

void print_process_header( FILE* fid ){
    fprintf( fid, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", "pid", "comm", "#mmap", "fork", "exit", "samples", "period" );
};

void print_process( struct process *proc, FILE* fid ){
    unsigned int n = 0;
    struct rmmap *itr;
    for( itr = proc->mmaps; itr != NULL; itr = itr->next ){
        n++;
    }
    fprintf( fid, "%i,\"%s\",%u,%llu,%llu,%llu,%llu\n", proc->pid, proc->comm, n, proc->fork_time, proc->exit_time, proc->samples, proc->period );
};

void print_processes( process_tree_t *tree, FILE* fid ){
    iterate_processes( tree, printer, fid );
}

