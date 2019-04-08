#include "test.h"
#include "screen.h"
#include "memsetup.h"

#define ACT_WRITE        "Write  "
#define ACT_VERIFY       "Verify "
#define ACT_SHIFT        "Shift  "
#define ACT_WAIT         "Waiting"
#define ACT_MOVE         "Move   "
#define ACT_SHUFFLE      "Shuffle"
#define ACT_READ         "Read   "

#define PAT_RAND         "RAND NUM"
#define PAT_EMPTY        "-       "


#define MAX_VISIBLE_ERRS 18


struct s_test     tests[TEST_MAX];
struct s_err_info errors[MAX_ERR_INFO];

ulong e_scroll_offs = 0;
ulong e_update      = 0;

ulong test_fail     = 0; // any error detected
ulong total_errors  = 0; // all errors //
ulong pass          = 0; // current pass //
int   test          = 0; // current test //
ulong test_max      = 0;
ulong max_addr      = 0;


// progress //
ulong t_p_max_addr;
ulong t_p_pass_count;
float t_p_pass_offs;
float t_p_pass_offs_val;
ulong t_p_win_count;
float t_p_win_offs;
float t_p_win_offs_val;
ulong t_inc_progress_pass;

void t_fail( )
{
   test_fail = 1;
}

void t_progress_window( ulong win, ulong count )
{
   t_p_win_count = count;
   t_p_win_offs_val = 100.0 / t_p_win_count;
   t_p_win_offs = t_p_win_offs_val * (win - 1); 
}

void t_progress_setup( ulong pages, ulong pass )
{
   t_p_pass_offs = 0;
   t_p_pass_count = pass;
   t_p_max_addr = pages << 12;
   t_p_pass_offs_val = 100.0 / t_p_pass_count;
}

void t_progress_pass( )
{
   t_inc_progress_pass = 1;
   testing_address = (ulong *)start_address;
}

void t_draw_progress( )
{
   float cur_addr, perc;
   
   if ( t_inc_progress_pass )
   {
      t_inc_progress_pass = 0;
      t_p_pass_offs += t_p_pass_offs_val;
   }
   
   cur_addr = (ulong)testing_address - start_address;
   perc = (((cur_addr / t_p_max_addr) * 100.0) / t_p_pass_count) + t_p_pass_offs;
   if ( perc > 100 )
         perc = 100;
   
   float t_prog = (perc / t_p_win_count) + t_p_win_offs;
      
   t_draw_progress_indicator(t_prog, (t_prog / test_max) + (100.0 / test_max * test));
}

void t_add_test( char *name, t_test_proc tp )
{
   ulong i;
   
   if ( test_max == TEST_MAX )
      fatal_error("add_test failed! [test_max = TEST_MAX]");
   
   for ( i = 0; name[i]; ++i )
   {
      tests[test_max].test_name[i] = name[i];
      if ( i == 25 )
      {
         tests[test_max].test_name[i] = '\0';
         break;
      }
   }
   
   tests[test_max].test_proc = tp;
   test_max++;
}

void t_skip_test( int index )
{
   if ( index >= TEST_MAX )
      fatal_error("t_skip_test failed! [bad index]");
   
   tests[index].skip = 1;
}

ulong t_next_test( )
{
   test++;
   if ( test == test_max )
   {
      test = 0;
      return 1; // pass complete
   }
   
   return 0;
}

void t_do_test( ulong start_addr, ulong p )
{
   max_addr = (p << 12) + start_addr;
   
   t_draw_test(&tests[test]);
   
   if ( !tests[test].skip ) // do test //
   {
      (*tests[test].test_proc)(start_addr, p); // call test proc //
   }
   else                    // skip test //
   {
      testing_address = (ulong *)max_addr;
      sleep(250); 
   }
   
   max_addr = 0;
}

//#define _REBOOT_AFTER_N_PASS_ 2

void t_inc_pass( )
{
   if ( total_errors == 0 && pass == 0 )
      print_str_xy(22, 14, "Pass complete. No errors found yet");
   
#ifdef _REBOOT_AFTER_N_PASS_
   
   if ( test_fail == 0 && pass == _REBOOT_AFTER_N_PASS_ - 1 )
      reboot();
   
#endif
   
   pass++;
   print_dec_xy(9, 3, pass); // print current pass //
}


