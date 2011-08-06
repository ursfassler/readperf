#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/wait.h>
#include <pthread.h>

int killid = -1;
int started = 0;

void* thread( void *data )
{
  int id = *((int*)data);
  started = 1;
  while( killid != id ){
    usleep( 1000 * 10 );
  }
  started = 2;
}

int main( int argc, char **argv )
{
  pid_t old_pid = -1;
  pid_t pid;
  int count = 0;
  pthread_t thread_data;

  while( 0 == pthread_create( &thread_data, NULL, thread, &count ) ){
    while( started == 0 ){
      usleep( 10 );
    }
    started = 0;
    count++;
  }
  
  for( killid = 0; killid < 1; killid++ ){
    while( started != 2 ){
      usleep( 10 );
    }
    started = 0;
  }
  
  
  #define CMD_COUNT 5
  static const char *CMD[CMD_COUNT] = { "ls", "df", "uptime", "date", "ps" };
  int k;
  for( k = 0; k < CMD_COUNT; k++ ){
    const char *cmd = CMD[k];
    FILE *fid = popen( cmd, "r" );
    pclose( fid );  
  }
  
  return 0;
}

