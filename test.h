#ifndef _TEST_H_
#define _TEST_H_

#include "common.h"

#define TEST_MAX        20
#define MAX_ERR_INFO    40

#define ERR_ADDR        0
#define ERR_CHECKSUM    1
#define ERR_BLOCK_INDEX 2

typedef void (*t_test_proc)( ulong, ulong );

struct s_test
{
   char        test_name[32];
   uchar       skip;
   t_test_proc test_proc;
};

struct s_err_info
{
   ulong num;
   uchar pass;
   uchar test;
   ulong type;
   ulong addr;
   uchar sign;
};

void  t_fail( );
void  t_progress_window( ulong win, ulong count );
void  t_progress_setup( ulong pages, ulong pass );
void  t_progress_pass( );
void  t_draw_progress( );

void  t_error( ulong type, ulong *addr );
void  t_scroll_errors( ulong direction );
void  t_draw_errors_info( );
void  t_draw_progress_indicator( float test, float pass  );
void  t_draw_test( struct s_test *t );
void  t_draw_action( char *s );
void  t_draw_pattern( ulong p );
void  t_inc_pass( );

void  t_draw_testing_address( );

void  t_add_test( char *name, t_test_proc tp );
void  t_skip_test( int index );
ulong t_next_test( );
void  t_do_test( ulong start_addr, ulong p );

// TESTS //
void  zero_and_one( ulong start_addr, ulong pages_count );
void  bits_inv8( ulong start_addr, ulong pages_count ); 
void  bits_inv32( ulong start_addr, ulong pages_count ); 
void  patterns_12( ulong start_addr, ulong pages_count );
void  bit_walking8( ulong start_addr, ulong pages_count );
void  walk32( ulong start_addr, ulong pages_count ); 
void  walk32_RA( ulong start_addr, ulong pages_count );
void  walk32_RD_RA( ulong start_addr, ulong pages_count );
void  multiple_read( ulong start_addr, ulong pages_count );
void  move4( ulong start_addr, ulong pages_count );
void  move64( ulong start_addr, ulong pages_count );
void  move512( ulong start_addr, ulong pages_count );
void  move1024( ulong start_addr, ulong pages_count );
void  test_low_mem( );

#endif
