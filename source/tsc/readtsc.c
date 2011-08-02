#include  <stdio.h>
#include  "cycle.h"

unsigned long long int rdtsc(void)
{
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

int main( int argc, char **argv )
{
  unsigned long long int ticka = rdtsc();
  printf( "%llu\n", ticka );
  
  ticks tickb = getticks();
  
  printf( "%llu\n", tickb );

  return 0;
}


