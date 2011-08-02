#include    "evtprint.h"
#include    "origperf.h"

void printSampleFormat( __u64 type ) {
    static const char *names[] = { "IP", "TID", "TIME", "ADDR", "READ", "CALLCHAIN", "ID", "CPU", "PERIOD", "STREAM_ID", "RAW" };
    unsigned int i;
    
    for( i = 0; i < sizeof(names)/sizeof(names[0]); i++ ){
        if( (type & 1) == 1 ){
            printf( " %s", names[i] );
        }
        type = type >> 1;
    }
}

void printSample( FILE* fid, u64 ev_nr, struct perf_sample *sample, u64 type, const char *sep ){
    fprintf( fid, "%llu%s", ev_nr, sep );
    if( PERF_SAMPLE_IP & type ){
        fprintf( fid, "%llu%s", sample->ip, sep );
    }
    if( PERF_SAMPLE_TID & type ){
        fprintf( fid, "%u%s", sample->pid, sep );
        fprintf( fid, "%u%s", sample->tid, sep );
    }
    if( PERF_SAMPLE_TIME & type ){
        fprintf( fid, "%llu%s", sample->time, sep );
    }
    if( PERF_SAMPLE_ADDR & type ){
        fprintf( fid, "%llu%s", sample->addr, sep );
    }
    if( PERF_SAMPLE_ID & type ){
        fprintf( fid, "%llu%s", sample->id, sep );
    }
    if( PERF_SAMPLE_STREAM_ID & type ){
        fprintf( fid, "%llu%s", sample->stream_id, sep );
    }
    if( PERF_SAMPLE_CPU & type ){
        fprintf( fid, "%u%s", sample->cpu, sep );
    }
    if( PERF_SAMPLE_PERIOD & type ){
        fprintf( fid, "%llu%s", sample->period, sep );
    }
    if( PERF_SAMPLE_RAW & type ){
        unsigned int k;
        unsigned char *p = (unsigned char*)sample->raw_data;
        for( k=0; k < sample->raw_size; k++ ){
          fprintf( fid, " %02x", (int)(p[k]) );
        }
        fprintf( fid, "%s", sep );
    }
    fprintf( fid, "\n" );
};

void printSampleHeader( FILE* fid, u64 type, const char *sep ){
    fprintf( fid, "nr%s", sep );
    if( PERF_SAMPLE_IP & type ){
        fprintf( fid, "ip%s", sep );
    }
    if( PERF_SAMPLE_TID & type ){
        fprintf( fid, "pid%s", sep );
        fprintf( fid, "tid%s", sep );
    }
    if( PERF_SAMPLE_TIME & type ){
        fprintf( fid, "time%s", sep );
    }
    if( PERF_SAMPLE_ADDR & type ){
        fprintf( fid, "addr%s", sep );
    }
    if( PERF_SAMPLE_ID & type ){
        fprintf( fid, "id%s", sep );
    }
    if( PERF_SAMPLE_STREAM_ID & type ){
        fprintf( fid, "stream_id%s", sep );
    }
    if( PERF_SAMPLE_CPU & type ){
        fprintf( fid, "cpu%s", sep );
    }
    if( PERF_SAMPLE_PERIOD & type ){
        fprintf( fid, "period%s", sep );
    }
    if( PERF_SAMPLE_RAW & type ){
        fprintf( fid, "raw%s", sep );
    }
    fprintf( fid, "\n" );
};

void printMmap( FILE* fid, u64 ev_nr, struct mmap_event *evt, const char *sep ){
    fprintf( fid, "%llu%s", ev_nr, sep );
    fprintf( fid, "%u%s", evt->pid, sep );
    fprintf( fid, "%u%s", evt->tid, sep );
    fprintf( fid, "%llu%s", evt->start, sep );
    fprintf( fid, "%llu%s", evt->len, sep );
    fprintf( fid, "%llu%s", evt->pgoff, sep );
    fprintf( fid, "\"%s\"%s", evt->filename, sep );
    fprintf( fid, "\n" );
};

void printMmapHeader( FILE* fid, const char *sep ){
    fprintf( fid, "nr%s", sep );
    fprintf( fid, "pid%s", sep );
    fprintf( fid, "tid%s", sep );
    fprintf( fid, "start%s", sep );
    fprintf( fid, "len%s", sep );
    fprintf( fid, "pgoff%s", sep );
    fprintf( fid, "filename%s", sep );
    fprintf( fid, "\n" );
};

void printComm( FILE* fid, u64 ev_nr, struct comm_event *evt, const char *sep ){
    fprintf( fid, "%llu%s", ev_nr, sep );
    fprintf( fid, "%u%s", evt->pid, sep );
    fprintf( fid, "%u%s", evt->tid, sep );
    fprintf( fid, "\"%s\"%s", evt->comm, sep );
    fprintf( fid, "\n" );
};

void printCommHeader( FILE* fid,const char *sep ){
    fprintf( fid, "nr%s", sep );
    fprintf( fid, "pid%s", sep );
    fprintf( fid, "tid%s", sep );
    fprintf( fid, "comm%s", sep );
    fprintf( fid, "\n" );
};
