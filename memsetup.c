#include "memsetup.h"
#include "screen.h"

struct  s_820_map bios_mem_map[E820_MAX];
ulong   bios_mem_map_max;

struct  s_pages_map pages_map[E820_MAX];
ulong   pages_map_max;

ulong64 total_pages;    // total pages count (size of available memory)
ulong64 reserved_mem;   // reserved by hardware
ulong   base_limit;


struct  change_member change_point_list[2 * E820_MAX];
struct  change_member *change_point[2 * E820_MAX];
struct  s_820_map *overlap_list[E820_MAX];
struct  s_820_map biosmap[E820_MAX];

int sanitize_e820_map( struct s_820_map *orig_map, struct s_820_map *new_bios, short old_nr )
{
   struct change_member *change_tmp;
   ulong current_type, last_type;
   ulong64 last_addr;
   int chgidx, still_changing;
   int overlap_entries;
   int new_bios_entry;
   int i;

   /* First make a copy of the map */
   for ( i = 0; i < old_nr; i++ ) 
   {
      biosmap[i].addr = orig_map[i].addr;
      biosmap[i].size = orig_map[i].size;
      biosmap[i].type = orig_map[i].type;
   }

   /* bail out if we find any unreasonable addresses in bios map */
   for ( i = 0; i < old_nr; i++ )
   {
      if ( biosmap[i].addr + biosmap[i].size < biosmap[i].addr )
         return 0;
   }

   /* create pointers for initial change-point information (for sorting) */
   for ( i=0; i < 2 * old_nr; i++ )
      change_point[i] = &change_point_list[i];

   /* record all known change-points (starting and ending addresses) */
   chgidx = 0;
   for ( i = 0; i < old_nr; i++ )
   {
      change_point[chgidx]->addr = biosmap[i].addr;
      change_point[chgidx++]->pbios = &biosmap[i];
      change_point[chgidx]->addr = biosmap[i].addr + biosmap[i].size;
      change_point[chgidx++]->pbios = &biosmap[i];
   }

   /* sort change-point list by memory addresses (low -> high) */
   still_changing = 1;
   while ( still_changing )   
   {
      still_changing = 0;
      for ( i = 1; i < 2 * old_nr; i++ ) 
      {
         /* if <current_addr> > <last_addr>, swap */
         /* or, if current=<start_addr> & last=<end_addr>, swap */
         if ( (change_point[i]->addr < change_point[i-1]->addr) ||
            ((change_point[i]->addr == change_point[i-1]->addr) &&
             (change_point[i]->addr == change_point[i]->pbios->addr) &&
             (change_point[i-1]->addr != change_point[i-1]->pbios->addr))
            )
         {
            change_tmp = change_point[i];
            change_point[i] = change_point[i-1];
            change_point[i-1] = change_tmp;
            still_changing=1;
         }
      }
   }

   /* create a new bios memory map, removing overlaps */
   overlap_entries = 0;   /* number of entries in the overlap table */
   new_bios_entry = 0;    /* index for creating new bios map entries */
   last_type = 0;         /* start with undefined memory type */
   last_addr = 0;         /* start with 0 as last starting address */
   /* loop through change-points, determining affect on the new bios map */
   for ( chgidx = 0; chgidx < 2 * old_nr; chgidx++ )
   {
      /* keep track of all overlapping bios entries */
      if ( change_point[chgidx]->addr == change_point[chgidx]->pbios->addr )
      {
         /* add map entry to overlap list (> 1 entry implies an overlap) */
         overlap_list[overlap_entries++]=change_point[chgidx]->pbios;
      }
      else
      {
         /* remove entry from list (order independent, so swap with last) */
         for ( i = 0; i < overlap_entries; i++ )
         {
            if ( overlap_list[i] == change_point[chgidx]->pbios )
               overlap_list[i] = overlap_list[overlap_entries-1];
         }
         overlap_entries--;
      }
      /* if there are overlapping entries, decide which "type" to use */
      /* (larger value takes precedence -- 1=usable, 2,3,4,4+=unusable) */
      current_type = 0;
      for ( i = 0; i < overlap_entries; i++ )
         if ( overlap_list[i]->type > current_type )
            current_type = overlap_list[i]->type;
      /* continue building up new bios map based on this information */
      if ( current_type != last_type ) 
      {
         if ( last_type != 0 )    
         {
            new_bios[new_bios_entry].size = change_point[chgidx]->addr - last_addr;
            /* move forward only if the new size was non-zero */
            if ( new_bios[new_bios_entry].size != 0 )
               if ( ++new_bios_entry >= E820_MAX )
                  break;   /* no more space left for new bios entries */
         }
         if ( current_type != 0 )   
         {
            new_bios[new_bios_entry].addr = change_point[chgidx]->addr;
            new_bios[new_bios_entry].type = current_type;
            last_addr=change_point[chgidx]->addr;
         }
         last_type = current_type;
      }
   }
   return new_bios_entry;
}