void t_error( ulong type, ulong *addr )
{
   ulong i;
   
   if ( total_errors == 0 )
      print_str_xy(22, 14, "                                  ");
   
   t_fail();
   
   total_errors++;
   
   e_scroll_offs = 0;
   
   // offset errors queue //
   for ( i = MAX_ERR_INFO - 1; i > 0; --i )
   {
      errors[i].num  = errors[i - 1].num; 
      errors[i].pass = errors[i - 1].pass;   
      errors[i].test = errors[i - 1].test;
      errors[i].type = errors[i - 1].type;
      errors[i].addr = errors[i - 1].addr;   
      errors[i].sign = errors[i - 1].sign;            
   }
   
   errors[i].num  = total_errors;
   errors[i].pass = pass;
   errors[i].test = test + 1;
   errors[i].type = type;
   
   if ( type == ERR_ADDR )
   {
      ulong64 phys_addr = get_phys_addr((ulong)addr);
      if ( phys_addr < 0x100000 )
      {
         errors[i].addr = (ulong)(phys_addr >> 10);
         errors[i].sign = 'K';
      }
      else
      {
         errors[i].addr = (ulong)(phys_addr >> 20);
         errors[i].sign = 'M';
      }
   }
   else
      errors[i].addr = 0;
   
   print_dec_xy(9, 4, total_errors);
   e_update = 1;
}


void t_draw_testing_address( )
{
   if ( !max_addr )
   {
      print_str_xy(49, 2,  "-     ");
      return;
   }
   
   // correct address overflow //
   if ( (ulong)testing_address == max_addr )
      testing_address--;

   // get physical address //
   unsigned long long phys = get_phys_addr((ulong)testing_address);
   
   print_str_xy(48, 2, "      ");
   printf_xy(48, 2, "%u%c", (phys < 0x600000) ? (ulong)(phys >> 10) : (ulong)(phys >> 20), (phys < 0x600000) ? 'K' : 'M');
}


void t_scroll_errors( ulong direction )
{
   if ( total_errors <= MAX_VISIBLE_ERRS )
      return;
   
   if ( direction == 0 ) // DOWN //
      if ( e_scroll_offs < (total_errors - MAX_VISIBLE_ERRS) && e_scroll_offs < MAX_ERR_INFO - MAX_VISIBLE_ERRS )
         e_scroll_offs++;
   
   if ( direction == 1 ) // UP //
      if ( e_scroll_offs )
         e_scroll_offs--;
      
   e_update = 1;
}


void t_draw_errors_info( )
{
   ulong i, y = 6;
   
   if ( !total_errors )
      return;
   
   if ( !e_update ) 
      return;
   
   e_update = 0;
   
   set_font(RED_FONT);
   
   for ( i = e_scroll_offs; i < MAX_VISIBLE_ERRS + e_scroll_offs; ++i )
   {
      if ( !errors[i].num ) // ignore empty lines //
         continue;
      
      set_font(ERR_BORDER);
      set_cursor(0, y);
      print_char(221);
      set_cursor(79, y);
      print_char(222);
      
      set_font(RED_FONT);
      set_cursor(1, y);
      ulong n;
      for ( n = 0; n < 78; ++n )
         print_char(' ');
      
      print_dec_xy(1, y, errors[i].num);
      print_dec_xy(21, y, errors[i].pass);
      print_dec_xy(34, y, errors[i].test);
      print_str_xy(67, y, "-");
      
      if ( errors[i].type == ERR_ADDR )
      {
         print_str_xy(46, y, "BAD ADDRESS");
         printf_xy(67, y, "%u%c", errors[i].addr, errors[i].sign);
      }
      
      if ( errors[i].type == ERR_CHECKSUM )
         print_str_xy(46, y, "BAD CHECKSUM");
      
      if ( errors[i].type == ERR_BLOCK_INDEX )
         print_str_xy(46, y, "BAD BLOCK NUM");
      
      ++y;
   }
   
   set_font(DEFAULT_FONT);
}

