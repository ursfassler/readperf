/**
 * @file
 * This file containes copies of types and functions of the original perf
 * source.
 */

#ifndef __ORIGPERF_H__
#define __ORIGPERF_H__

#include  <limits.h>
#include  <linux/perf_event.h>
#include  <sys/types.h>
#include  <unistd.h>
#include  "types.h"

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

/*
 * PERF_SAMPLE_IP | PERF_SAMPLE_TID | *
 */
struct ip_event {
	struct perf_event_header header;
	u64 ip;
	u32 pid, tid;
	unsigned char __more_data[];
};

struct mmap_event {
	struct perf_event_header header;
	u32 pid, tid;
	u64 start;
	u64 len;
	u64 pgoff;
	char filename[PATH_MAX];
};

struct comm_event {
	struct perf_event_header header;
	u32 pid, tid;
	char comm[16];
};

struct fork_event {
	struct perf_event_header header;
	u32 pid, ppid;
	u32 tid, ptid;
	u64 time;
};

struct lost_event {
	struct perf_event_header header;
	u64 id;
	u64 lost;
};

/*
 * PERF_FORMAT_ENABLED | PERF_FORMAT_RUNNING | PERF_FORMAT_ID
 */
struct read_event {
	struct perf_event_header header;
	u32 pid, tid;
	u64 value;
	u64 time_enabled;
	u64 time_running;
	u64 id;
};

struct sample_event {
	struct perf_event_header        header;
	u64 array[];
};

struct ip_callchain {
	u64 nr;
	u64 ips[0];
};

struct perf_sample {
	u64 ip;
	u32 pid, tid;
	u64 time;
	u64 addr;
	u64 id;
	u64 stream_id;
	u64 period;
	u32 cpu;
	u32 raw_size;
	void *raw_data;
	struct ip_callchain *callchain;
};

#define BUILD_ID_SIZE 20

struct build_id_event {
	struct perf_event_header header;
	pid_t			 pid;
	u8			   build_id[ALIGN(BUILD_ID_SIZE, sizeof(u64))];
	char			 filename[];
};

enum perf_user_event_type { /* above any possible kernel type */
	PERF_RECORD_USER_TYPE_START		= 64,
	PERF_RECORD_HEADER_ATTR			= 64,
	PERF_RECORD_HEADER_EVENT_TYPE		= 65,
	PERF_RECORD_HEADER_TRACING_DATA		= 66,
	PERF_RECORD_HEADER_BUILD_ID		= 67,
	PERF_RECORD_FINISHED_ROUND		= 68,
	PERF_RECORD_HEADER_MAX
};

struct attr_event {
	struct perf_event_header header;
	struct perf_event_attr attr;
	u64 id[];
};

#define MAX_EVENT_NAME 64

struct perf_trace_event_type {
	u64	event_id;
	char	name[MAX_EVENT_NAME];
};

struct event_type_event {
	struct perf_event_header header;
	struct perf_trace_event_type event_type;
};

struct tracing_data_event {
	struct perf_event_header header;
	u32 size;
};

union perf_event {
	struct perf_event_header	header;
	struct ip_event			ip;
	struct mmap_event		mmap;
	struct comm_event		comm;
	struct fork_event		fork;
	struct lost_event		lost;
	struct read_event		read;
	struct sample_event		sample;
	struct attr_event		attr;
	struct event_type_event		event_type;
	struct tracing_data_event	tracing_data;
	struct build_id_event		build_id;
};

int perf_event__parse_sample(const union perf_event *event, u64 type, bool sample_id_all, struct perf_sample *data);

#endif

