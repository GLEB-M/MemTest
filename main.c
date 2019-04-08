#include "common.h"
#include "screen.h"
#include "test.h"
#include "memsetup.h"
#include "hw.h"
#include "version.h"

ulong *testing_address = (ulong *)0x600000;//START_ADDRESS; // fix
ulong start_address = 0x600000;

ulong _wait_key = 0xFFFFFFFF;

ulong interface_init;

ulong config_mode;
ulong start_test;
ulong set_tests;
ulong test_mode;

#define MAX_MODES 4

char *modes[MAX_MODES] = 
{
   " Full Test        ",
   " Fast Test        ", 
   " Hard Errors Only ",
   " Stability Only   " 
};

#define MODE_FULL 0
#define MODE_FAST 1
#define MODE_HARD 2
#define MODE_STAB 3

void draw_test_config( )
{
   ulong i;
   printf_xy(1, 0, "Test Mode:");
   printf_xy(1, 24, "%c%c - Select mode, ENTER - Start test, ESC - Reboot", 24, 25);
   
   for ( i = 0; i < MAX_MODES; ++i )
   {
      if ( test_mode == i )
         set_font(HEADER_FONT);
      
      print_str_xy(3, 2 + i, modes[i]);
      
      set_font(DEFAULT_FONT);
   }
}

void draw_interface( )
{
   ulong i;
   
   clear_screen();
   
   for ( i = 1; i < 24; ++i )
   {
      set_cursor(0, i);
      print_char(221);
   }
   
   for ( i = 1; i < 24; ++i )
   {
      set_cursor(79, i);
      print_char(222);
   }
   
   // HEADERS //
   set_font(HEADER_FONT);
   
   set_cursor(0, 0);
   for ( i = 0; i < 80; ++i )
      print_char(' ');
   
   printf_xy(1, 0, "GL MemTest -%s", modes[test_mode]);
   print_str_xy(71, 0, "00:00:00");
   
   
   set_cursor(0, 5);
   for ( i = 0; i < 80; ++i )
      print_char(' ');
   print_str_xy(1, 5, "Error number        Pass         Test        Type                 Address");
   

   set_cursor(0, 24);
   for ( i = 0; i < 80; ++i )
      print_char(' ');
   
   
   print_str_xy(1, 24, "ESC - Reboot");
   printf_xy(15, 24, "%c%c - Scroll Errors", 24, 25);
      
   set_font(DEFAULT_FONT); 
   
   print_str_xy(1,  1, "Memory:");
   print_str_xy(1,  3, "Pass:   0");
   print_str_xy(1,  4, "Errors: 0");
   
   print_str_xy(1,  2, "Test:");
   
   print_str_xy(39, 1, "Pattern:");
   print_str_xy(58, 1, "Action:");
   
   
   print_str_xy(58, 2, "Block:");
   print_str_xy(39, 2, "Address:");
   
      
   print_str_xy(39, 3, "Current: 0%");
   print_str_xy(39, 4, "Total:   0%");
         
   interface_init = 1;
}

void draw_mem_info( )
{
   printf_xy(9, 1, "%uM [%uK]", (get_total_pages() + BASE_MEM) >> 8, (get_total_pages() + BASE_MEM) << 2);
}

void draw_cpu_heat( )
{
   static ulong overheat = 0;
   ulong h = get_cpu_heat(&overheat);

   if ( h )
   {
      set_font(HEADER_FONT);
      
      if ( h >= 95 ) // overheat! //
      {
         overheat = 1;
         t_fail();
      }
      
      if ( overheat )
         set_font(RED_FONT);
      
      print_str_xy(63, 24, "               ");
      printf_xy(63, 24,    " CPU Heat: %u% ", h);
      
      set_font(DEFAULT_FONT);
   }
}

void draw_time( )
{
   static ulong cl_sec, cl_min, cl_hour;
   
   if ( ++cl_sec == 60 )
   {
      cl_sec = 0;
      cl_min++;
   }
         
   if ( cl_min == 60 )
   {
      cl_min = 0;
      cl_hour++;
   }
         
   if ( cl_hour == 100 )
      cl_hour = 0;
         
   set_font(HEADER_FONT);
   print_dec_ra_xy(71, 0, cl_hour, 2, '0');
   print_dec_ra_xy(74, 0, cl_min, 2, '0');
   print_dec_ra_xy(77, 0, cl_sec, 2, '0');
   set_font(DEFAULT_FONT);
}


