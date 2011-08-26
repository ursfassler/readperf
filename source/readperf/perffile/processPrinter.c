#include    "processPrinter.h"


static FILE *summary_file = NULL;

static void printer(struct process *proc, void *stream) {
    (void)stream;
    print_process( proc );
}

static void print_process_header(){
    fprintf( summary_file, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n", "pid", "comm", "#mmap", "fork", "exit", "samples", "period" );
};

void print_process_init( FILE* fid ){
    summary_file    = fid;
    print_process_header();
}

void print_process( struct process *proc ){
    unsigned int n = 0;
    struct rmmap *itr;
    for( itr = proc->mmaps; itr != NULL; itr = itr->next ){
        n++;
    }
    fprintf( summary_file, "%i,\"%s\",%u,%llu,%llu,%llu,%llu\n", proc->pid, proc->comm, n, proc->fork_time, proc->exit_time, proc->samples, proc->period );
};

void print_processes( process_tree_t *tree ){
    iterate_processes( tree, printer, NULL );
}


