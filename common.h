#ifndef _COMMON_H_
#define _COMMON_H_

typedef unsigned char      uchar;
typedef unsigned short     ushort;
typedef unsigned long      ulong;
typedef unsigned long long ulong64;


#define DEFAULT_FONT 113 
#define HEADER_FONT  31 
#define RED_FONT     79
#define GREEN_FONT   47

#define ERR_BORDER   65

// GLOBAL VARS //
extern ulong tick_counter;
extern float time;
extern ulong *testing_address;
extern ulong start_address;

extern ulong crc32_table[256];

// COMMON PROCS //
void  fatal_error( char *msg );

void  sleep( ulong msec );

void  randomize( );
ulong rand( );
void  rand_seed( ulong seed1, ulong seed2 );

void  disable_interrupts( );
void  enable_interrupts( );

void  cache_off( );
void  cache_on( );
void  wbinvd( );

void  reboot( );

ulong get_tick_count( );

#endif
