#include <stdio.h>

#define FALSE 0
#define TRUE 1

typedef unsigned long int uint32;
typedef long int int32;
typedef unsigned short int uint16;
typedef short int int16;
typedef unsigned char uint8;
typedef char int8;

uint32 reverse32( uint32 n );
uint16 reverse16( uint16 n );
int skip_vlq( FILE *infile );
int vlq_length( uint32 value );
int read_vlq( uint32* value, FILE* infile );
int write_vlq( uint32 value, FILE* outfile );