void t_draw_progress_indicator( float test, float pass )
{
   ulong i;
   
   test = (int)(test + 0.5);
   if ( test > 100 ) test = 100;
   if ( test < 0 ) test = 0;
   ulong t_progress = (test * 26.0) / 100.0;
   
   pass = (int)(pass + 0.5);
   if ( pass > 100 ) pass = 100;
   if ( pass < 0 ) pass = 0;
   
   ulong p_progress = (pass * 26.0) / 100.0;
   
   set_cursor(48, 3);
   print_str("    ");
   set_cursor(48, 3);
   print_dec(test);
   print_char('%');
   
   set_cursor(48, 4);
   print_str("    ");
   set_cursor(48, 4);
   print_dec(pass);
   print_char('%');
   
   // clear //
   set_cursor(53, 3);
   for ( i = 0; i < 26; ++i )
      print_char(' ');
   
   set_cursor(53, 4);
   for ( i = 0; i < 26; ++i )
      print_char(' ');
   
   // test progress //
   set_cursor(53, 3);
   for ( i = 0; i < t_progress; ++i )
      print_char(254);

   // pass progress //
   set_cursor(53, 4);
   for ( i = 0; i < p_progress; ++i )
      print_char(254);
}


void t_draw_test( struct s_test *t )
{
   ulong i;
   
   set_cursor(9, 2);
   for ( i = 0; i < 30; ++i )
      print_char(' ');
   
   if ( !t->skip )
      printf_xy(9, 2, "%u - %s", test + 1, t->test_name);
   else
      printf_xy(9, 2, "%u - [Skip]", test + 1);
}


void t_draw_action( char *s )
{
   print_str_xy(66, 1, s);
}

void t_draw_pattern( ulong p )
{
   print_hex_xy(48, 1, p);
}

void t_draw_pattern_str( char *s )
{
   print_str_xy(48, 1, s);
}

void shuffle_pages( ulong pages_count ) // FIX 0x100000
{
   ulong i;
   ulong64 tmp;
   ulong64 *pages = (ulong64 *)(0x100000 + WINDOW_PAGES_OFFSET); // window pages address //
   
   disable_paging();
   
   for ( i = 0; i < pages_count; ++i )
   {
      tmp = pages[i];
      ulong r = rand() % pages_count;
      pages[i] = pages[r];
      pages[r] = tmp;
   }
   
   enable_paging(0x100000);
}


/****************************************************************************************************************


                  RANDOM BLOCKS MOVE


****************************************************************************************************************/

void write_blocks( ulong start_addr, ulong pages_count, ulong block_size )
{
   ulong register i, n, k;
   ulong blocks = ( pages_count << 12 ) / block_size;
   ulong max = block_size / 4;
   ulong crc;
   ulong pattern;
   
   // WRITE //
   t_draw_action(ACT_WRITE);
   t_draw_pattern_str(PAT_RAND);
   
   randomize();
   for ( n = 0; n < blocks; ++n )
   {
      crc = 0xFFFFFFFF;
      testing_address = (ulong *)(n * block_size + start_addr);
      testing_address += 2;
      for ( i = 0; i < max - 2; ++i, testing_address++ )
      {
         pattern = rand();
         uchar *p = (uchar *)&pattern;
         for ( k = 0; k < 4; ++k )
            crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
         *testing_address = pattern;
      }
      
      testing_address = (ulong *)(n * block_size + start_addr);
      *testing_address = n;                        // block number //
      *(testing_address + 1) = (crc ^ 0xFFFFFFFF); // block checksum //
   }  
}


void shuffle_blocks( ulong start_addr, ulong pages_count, ulong block_size )
{
   ulong register n;
   ulong blocks = ( pages_count << 12 ) / block_size;
   
   t_draw_action(ACT_SHUFFLE);
   t_draw_pattern_str(PAT_EMPTY);
      
   // shuffle blocks indices //
   for ( n = 0; n < blocks; ++n )
   {
      ulong r = rand() % blocks;
         
      ulong *s_block = (ulong *)(n * block_size + start_addr);
      ulong *r_block = (ulong *)(r * block_size + start_addr);
         
      ulong tmp = *s_block;
      *s_block = *r_block;
      *r_block = tmp;
   }
}


