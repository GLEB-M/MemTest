#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "common.h"

#define VID_MEM 0xB8000

void clear_screen( );
void set_cursor( ulong x, ulong y );
void set_font( ulong font );
void save_cursor( );
void restore_cursor( );
void print_char( char c );
void print_str( char *s );
void print_dec( ulong num );
void print_dec_64( unsigned long long num );
void print_dec_ra( ulong num, ulong len, char fill );
void print_hex( ulong num );

void print_char_xy( ulong x, ulong y, char c );
void print_str_xy( ulong x, ulong y, char *s );
void print_dec_xy( ulong x, ulong y, ulong num );
void print_dec_ra_xy( ulong x, ulong y, ulong num, ulong len, char fill );
void print_hex_xy( ulong x, ulong y, ulong num );

void print_hex_64( ulong64 num );
void print_hex_64_xy( ulong x, ulong y, ulong64 num );

void printf( char *format, ... );
void printf_xy( int x, int y, char *format, ... );

#endif