void build_pages( ulong address )
{
   ulong i;
   
   // fill PDPT //
   ulong64 *pdpt = (ulong64 *)address;//0x100000;
   ulong dirs = address + 0x1000; // 1M + 4K
   
   for ( i = 0; i < 4; ++i )
   {
      *pdpt++ = dirs | 1;
      dirs += 4096;
   }
   
   // FILL 4 PDs //
   ulong64 *pd = (ulong64 *)(address + 0x1000);//0x101000;
   ulong tables = address + 0x5000;  // 1M + 20K
   
   for ( i = 0; i < 2048; ++i )
   {
      *pd++ = tables | 7;
      tables += 4096;
   }
   
   // make first 6 MB pages (1 first megabyte + 12KB base pages + 4 MB window pages + ~1MB for temp data ) //
   ulong64 *pages = (ulong64 *)(address + 0x5000);//0x105000;
   ulong64 start_addr = 0x0;
   for ( i = 0; i < BASE_MEM; ++i )
   {
      pages[i] = start_addr | 7;
      start_addr += 4096;
   }
}

void enable_paging( ulong cr3 )
{
   disable_interrupts();
   __asm volatile("\
            mov eax, dword ptr[%0]\n\
            mov cr3, eax\n\
            mov eax, cr4\n\
            or  eax, 0x00000020\n\
            mov cr4, eax\n\
            mov eax, cr0\n\
            or  eax, 0x80000000\n\
            mov cr0, eax\n\
            jmp p_m\n\
            p_m:\
            " :: "m"(cr3));
   enable_interrupts();
}

void disable_paging( )
{
   disable_interrupts();
   __asm volatile("\
            mov eax, cr0\n\
            and eax, 0x7FFFFFFF\n\
            mov cr0, eax\n\
            mov eax, cr4\n\
            and eax, 0x0000001F\n\
            mov cr4, eax\n\
            jmp f_m\n\
            f_m:\
            ");
   enable_interrupts();
}

int check_pae( )
{
   ulong pae = 0;
   __asm volatile("mov eax, 0x1\ncpuid\nmov dword ptr [%0], edx" :: "m"(pae));
   return ( pae & 64 );
}

ulong64 get_phys_addr( ulong virtual_addr )
{
   ulong cr0 = 0; 
   __asm volatile("mov eax, cr0\nmov dword ptr [%0], eax" :: "m"(cr0));
   if ( !(cr0 & 0x80000000) ) // pages off
      return virtual_addr; 
   
   ulong PD_index = (virtual_addr & 0xC0000000) >> 30;
   ulong PT_index = ((virtual_addr & 0x3FE00000) << 2) >> 23;
   ulong P_index  = ((virtual_addr & 0x001FF000) << 11) >> 23;
   ulong P_offset = (virtual_addr & 0x00000FFF);
   
   ulong cr3 = 0; 
   __asm volatile("mov eax, cr3\nmov dword ptr [%0], eax" :: "m"(cr3));
   
   ulong64 *PDPT     = (ulong64 *)(cr3 & 0xFFFFFFE0);
   ulong64 *PD       = (ulong64 *)((PDPT[PD_index] >> 12) << 12);
   ulong64 *PT       = (ulong64 *)((PD[PT_index] >> 12) << 12);
   ulong64 P         = (PT[P_index] >> 12) << 12;
   ulong64 phys_addr = P + P_offset;
   
   return phys_addr;
}

struct s_pages_map *get_pages_map( )
{
   return pages_map;
}
ulong get_pages_map_entries_num( )
{
   return pages_map_max;
}
ulong get_total_pages( )
{
   return total_pages;
}