void draw_window( ulong win, ulong count, ulong size )
{
   printf_xy(66, 2, "             ");
   printf_xy(66, 2, "%u/%u - %u%c", win, count, (size < 256) ? (size << 2) : (size >> 8), (size < 256) ? 'K' : 'M');
   t_progress_window(win, count);
}

void timer_proc( )
{
   save_cursor();
   
   tick_counter++;
   
   time += 54.925493;
   
   // delay 55 ms //
   static ulong t;
   t++;
   if ( t <= 1 )
      return;
   t = 0;
   
   if ( interface_init )
   {
      // draw time //
      if ( time >= 1000 )
      {
         draw_time();
         time = 0;
      }
      
      // draw current address //
      t_draw_testing_address();
      
      // draw errors //
      t_draw_errors_info();
      
      // draw progress //
      t_draw_progress();
      
      // draw CPU heat //
      draw_cpu_heat();
   }
   
   restore_cursor();
}

ulong pause = 0;

void keyb_proc( ulong code )
{
   if ( code & 0x80 ) // ignore key release
      return;
      
   if ( _wait_key == 0 )
   {
      _wait_key = code;
      return;
   }
   
   save_cursor();
   
   if ( !start_test )
   {
      if ( code == 28 ) // ENTER - start test //
         start_test = 1;
      
      if ( code == 46 ) // C 
      {
         clear_screen();
         draw_test_config();
         config_mode = 1;
         pause = 1;
      }
      
      if ( code == 1 ) // ESC - reboot //
      {
         if ( !pause )
         {
            printf_xy(0, 24, " ENTER - Start test, C - Configure test, ESC - Reboot");
            printf_xy(78, 24, " ");
            pause = 1;
         }
         else
         {
            reboot();
         }
      }
   }
   
   if ( config_mode )
   {
      if ( code == 72 ) // UP
      {
         if ( test_mode == 0 )
            test_mode = MAX_MODES - 1;
         else 
            test_mode--;
         
         draw_test_config();
      }

      if ( code == 80 ) // DOWN
      {
         if ( test_mode == MAX_MODES - 1 )
            test_mode = 0;
         else 
            test_mode++;
         
         draw_test_config();
      }
      
   }
   
   if ( interface_init ) 
   {
      if ( code == 1 )  // ESC - reboot //
         reboot();

      if ( code == 72 ) // UP
         t_scroll_errors(1);
      
      if ( code == 80 ) // DOWN
         t_scroll_errors(0);
   
   }
   restore_cursor();
}


void test_window( ulong win, ulong win_count, ulong pages_count )
{
   draw_window(win, win_count, pages_count);
   
   t_do_test(start_address, pages_count); // FIX
   
   if ( win == win_count )
      if ( t_next_test() )
         t_inc_pass();
}


void run_test( )
{
   ulong i, n;
   
   ulong64 *pages = (ulong64 *)(0x100000 + WINDOW_PAGES_OFFSET); // window pages address //
   ulong64 window_pages_num = 0;
   ulong64 pages_max = 0;
   
   // create 2GB windows //
   struct s_pages_map *pmap = get_pages_map();
   ulong pmap_max = get_pages_map_entries_num();
   ulong total_pages = get_total_pages();
   
   ulong window = 1;
   ulong windows_count = mem_get_windows(0);
   
   disable_paging();
   
   for ( i = 2; i < pmap_max; ++i )
   {
      ulong64 addr = pmap[i].addr;
      for ( n = 0; n < pmap[i].num; ++n )
      {
         pages[window_pages_num] = addr | 7;
         addr += 4096;
         
         window_pages_num++;
         pages_max++;
   
         // window created //
         if ( window_pages_num == WINDOW_SIZE || pages_max == (total_pages /*- BASE_MEM*/) )
         {
            // test window //
            enable_paging(0x100000);
            test_window(window, windows_count, window_pages_num);
            disable_paging();
            
            window_pages_num = 0;
            window++;
         }
      }
   }
   
   enable_paging(0x100000);
}


