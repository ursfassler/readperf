#include  <stdio.h>
#include  <linux/types.h>
#include  <linux/perf_event.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <string.h>
#include    <stdlib.h>
#include  "types.h"
#include  "origperf.h"
#include  "evtypes.h"
#include  "session.h"
#include    "errhandler.h"
#include    "evtprint.h"
#include    "memorymap.h"
#include    "funcstat.h"
#include    "processes.h"

static u32 fail_counter = 0;

#define     MAGIC_ACCESS_DISTANCE   100

static bool produce_statistic = false;

#define SEP ","
static int count[PERF_RECORD_MAX];

#define SAMPLE_LOG_NAME "sample.csv"
static FILE *sample_fid = NULL;
#define MMAP_LOG_NAME   "mmap.csv"
static FILE *mmap_file = NULL;
#define COMM_LOG_NAME   "comm.csv"
static FILE *comm_file = NULL;
#define FORK_LOG_NAME   "fork.csv"
static FILE *fork_file = NULL;
#define EXIT_LOG_NAME   "exit.csv"
static FILE *exit_file = NULL;
#define OVERVIEW_LOG_NAME   "overview.csv"
static FILE *overview_file = NULL;

#define SUMMARY_LOG_NAME   "summary.csv"
static FILE *summary_file = NULL;


#define STAT_LOG_NAME   "stat.csv"
#define FUNC_LOG_NAME   "func.csv"

proc_tree_t activeTree;

static const char *NAMES[] = { NULL, "MMAP", "LOST", "COMM", "EXIT", "THROTTLE", "UNTHROTTLE", "FORK", "READ", "SAMPLE" };

static void log_overview_head( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    fprintf( overview_file, "%llu%s%s%s%u%s%u%s%llu%s", ev_nr, SEP, NAMES[type], SEP, pid, SEP, tid, SEP, time, SEP );
}

static void log_overview_s( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "%s\n", info );
}

static void log_overview_i( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "%llu\n", info );
}

static void log_overview( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    log_overview_head( ev_nr, type, pid, tid, time );
    fprintf( overview_file, "\n" );
}

static bool decodeSample( u64 ev_nr, union perf_event *evt ) {
    struct perf_sample  sample;
    memset( &sample, 0, sizeof(sample) );
    if( perf_event__parse_sample( evt, get_sampling_type(), true, &sample ) < 0 ){
        set_last_error( ERR_DECODE_SAMPLE, NULL );
        return false;
    }
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_TIME)) == (PERF_SAMPLE_TID | PERF_SAMPLE_TIME) ){
        struct process *proc = find_process( &activeTree, sample.pid, sample.tid );
        if( proc == NULL ){
            fail_counter++;
            /*            proc = create_process(&activeTree, sample.pid, sample.tid, ev_nr);
             * trysys( proc != NULL );*/
        } else {
            if( proc->fork_time > sample.time ){
                fail_counter++;
            }
            if( (proc->exit_time != 0) && (proc->exit_time < sample.time) ){  // now we have a sample outside a fork - exit :-(
                fail_counter++;
                /*                print_process( proc, summary_file );
                 * remove_process( &activeTree, proc );
                 * free( proc );
                 * proc = create_process(&activeTree, sample.pid, sample.tid, ev_nr);
                 * trysys( proc != NULL );*/
            }
        }
        if( proc != NULL ){
            proc->samples++;
            proc->period += sample.period;
        }
    }
    
    
    struct event_type_entry* entry = get_entry( sample.id );
    try( entry != NULL );
    entry->count++;
    
    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
        struct mmap_entry *entry = mm_get_entry(sample.pid, sample.tid, sample.ip);
        entry->samples++;
        entry->period += sample.period;
        
        if( produce_statistic ){
            struct func_dir* func;
            
            if( entry == mm_get_error() ){
                func = func_error();
            } else {
                func = force_entry( sample.ip, entry->filename );
                if( func == NULL ){
                    func = func_error();
                }
            }
            
            try( func != NULL );
            
            func->period += sample.period;
            func->samples++;
        }
    }
    /*    if( (get_sampling_type() & (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP)) == (PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_IP) ){
     * struct log_entry *entry = forceLogEntry( sample.pid, sample.tid, sample.ip, NULL, NULL );
     * entry->period += sample.period;
     * entry->samples++;
     * }*/
    printSample( sample_fid, ev_nr, &sample, get_sampling_type(), SEP );
    log_overview_i( ev_nr, PERF_RECORD_SAMPLE, sample.pid, sample.tid, sample.time, sample.period );
    
    return true;
}

