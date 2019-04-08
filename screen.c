#include "screen.h"

ulong cursor_x  = 0;
ulong cursor_y  = 0;
uchar char_font = DEFAULT_FONT;

ulong _cursor_x;
ulong _cursor_y;
uchar _char_font;

void clear_screen( )
{
   ulong i;
   
   cursor_x = 0;
   cursor_y = 0;
   char_font = DEFAULT_FONT;
   
   for ( i = 0; i < 80 * 25; ++i )
      print_char(' ');
   
   cursor_x = 0;
   cursor_y = 0;
}

void set_cursor( ulong x, ulong y )
{
   cursor_x = x;
   cursor_y = y;
}

void set_font( ulong font )
{
   char_font = font;
}

void save_cursor( )
{
   _cursor_x = cursor_x;
   _cursor_y = cursor_y;
   _char_font = char_font;
}

void restore_cursor( )
{
   cursor_x = _cursor_x;
   cursor_y = _cursor_y;
   char_font = _char_font;
}

void print_char( char c )
{
   int cursor;
   
   if ( c == '\n' )
   {
      cursor_y++;
      cursor_x = 0;
      return;
   }
   
   if ( cursor_x >= 80 )
   {
      cursor_x = 0;
      cursor_y++;
   }
   
   if ( cursor_y >= 25 )
      cursor_y = 0;
         
   cursor = (cursor_y * 80 + cursor_x) * 2;
   
   uchar *v = (uchar *)VID_MEM;
   v[cursor] = c;
   v[cursor + 1] = char_font;

   cursor_x++;
}

void print_str( char *s )
{
   int i;
   for ( i = 0; s[i]; ++i )
      print_char(s[i]);
}

void print_dec( ulong num )
{
   int i = 0;
   int rem;
   char buf[] = "          ";
   do
   {
      rem = (num % 10) + 0x30;
      buf[i++] = rem;
      num /= 10;
   }
   while ( num );
   
   for ( i = 9; i >= 0; --i )
      if ( buf[i] != ' ' )
         print_char(buf[i]);
}

void print_dec_ra( ulong num, ulong len, char fill )
{
   int i = 0;
   int rem;
   char buf[10];
   
   for ( i = 0; i < 10; ++i )
      buf[i] = fill;
   
   if ( len > 10 )
      len = 10;
      
   i = 0;
   do
   {
      rem = (num % 10) + 0x30;
      buf[i++] = rem;
      num /= 10;
   }
   while ( num );
   
   for ( i = len - 1; i >= 0; --i )
         print_char(buf[i]);
}

void print_hex( ulong num )
{
   int i = 0;
   int rem;
   char buf[] = "00000000";
   char hex[] = "0123456789abcdef";
   
   do
   {
      rem = num % 16;
      buf[i++] = hex[rem];
      num /= 16;
   }
   while ( num );
   
   for ( i = 7; i >= 0; --i )
      print_char(buf[i]);
}

void print_char_xy( ulong x, ulong y, char c )
{
   cursor_x = x;
   cursor_y = y;
   print_char(c);
}

void print_str_xy( ulong x, ulong y, char *s )
{
   cursor_x = x;
   cursor_y = y;
   print_str(s);
}

void print_dec_xy( ulong x, ulong y, ulong num )
{
   cursor_x = x;
   cursor_y = y;
   print_dec(num);
}

void print_dec_ra_xy( ulong x, ulong y, ulong num, ulong len, char fill )
{
   cursor_x = x;
   cursor_y = y;
   print_dec_ra(num, len, fill);
}

void print_hex_xy( ulong x, ulong y, ulong num )
{
   cursor_x = x;
   cursor_y = y;
   print_hex(num);
}

void print_hex_64( ulong64 num )
{
   print_hex((num >> 32));
   print_hex((num << 32) >> 32);
}

void print_hex_64_xy( ulong x, ulong y, ulong64 num )
{
   cursor_x = x;
   cursor_y = y;
   print_hex_64(num);
}

void _print( char *format, char *arg )
{
   ulong i;
   
   for ( i = 0; format[i]; ++i )
   {
      if ( format[i] == '%' )
      {
         if ( format[i + 1] == 'u' ) // dec
         {
            print_dec(*(ulong *)arg);
            arg += sizeof(ulong);
            ++i;
            continue;
         }
         
         if ( format[i + 1] == 'x' ) // hex
         {
            print_hex(*(ulong *)arg);
            arg += sizeof(ulong);
            ++i;
            continue;
         }
         
         if ( format[i + 1] == 'X' ) // hex64
         {
            print_hex_64(*(ulong64 *)arg);
            arg += sizeof(ulong64);
            ++i;
            continue;
         }
         
         if ( format[i + 1] == 'c' ) // char
         {
            print_char((*(ulong *)arg));
            arg += sizeof(ulong);
            ++i;
            continue;
         }
         
         if ( format[i + 1] == 's' ) // string
         {
            print_str((char *)(*(ulong *)arg));
            arg += sizeof(char *);
            ++i;
            continue;
         }
      }
      print_char(format[i]);
   }
}

void printf( char *format, ... )
{
   _print(format, (char *)&format + sizeof(char *));
}

void printf_xy( int x, int y, char *format, ... )
{
   set_cursor(x, y);
   _print(format, (char *)&format + sizeof(char *));
}