void read_blocks( ulong start_addr, ulong pages_count, ulong block_size, ulong random )
{
   ulong register i, n, k;
   ulong blocks = ( pages_count << 12 ) / block_size;
   ulong max = block_size / 4;
   ulong crc;
   ulong *addr;
   
   // shuffle blocks //
   if ( random )
   {
      t_draw_action(ACT_SHUFFLE);
      t_draw_pattern_str(PAT_EMPTY);
      
      for ( n = 0; n < blocks; ++n )
      {
         ulong r = rand() % blocks;
         
         ulong *s_block = (ulong *)(n * block_size + start_addr);
         ulong *r_block = (ulong *)(r * block_size + start_addr);
         
         ulong tmp = *s_block;
         *s_block = *r_block;
         *r_block = tmp;
      }
   }
   
   // READ //
   t_draw_action(ACT_VERIFY);
   t_draw_pattern_str(PAT_EMPTY);
   
   
   testing_address = (ulong *)start_addr; // for progress
   
   for ( n = 0; n < blocks; ++n )
   {
      crc = 0xFFFFFFFF;
      
      addr = (ulong *)(n * block_size + start_addr);
      
      if ( random )
      {
         ulong block_index = *addr;
         
         if ( block_index >= blocks )
         {
            block_index = blocks - 1;
            t_error(ERR_BLOCK_INDEX, 0);
         }
         
         addr = (ulong *)(block_index * block_size + start_addr);
      }
      
      addr += 2;
      for ( i = 0; i < max - 2; ++i, addr++ )
      {
         //crc32_proceed((uchar *)testing_address, &crc);
         uchar *p = (uchar *)addr;
         for ( k = 0; k < 4; ++k )
            crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
         
         testing_address++; // for progress
      }
      
      // Verify //
      addr = (ulong *)(n * block_size + start_addr);
      if ( random )
      {
         ulong block_index = *addr;
         
         if ( block_index >= blocks )
         {
            block_index = blocks - 1;
            t_error(ERR_BLOCK_INDEX, 0);
         }
         
         addr = (ulong *)(block_index * block_size + start_addr);
      }
      if ( *(addr + 1) != (crc ^ 0xFFFFFFFF) ) // bad checksum //
         t_error(ERR_CHECKSUM, 0);
   }
}


void exchange_blocks( ulong start_addr, ulong pages_count, ulong block_size )
{
   ulong register i, n;
   ulong blocks = ( pages_count << 12 ) / block_size;
   ulong max = block_size / 4;
   
   
   t_draw_action(ACT_MOVE);
   t_draw_pattern_str(PAT_EMPTY);
   
   testing_address = (ulong *)start_addr; // for progress
   
   for ( n = 0; n < blocks; ++n )
   {
      ulong *s = (ulong *)(n * block_size + start_addr);
      ulong dest_index = *s;
      if ( dest_index >= blocks )
      {
         dest_index = blocks - 1;
         t_error(ERR_BLOCK_INDEX, 0);
      }
      ulong *d = (ulong *)(dest_index * block_size + start_addr);
      
      ulong tmp;
      
      // skip block index
      s++;
      d++;
      
      for ( i = 0; i < max - 1; ++i, s++, d++ )
      {
         tmp = *s;
         *s = *d;
         *d = tmp;
         
         testing_address++; // for progress
      }
   }
}

void move_blocks( ulong start_addr, ulong pages_count, ulong block_size )
{
   ulong i;

   t_progress_setup(pages_count, 66);
   
   write_blocks(start_addr, pages_count, block_size);
   
   for ( i = 0; i < 64; ++i )
   {
      t_progress_pass();
      shuffle_blocks(start_addr, pages_count, block_size);
      exchange_blocks(start_addr, pages_count, block_size);
   }
   
   t_progress_pass();
   read_blocks(start_addr, pages_count, block_size, 0);
}


