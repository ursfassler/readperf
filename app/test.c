#include  <stdio.h>
#include  <stdlib.h>

unsigned int recFib( unsigned int n )
{
  if( n == 0 ){
    return 0;
  } else if( n == 1 ){
    return 1;
  } else {
    return recFib( n-1 ) + recFib( n-2 );
  }
}

unsigned int loopFib( unsigned int n )
{
  unsigned int a0 = 0;
  unsigned int a1 = 1;
  
  while( n > 0 ){
    n--;
    unsigned int t = a1;
    a1 = a1 + a0;
    a0 = t;
  }
  
  return a0;
}

int main( int argc, char **argv )
{
  recFib( 33 );
  loopFib( 10000000 );
  
  return 0;
}

