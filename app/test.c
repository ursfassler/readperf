#include  <stdio.h>
#include  <stdlib.h>

struct node_t {
  struct node_t *left;
  struct node_t *rigth;
};

struct node_t* randTree( unsigned int deepth );

struct node_t* makeNode( unsigned int deepth )
{
  if( (deepth == 0) || (rand() % 4 == 0) ){
    return NULL;
  } else {
    return randTree( deepth );
  }
}

struct node_t* randTree( unsigned int deepth )
{
  struct node_t *node;
  node = malloc( sizeof( *node ) );
  if( node == NULL ){
    perror( "randTree" );
    exit( -1 );
  }
  node->left = makeNode( deepth-1 );
  node->rigth = makeNode( deepth-1 );
  return node;
}

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
  srand( 42 );
  struct node_t* node = randTree( 30 );
  printf( "%u\n", recFib( 33 ) );
  printf( "%u\n", loopFib( 10000000 ) );
  
  return 0;
}