void wait_key( uchar key )
{
   _wait_key = 0;
   
   while ( 1 )
   {
      if ( _wait_key )
      {
         if ( !key ) // any key
            break;
         else
         {
            if ( key == _wait_key )
               break;
         }
         _wait_key = 0;
      }
   }
            
   _wait_key = 0xFFFFFFFF;
}


void interrupts_init( )
{
   // set timer & keyb proc // 
   disable_interrupts();
   *((ulong *)(0x8000 + 8))  = (ulong)&timer_proc;
   *((ulong *)(0x8000 + 12)) = (ulong)&keyb_proc;
   enable_interrupts();
}

void draw_hw_info( )
{
   char cpu_model[100];
   cpu_model[0] = 0;
   get_cpu_model(cpu_model);
   
   if ( cpu_model[0] )
      printf(" CPU:       %s\n", cpu_model);
   else
      printf(" CPU:       %s\n", get_cpu_vendor());
   
   printf("\n Installed:");
   
   if ( !dmi_init() )
      printf(" Unknown\n");
   
   printf("\n");
}

void configure( )
{
   ulong t0 = tick_counter;
   ulong timeout = 5;
   
   print_dec_xy(78, 24, timeout);
   
   while ( 1 )
   {
      if ( start_test )
      {
         config_mode = 0;
         break;
      }
      
      if ( !pause )
      {
         if ( tick_counter - t0 >= 1000 / 55 )
         {
            print_dec_xy(78, 24, --timeout);
            t0 = tick_counter;
         }
         
         if ( !timeout )
         {
            config_mode = 0;
            break;
         }
      }
   }
}

void kernel_main( )
{
   clear_screen();
   
   printf(" GL MemTest v3.0 - 19/01/2012 - Build %u\n\n", BUILD_NUM); 

   cpu_init();
   
   draw_hw_info();
   
   mem_init();
   
   interrupts_init();
   
   print_str_xy(0, 24, " ENTER - Start test, C - Configure test, ESC - Pause");

   configure();
   
   draw_interface();
   draw_mem_info();
   
   if ( !(test_mode == MODE_FAST || test_mode == MODE_HARD || test_mode == MODE_STAB) )
      test_low_mem();
   
   build_pages(0x100000);
   enable_paging(0x100000);
   
   /*0*/  t_add_test("Zero And One",              zero_and_one);
   /*1*/  t_add_test("Bits Inversion (8Bit)",     bits_inv8);
   /*2*/  t_add_test("Bits Inversion (32Bit)",    bits_inv32);
   /*3*/  t_add_test("12 Patterns (8Bit)",        patterns_12);
   /*4*/  t_add_test("Bit Walking (8Bit)",        bit_walking8);
   /*5*/  t_add_test("Bit Walking (32Bit)",       walk32);
   /*6*/  t_add_test("Bit Walking (RA 32Bit)",    walk32_RA);
   /*7*/  t_add_test("Bit Walking (RD/RA 32Bit)", walk32_RD_RA);
   /*8*/  t_add_test("Multiple Read",             multiple_read);
   /*9*/  t_add_test("Blocks Move (4K)",          move4);
   /*10*/ t_add_test("Blocks Move (64K)",         move64);
   /*11*/ t_add_test("Blocks Move (512K)",        move512);
   /*12*/ t_add_test("Blocks Move (1M)",          move1024);
   
   
   if ( test_mode == MODE_FAST )
   {
      t_skip_test(3);
      t_skip_test(5);
      t_skip_test(6);
      t_skip_test(10);
      t_skip_test(11);
      t_skip_test(12);
   }
   
   if ( test_mode == MODE_HARD )
   {
      t_skip_test(6);
      t_skip_test(8);
      t_skip_test(9);
      t_skip_test(10);
      t_skip_test(11);
      t_skip_test(12);
   }
   
   if ( test_mode == MODE_STAB )
   {
      t_skip_test(0);
      t_skip_test(1);
      t_skip_test(2);
      t_skip_test(3);
      t_skip_test(4);
      t_skip_test(5);
   }
   
   // testing... //
   while ( 1 )
      run_test();
}