void mem_init( )
{
   ulong  i;
   ulong  map_max = *((ulong *)(0x8000 + 16));                     // E820 entries count //
   struct s_820_map *mem_map = (struct s_820_map *)(0x8000 + 20);  // E820 entries data  //
   
   // check PAE support //
   if ( !check_pae() )
      //print_str("PAE supported\n");
   //else
      fatal_error("ERROR: No PAE support");
   

   // sanitize E820 result //
   bios_mem_map_max = sanitize_e820_map(mem_map, bios_mem_map, map_max);
   
   /*
   // print e820 map
   print_str("\n");
   for ( i = 0; i < bios_mem_map_max; ++i )
   {
      if ( bios_mem_map[i].type == E820_RAM || bios_mem_map[i].type == E820_ACPI ) {
      
      print_dec(bios_mem_map[i].addr >> 10); 
      print_str(" - ");
      print_dec(bios_mem_map[i].size >> 10); 
      print_str("K\n");
      }
   }
   print_str("\n");
   */
   
   // build memory map //
   
   pages_map[0].addr = 0;     // add First MB //
   pages_map[0].num = 256;
   pages_map_max = 1; 
   
   base_limit = bios_mem_map[0].size; //??????????????????????
   
   for ( i = 0; i < bios_mem_map_max; ++i )
   {
      if ( bios_mem_map[i].addr < 0x100000 ) // ignore first megabyte //
         continue;
      
      if ( bios_mem_map[i].type == E820_RAM || bios_mem_map[i].type == E820_ACPI )
      {
         if ( bios_mem_map[i].addr == 0x100000 )
         {
            pages_map[pages_map_max].addr = 0x100000; // add 5 MB block for pages //
            pages_map[pages_map_max].num = (BASE_MEM - 256);//2048;
            pages_map_max++;
            
            pages_map[pages_map_max].addr = 0x600000;//0x900000;
            pages_map[pages_map_max].num = (bios_mem_map[i].size >> 12) - (BASE_MEM - 256);//2048;
            pages_map_max++;
         }
         else
         {
            pages_map[pages_map_max].addr = bios_mem_map[i].addr;
            pages_map[pages_map_max].num = bios_mem_map[i].size >> 12;
            pages_map_max++;
         }
      }
      else
      {
         if ( bios_mem_map[i].type == E820_NVS )
            reserved_mem += bios_mem_map[i].size;
      }
   }
   
   // calc total pages count //
   total_pages = 0;
   for ( i = 2; i < pages_map_max; ++i )
      total_pages += pages_map[i].num;
   
   // check minimum memory size //
   if ( (total_pages + BASE_MEM) < 4096 ) // 16 MB
      fatal_error("Available memory less than 16M!");
   
   // print available memory //
   printf(" Available: %uM  ", (ulong)((total_pages + BASE_MEM) >> 8));
   
   // print reserved memory //
   printf("Reserved: %u%c  ", (reserved_mem < 1048576) ? (ulong)(reserved_mem >> 10) : (ulong)(reserved_mem >> 20), (ulong)(reserved_mem < 1048576) ? 'K' : 'M');
   
   printf("Base Memory: %uK\n", base_limit >> 10);
      
   /*// print memory map //
   print_str("\nMemory map [Address - Size]:            Memory windows [Size]:\n\n");
   for ( i = 0; i < pages_map_max; ++i )
   {
      char _s = 'K';
      ulong s = pages_map[i].addr >> 10;
      if ( s >= 1024 )
      {
         s = pages_map[i].addr >> 20;
         _s = 'M';
      }
      
      print_dec_ra(s, 8, ' '); 
      print_char(_s);
      print_str(" - ");
      
      _s = 'K';
      s = pages_map[i].num << 2;
      if ( s >= 1024 )
      {
         s = pages_map[i].num >> 8;
         _s = 'M';
      }
      
      print_dec(s); 
      print_char(_s);
      print_str("\n");
   }
   print_str("\n");*/
   
   // show windows //
   mem_get_windows(/*1*/0);
}

ulong mem_get_windows( ulong show )
{
   ulong i, n;
   ulong wnd = 0;
   
   ulong64 pages_max = 0;
   ulong64 window_pages_num = 0; 
   
   for ( i = 2; i < pages_map_max; ++i )
   {
      //unsigned long long addr = pages_map[i].addr;
      for ( n = 0; n < pages_map[i].num; ++n )
      {
         //addr += 4096;
         window_pages_num++;
         pages_max++;
   
         if ( window_pages_num == WINDOW_SIZE || pages_max == (total_pages /*- BASE_MEM*/) )
         {
            /*
            print_str("WINDOW:");
            print_dec(start_addr >> 20);
            print_str("-");
            print_dec(window_pages_num - 2304);
            print_str("\n");
            start_addr = addr;
            */
            if ( show )
            {
               static int pos = 6;
               static int num = 0;
               char _s = 'K';
               ulong s = (window_pages_num) << 2;
               if ( s >= 1024 )
               {
                  s = (window_pages_num ) >> 8;
                  _s = 'M';
               }
               
               print_str_xy(48, pos++, "#");
               print_dec(++num);
               print_str(" - ");
               print_dec(s); 
               print_char(_s);
            }
      
            window_pages_num = 0;
            
            wnd++;
         }
      }
   }
   return wnd;
}

ulong mem_get_base_mem_size( )
{ 
   return base_limit;
}