/********************************************************

                 ZERO & ONE

*********************************************************/
void zero_and_one( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong n;
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 6);
   
   t_draw_pattern(0x0);
   t_draw_action(ACT_WRITE);
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      *testing_address = 0x0;
   
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      if ( *testing_address != 0x0 )
         t_error(ERR_ADDR, testing_address);
      
   t_draw_pattern(0xFFFFFFFF);
   t_draw_action(ACT_WRITE);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      *testing_address = 0xFFFFFFFF;
   
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      if ( *testing_address != 0xFFFFFFFF )
         t_error(ERR_ADDR, testing_address);
      
   t_draw_pattern(0x0);
   t_draw_action(ACT_WRITE);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      *testing_address = 0x0;
   
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      if ( *testing_address != 0x0 )
         t_error(ERR_ADDR, testing_address);
}


/********************************************************

              BITS INVERSION (8 BIT)

*********************************************************/

void bits_inv8( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 4);
   
   // Write 0xFF00FF00 //
   t_draw_action(ACT_WRITE);
   t_draw_pattern(0xFF00FF00);
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      *testing_address = 0xFF00FF00;
   
   // Verify 0xFF00FF00 //
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      if ( *testing_address != 0xFF00FF00 )
         t_error(ERR_ADDR, testing_address);
      
   // Write 0xFF00FF00 //
   t_draw_action(ACT_WRITE);
   t_draw_pattern(0x00FF00FF);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      *testing_address = 0x00FF00FF;   
   
   // Verify 0x00FF00FF  //
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
      if ( *testing_address != 0x00FF00FF )
         t_error(ERR_ADDR, testing_address);
}


/********************************************************

              BITS INVERSION (32BIT)

*********************************************************/
void bits_inv32( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong pattern;
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 4);
   
   // Write //
   pattern = 0x00000000;
   t_draw_action(ACT_WRITE);
   t_draw_pattern(pattern);
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {
      *testing_address = pattern;
      pattern = ~pattern;
   }
      
   // Verify  //
   pattern = 0x00000000;
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {  
      if ( *testing_address != pattern )
         t_error(ERR_ADDR, testing_address);
      pattern = ~pattern;
   }

   // Write //
   pattern = 0xFFFFFFFF;
   t_draw_action(ACT_WRITE);
   t_draw_pattern(pattern);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {
      *testing_address = pattern;
      pattern = ~pattern;
   }
      
   // Verify  //
   pattern = 0xFFFFFFFF;
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {  
      if ( *testing_address != pattern )
         t_error(ERR_ADDR, testing_address);
      pattern = ~pattern;
   }
}


/********************************************************

         12 Patterns (8 Bit)

*********************************************************/
void patterns_12( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong n;
   ulong patterns[] = { 
                  0x55555555, 0xAAAAAAAA, 
                  0x0F0F0F0F, 0xF0F0F0F0, 
                  0x3C3C3C3C, 0xC3C3C3C3, 
                  0x66666666, 0x99999999, 
                  0x81818181, 0x7E7E7E7E, 
                  0x18181818, 0xE7E7E7E7 
                  };
                  
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 24);
   
   for ( n = 0; n < 12; ++n )
   {
      // WRITE //
      t_draw_action(ACT_WRITE);
      t_draw_pattern(patterns[n]);
      if ( n > 0 )
         t_progress_pass();
      for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
         *testing_address = patterns[n];
      
      // Verify //   
      t_draw_action(ACT_VERIFY);
      t_draw_pattern(patterns[n]);
      t_progress_pass();
      for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
         if ( *testing_address != patterns[n] )
            t_error(ERR_ADDR, testing_address); 
   }
}


/********************************************************

               Bit Walking (8BIT)

*********************************************************/
void bit_walking8( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong j, n;
   ulong pattern; 
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 32);
   
   pattern = 0xFEFEFEFE; // 11111110111111101111111011111110
   
   for ( j = 0; j < 2; ++j )
   {
      for ( n = 0; n < 8; ++n )
      {
         // WRITE //
         t_draw_action(ACT_WRITE);
         t_draw_pattern(pattern);
         
         if ( !(n == 0 && j == 0) )
            t_progress_pass();
         
         for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )  
            *testing_address = pattern;
         
         // CHECK //
         t_draw_action(ACT_VERIFY);
         t_draw_pattern(pattern);
         
         t_progress_pass();
         for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )  
            if ( *testing_address != pattern )
               t_error(ERR_ADDR, testing_address);
         
         // Shift pattern //        
         __asm volatile("rol dword ptr [%0], 0x00000001" : : "m"(pattern));   
         
      }
      // Invert pattern //
      pattern = ~pattern;
   }
}