static bool decodeMmap( u64 ev_nr, struct mmap_event *evt ) {
    struct process *proc = find_process( &activeTree, evt->pid, evt->tid );
    if( proc == NULL ){
        proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr);
        trysys( proc != NULL );
    } else {
        if( proc->exit_time != 0 ){     // it is an old process
            print_process( proc, summary_file );
            remove_process( &activeTree, proc );
            free( proc );
            proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr);
            trysys( proc != NULL );
        }
    }
    
    if( produce_statistic ){
        struct mmap_entry *entry = mm_add_entry();
        if( entry == NULL ){
            return false;
        }
        entry->pid = evt->pid;
        entry->tid = evt->tid;
        entry->start = evt->start;
        entry->end   = evt->start+evt->len-1;
        memcpy( entry->filename, evt->filename, sizeof(entry->filename) );
    }
    
    /*    struct log_entry *entry = forceLogEntry( evt->mmap.pid, evt->mmap.tid, 0, NULL, evt->mmap.filename );
     * if( entry->filename == NULL ){
     * entry->filename = strndup( evt->mmap.filename, sizeof( evt->mmap.filename) );
     * }*/
    printMmap( mmap_file, ev_nr, evt, SEP );
    log_overview_s( ev_nr, PERF_RECORD_MMAP, evt->pid, evt->tid, 0, evt->filename );
    return true;
}

static bool decodeComm( u64 ev_nr, struct comm_event *evt ) {
    struct process *proc = find_process( &activeTree, evt->pid, evt->tid );
    if( proc == NULL ){
        proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr);
        trysys( proc != NULL );
    } else {
        if( proc->exit_time != 0 ){     // it is an old process
            print_process( proc, summary_file );
            remove_process( &activeTree, proc );
            free( proc );
            proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr );
            trysys( proc != NULL );
        }
    }
    memcpy( proc->comm, evt->comm, sizeof(proc->comm) );
    
    printComm( comm_file, ev_nr, evt, SEP );
    log_overview_s( ev_nr, PERF_RECORD_COMM, evt->pid, evt->tid, 0, evt->comm );
    return true;
}

static bool decodeFork( u64 ev_nr, struct fork_event *evt ) {
    struct process *proc = find_process( &activeTree, evt->pid, evt->tid );
    if( proc == NULL ){
        proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr);
        trysys( proc != NULL );
    } else {
        if( proc->fork_time != 0 ){
            if( proc->exit_time != 0 ){     // it is an old process
                print_process( proc, summary_file );
                remove_process( &activeTree, proc );
                free( proc );
                proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr);
                trysys( proc != NULL );
            } else {
                char buffer[1024];
                snprintf( buffer, sizeof(buffer), "%llu %i:%i", ev_nr, evt->pid, evt->tid );
                set_last_error( ERR_PROC_ALREADY_EXISTS, strdup(buffer) );
                return true;
            }
        }
    }
    proc->fork_time = evt->time;
    
    printFork( fork_file, ev_nr, evt, SEP );
    log_overview_i( ev_nr, PERF_RECORD_FORK, evt->pid, evt->tid, evt->time, evt->ppid );
    return true;
}

static bool decodeExit( u64 ev_nr, struct fork_event *evt ) {
    //EXIT comes twice with different time
    static u64 last_exit_nr = 0;
    static bool second_exit = false;
    if( !second_exit ){
        last_exit_nr = ev_nr;
        second_exit = true;
    } else {
        if( ev_nr-last_exit_nr > 42 ){
            char buffer[1024];
            snprintf( buffer, sizeof(buffer), "%llu | %llu", last_exit_nr, ev_nr );
            set_last_error( ERR_EXIT_HACK_DOES_NOT_WORK, strdup(buffer) );
            return false;
        }
        second_exit = false;
        
        struct process *proc = find_process(&activeTree, evt->pid, evt->tid );
        if( proc == NULL ){
            /*            proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr );
             * try( proc != NULL );*/
        } else {
            proc->exit_time = evt->time;
            print_process( proc, summary_file );
            remove_process( &activeTree, proc );
            free( proc );
            /*            if( proc->exit_time != 0 ){
             * print_process( proc, summary_file );
             * remove_process( &activeTree, proc );
             * free( proc );
             * proc = create_process(&activeTree, evt->pid, evt->tid, ev_nr );
             * trysys( proc != NULL );
             * }*/
        }
//        proc->exit_time = evt->time;
    }
    
    printExit( exit_file, ev_nr, evt, SEP );
    log_overview( ev_nr, PERF_RECORD_EXIT, evt->pid, evt->tid, evt->time );
    return true;
}

