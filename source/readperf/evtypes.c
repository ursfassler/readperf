
#include  <stdlib.h>
#include  <string.h>
#include  "evtypes.h"

struct evlist{
  unsigned int        count;
  struct evlistEntry  data;
};

void* createEvlist( unsigned int count )
{
  size_t size = sizeof(unsigned int) + count * sizeof(struct evlistEntry);
  struct evlist *list = (struct evlist *)malloc( size );
  memset( list, 0, size );
  list->count = count;
  return list;
}

void destroyEvlist( void *list )
{
  free( list );
}

struct evlistEntry* getEvtypeById( void *list, u64 id )
{
  unsigned int i = ((struct evlist*)list)->count;
  struct evlistEntry  *itr = &(((struct evlist*)list)->data);
  
  while( i > 0 ){
    if( itr->event.event_id == id ){
      return itr;
    }
    itr++;
    i--;
  }
  return  NULL;
}

struct evlistEntry* getEvtypeByNr( void *list, unsigned int nr )
{
  if( nr >= ((struct evlist*)list)->count ){
    return NULL;
  }
  struct evlistEntry  *itr = &(((struct evlist*)list)->data);
  return itr+nr;
}