/********************************************************

         Bit Walking (32BIT)

*********************************************************/
void bit_walking32( ulong start_addr, ulong pages_count, ulong random )
{
   ulong register i;
   ulong j, n;
   ulong pattern; 
   ulong max = (pages_count << 12) / 4;
   
   t_progress_setup(pages_count, 128);
   
   pattern = 0xFFFFFFFE; // 11111111111111111111111111111110
   
   for ( j = 0; j < 2; ++j )
   {
      for ( n = 0; n < 32; ++n )
      {
         if ( random ) 
            shuffle_pages(pages_count);
         
         // WRITE //
         t_draw_action(ACT_WRITE);
         t_draw_pattern(pattern);
         
         if ( !( n == 0 && j == 0 ) )
            t_progress_pass();
         for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )  
            *testing_address = pattern;
         
         if ( random ) 
            shuffle_pages(pages_count);
         
         // CHECK //
         t_draw_action(ACT_VERIFY);
         t_draw_pattern(pattern);
         
         t_progress_pass();
         for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )  
            if ( *testing_address != pattern )
               t_error(ERR_ADDR, testing_address);
            
         // Shift pattern //  
         __asm volatile("rol dword ptr [%0], 0x00000001" : : "m"(pattern));
         
      }
      // Invert pattern //
      pattern = ~pattern;
   }
}


void walk32( ulong start_addr, ulong pages_count )
{
   bit_walking32(start_addr, pages_count, 0);
}

void walk32_RA( ulong start_addr, ulong pages_count )
{
   bit_walking32(start_addr, pages_count, 1);
}


/********************************************************

   Bit Walking (32BIT, Random data, Random access) 

*********************************************************/ 
void walk32_RD_RA( ulong start_addr, ulong pages_count )
{
   ulong register i;
   ulong n;
   ulong pattern;
   ulong crc;
   ulong max = (pages_count << 12) / 4; 
   
   t_progress_setup(pages_count, 34);
   
   // WRITE //
   t_draw_action(ACT_WRITE);
   t_draw_pattern_str(PAT_RAND);
   
   randomize();
   for ( n = 0; n < pages_count; ++n )
   {
      crc = 0xFFFFFFFF;
      testing_address = (ulong *)(n * 0x1000 + start_addr);
      for ( i = 0; i < 1024 - 1; ++i, testing_address++ )
      {
         pattern = rand();
         //crc32_proceed((uchar *)&pattern, &crc);
         ulong k;
         uchar *p = (uchar *)&pattern;
         for ( k = 0; k < 4; ++k )
            crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
         *testing_address = pattern;
      }
      *testing_address = crc ^ 0xFFFFFFFF; // save checksum to last dword //
   }  

   // SHIFT //
   t_draw_action(ACT_SHIFT);
   t_draw_pattern_str(PAT_EMPTY);
   for ( n = 0; n < 32; ++n )
   {
      t_progress_pass();
      shuffle_pages(pages_count);
      for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )  
         __asm volatile("rol dword ptr [%0], 0x00000001" : : "m"(*testing_address));
      
   }
   
   // VERIFY //
   t_draw_action(ACT_VERIFY);

   shuffle_pages(pages_count);
   t_progress_pass();
   for ( n = 0; n < pages_count; ++n )
   {
      crc = 0xFFFFFFFF;
      testing_address = (ulong *)(n * 0x1000 + start_addr);
      for ( i = 0; i < 1024 - 1; ++i, testing_address++ )
      {
         //crc32_proceed((uchar *)testing_address, &crc);
         ulong k;
         uchar *p = (uchar *)testing_address;
         for ( k = 0; k < 4; ++k )
            crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
      }
      
      if ( *testing_address != (crc ^ 0xFFFFFFFF) ) // bad checksum //
         t_error(ERR_ADDR, testing_address - 1023); // page address
   }
}


