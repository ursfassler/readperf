
#include    "overviewPrinter.h"
#include    <string.h>
#include    "../util/errhandler.h"

#define SEP ","

static FILE *overview_file = NULL;
static const char *NAMES[] = { NULL, "MMAP", "LOST", "COMM", "EXIT", "THROTTLE", "UNTHROTTLE", "FORK", "READ", "SAMPLE" };
static int count[PERF_RECORD_MAX] = { 0 };

static void log_overview_header(){
    if( overview_file != NULL ){
        fprintf( overview_file, "nr%stype%spid%stid%stime%sinfo\n", SEP, SEP, SEP, SEP, SEP );
    }
}

static void log_overview_head( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    if( overview_file != NULL ){
        fprintf( overview_file, "%llu%s%s%s%u%s%u%s%llu%s", ev_nr, SEP, NAMES[type], SEP, pid, SEP, tid, SEP, time, SEP );
    }
}

void log_overview_s( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info ){
    if( overview_file != NULL ){
        log_overview_head( ev_nr, type, pid, tid, time );
        fprintf( overview_file, "%s\n", info );
    }
}

void log_overview_siii( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, char* info1, u64 info2, u64 info3, u64 info4 ){
    if( overview_file != NULL ){
        log_overview_head( ev_nr, type, pid, tid, time );
        fprintf( overview_file, "%s%s", info1, SEP );
        fprintf( overview_file, "%llu%s", info2, SEP );
        fprintf( overview_file, "%llu%s", info3, SEP );
        fprintf( overview_file, "%llu\n", info4 );
    }
}

void log_overview_i( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info ){
    if( overview_file != NULL ){
        log_overview_head( ev_nr, type, pid, tid, time );
        fprintf( overview_file, "%llu\n", info );
    }
}

void log_overview_ii( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time, u64 info1, u64 info2 ){
    if( overview_file != NULL ){
        log_overview_head( ev_nr, type, pid, tid, time );
        fprintf( overview_file, "%llu%s", info1, SEP );
        fprintf( overview_file, "%llu\n", info2 );
    }
}

void log_overview( u64 ev_nr, enum perf_event_type type, u32 pid, u32 tid, u64 time ){
    if( overview_file != NULL ){
        log_overview_head( ev_nr, type, pid, tid, time );
        fprintf( overview_file, "\n" );
    }
}

bool overviewInit( FILE* overview ){
    overview_file   = overview;
    log_overview_header();
    return  true;
}

bool log_type( __u32 type ){
    if( (type > 0) && (type < PERF_RECORD_MAX) ){
        count[type]++;
    } else {
        count[0]++;
    }
    return true;
}

void print_type_summary( FILE *stat ){
    fprintf( stat, "record%scount\n", SEP );
    unsigned int i;
    for( i = 1; i < PERF_RECORD_MAX; i++ ){
        fprintf( stat, "%s%s%i\n", NAMES[i], SEP, count[i] );
    }
    fprintf( stat, "unknown%s%i\n", SEP, count[0] );
}