bool readEvents() {
    u64 ev_nr = 0;
    printSampleHeader( sample_fid, get_sampling_type(), SEP );
    while( has_more_events() ){
        union perf_event evt;
        try( next_event_header( &evt.header ) );
        if( (evt.header.type > 0) && (evt.header.type < PERF_RECORD_MAX) ){
            count[evt.header.type]++;
        } else {
            count[0]++;
        }
        switch( evt.header.type ){
            case PERF_RECORD_COMM:{
                try( read_event_data( &evt ) );
                try( decodeComm( ev_nr, &evt.comm ) );
                break;
            }
            case PERF_RECORD_MMAP:{
                try( read_event_data( &evt ) );
                try( decodeMmap( ev_nr, &evt.mmap ) );
                break;
            }
            case PERF_RECORD_FORK:{
                try( read_event_data( &evt ) );
                try( decodeFork( ev_nr, &evt.fork ) );
                break;
            }
            case PERF_RECORD_EXIT:{
                try( read_event_data( &evt ) );
                try( decodeExit( ev_nr, &evt.fork ) );
                break;
            }
            case PERF_RECORD_SAMPLE:{
                try( read_event_data( &evt ) );
                try( decodeSample( ev_nr, &evt ) );
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

int main( int argc, char **argv ) {
    if( argc != 2 ){
        printf( "expect a file name\n" );
        return -1;
    }
    
    int fd = open( argv[1], O_RDONLY );
    if( fd < 0 ){
        perror( "open data file\n" );
        return -1;
    }
    
    activeTree = init_processes();
    
    memset( count, 0, sizeof(count) );
    
    if( !start_session( fd ) ){
        print_last_error();
        return -1;
    }
    
    printf( "Sampling types: " );
    printSampleFormat( get_sampling_type() );
    printf( "\n" );
    
    overview_file  = fopen( OVERVIEW_LOG_NAME, "w+" );
    fprintf( overview_file, "nr%stype%spid%stid%stime%sinfo\n", SEP, SEP, SEP, SEP, SEP );
    
    mmap_file  = fopen( MMAP_LOG_NAME, "w+" );
    comm_file  = fopen( COMM_LOG_NAME, "w+" );
    fork_file  = fopen( FORK_LOG_NAME, "w+" );
    exit_file  = fopen( EXIT_LOG_NAME, "w+" );
    sample_fid  = fopen( SAMPLE_LOG_NAME, "w+" );
    summary_file  = fopen( SUMMARY_LOG_NAME, "w+" );
    print_process_header( summary_file );
    
    if( !readEvents() ){
        print_last_error();
        return -1;
    }
    
    print_processes( &activeTree, summary_file );
    
    fclose( summary_file );
    fclose( sample_fid );
    fclose( exit_file );
    fclose( fork_file );
    fclose( comm_file );
    fclose( mmap_file );
    fclose( overview_file );
    
    printf( "found events\n" );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        printf( "  %s: %i\n", NAMES[i], count[i] );
    }
    printf( "  unknown: %i\n", count[0] );
    printf( "\n" );
    
    if( produce_statistic ){
        printf( "Detailed:\n" );
        for( i = 0; i < get_entry_count(); i++ ){
            struct event_type_entry* entry = get_entry_by_index( i );
            printf( "  %s: %i\n", entry->name, entry->count );
        }
        
        FILE *stat_fid  = fopen( STAT_LOG_NAME, "w+" );
        mm_print( stat_fid, false );
        fclose( stat_fid );
        
        FILE *func_fid = fopen( FUNC_LOG_NAME, "w+" );
        func_print( func_fid );
        fclose( func_fid );
        
        fprintf( stdout, "\nStatistics per function:\n" );
        func_stat_print( stdout );
    }
    
    printf( "fail_counter: %u\n", fail_counter );
    
    /*    fprintf( stdout, "-- activeTree tree --\n" );
     * print_processes( &activeTree, stdout );
     * fprintf( stdout, "-- dead tree --\n" );
     * print_processes( &deadTree, stdout );*/
    
    return 0;
}
