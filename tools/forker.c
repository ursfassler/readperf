/**
 * @file
 * Produces the desired number of forks / processes.
 * Can be used to stress the system and force an overflow of pid.
 *
 * Copyright 2011 Urs Fässler, www.bitzgi.ch
 * SPDX-License-Identifier:	GPL-3.0+
 */

#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/wait.h>

#define CMD_COUNT 5
static const char *CMD[CMD_COUNT] = { "ls", "df", "uptime", "date", "ps" };

int main( int argc, char **argv )
{
  if( argc != 2 ){
    printf( "need number of forks\n" );
    exit( -1 );
  }
  srand( 42 );
  
  unsigned int forks = atoi( argv[1] );
  unsigned int parent_forks = 0;
  unsigned int child_forks = 0;
  
  
  do{
    forks--;
    parent_forks = rand() % forks;
    child_forks  = forks - parent_forks;
    
    pid_t pid = fork();
    
    if( pid < 0 ){
      perror( ":-(" );
      exit( -1 );
    }
    
    usleep( rand() % 2000 );
    
    if( pid == 0 ){
      forks = child_forks;
    } else {
      forks = parent_forks;
    }
  
    const char *cmd = CMD[rand() % CMD_COUNT];
  
    FILE *fid = popen( cmd, "r" );
    pclose( fid );  
    
    if( pid > 0 ){
      int status;
      waitpid( pid, &status, 0 );
    }
    
  } while( forks > 0 );
  
  return 0;
}