/********************************************************

            Multiple Read

*********************************************************/ 

void multiple_read(ulong start_addr,  ulong pages_count )
{
   ulong register i;
   ulong n;
   ulong pattern;
   ulong crc;
   ulong checksum;
   ulong max = (pages_count << 12) / 4; 
   volatile ulong dest = 0;
   
   t_progress_setup(pages_count, 66);
   
   // WRITE //
   t_draw_action(ACT_WRITE);
   t_draw_pattern_str(PAT_RAND);
   
   randomize();
   crc = 0xFFFFFFFF;
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {
      pattern = rand();
      //crc32_proceed((uchar *)&pattern, &crc);
      ulong k;
      uchar *p = (uchar *)&pattern;
      for ( k = 0; k < 4; ++k )
         crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
      *testing_address = pattern;
   }
   checksum = crc ^ 0xFFFFFFFF; // save checksum 
   

   // Multiple read //
   t_draw_action(ACT_READ);
   t_draw_pattern_str(PAT_EMPTY);

   for ( n = 0; n < 64; ++n )
   {
      t_progress_pass();
      for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
         dest = *testing_address;
   }
   
   // Verify Checksum  //
   t_draw_action(ACT_VERIFY);
   t_progress_pass();
   
   crc = 0xFFFFFFFF;
   for ( testing_address = (ulong *)start_addr, i = 0; i < max; ++i, testing_address++ )
   {
      //crc32_proceed((uchar *)testing_address, &crc);
      ulong k;
      uchar *p = (uchar *)testing_address;
      for ( k = 0; k < 4; ++k )
         crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
   }
   
   if ( checksum != (crc ^ 0xFFFFFFFF) )
      t_error(ERR_CHECKSUM, 0);
}

/********************************************************

                  Blocks Move

*********************************************************/ 

void move4( ulong start_addr, ulong pages_count )
{
   move_blocks(start_addr, pages_count, 4096);
}

void move64( ulong start_addr, ulong pages_count )
{
   move_blocks(start_addr, pages_count, 65536);
}

void move512( ulong start_addr, ulong pages_count )
{
   move_blocks(start_addr, pages_count, 524288);
}

void move1024( ulong start_addr, ulong pages_count )
{
   move_blocks(start_addr, pages_count, 1048576);
}


/********************************************************

   Test First Megabyte And Pages Location (1-5 Mb)

*********************************************************/ 

void test0( ulong start, ulong pages )
{
   zero_and_one(start, pages);
   bits_inv8(start, pages);
   bits_inv32(start, pages);
   patterns_12(start, pages);
   bit_walking8(start, pages);
   bit_walking32(start, pages, 0);
}


void test_low_mem( )
{
   printf_xy(9, 2, "0 - Low Memory (Cache Off)");
   
   ulong base_limit = mem_get_base_mem_size( );
   
   printf_xy(66, 2, "%uK", base_limit >> 10);
   
   base_limit -= 0x21000;
   base_limit >>= 12;
   
   cache_off();
   
   test = -1;
   
   t_progress_window(1, 1);
   
   // 0..8000 //
   start_address = 0x0;
   max_addr = 0x8000;       
   test0(start_address, 0x8);
   
   // 8000..21000 = 100KB don't test, it's program code location //
   
   // 21000..EBDA //
   start_address = 0x21000;
   max_addr = /*0xA0000*/base_limit << 12;
   test0(start_address, /*0x7F*/base_limit);
   
   cache_on();
   
   printf_xy(9, 2, "0 - Pages Memory          ");
   
   printf_xy(66, 2, "6M  ");
   
   // 100000..600000 //
   start_address = 0x100000;
   max_addr = 0x600000;
   test0(start_address, 0x500);
   
   test = 0;
   testing_address = (ulong *)0x600000;
   start_address = 0x600000;
   
   
   // clear pages mem ???? //
   ulong register i;
   volatile ulong *addr = (ulong *)0x100000;
   for ( i = 0; i < 1310720; ++i )
      *addr++ = 0;
}
