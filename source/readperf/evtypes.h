#ifndef __EVTYPES_H__
#define __EVTYPES_H__

#include  "evsample.h"

struct evlistEntry{
  struct perf_trace_event_type  event;
  unsigned int                  count;
};

void* createEvlist( unsigned int count );
void destroyEvlist( void *list );
struct evlistEntry* getEvtypeById( void *list, u64 id );
struct evlistEntry* getEvtypeByNr( void *list, unsigned int nr );

#endif

