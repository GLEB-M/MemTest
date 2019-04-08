#ifndef _MEMSETUP_H_
#define _MEMSETUP_H_

#include "common.h"


#define E820_MAX           64

#define E820_RAM           1       // usable //
#define E820_RESERVED      2
#define E820_ACPI          3       // usable as RAM once ACPI tables have been read //
#define E820_NVS           4

#define WINDOW_SIZE        0x80000 //131072 //0x80000 // 131072 //1046272 // 2GB //
#define BASE_MEM           1536    // 6MB (1 first MB + 4MB + 14KB pages + ~1MB for temp data)

#define WINDOW_PAGES_OFFSET  0x8000  // 20kb

#define START_ADDRESS_ 0


struct s_820_map
{
   ulong64 addr;
   ulong64 size;
   ulong   type;
} __attribute__((packed)) s_820_map;

struct s_pages_map
{
   ulong64 addr;
   ulong64 num;
};

struct change_member 
{
   struct s_820_map *pbios; /* pointer to original bios entry */
   ulong64          addr;   /* address for this change point */
};


void build_pages( ulong address );
void enable_paging( ulong cr3 );
void disable_paging( );


ulong64              get_phys_addr( ulong virtual_addr );

struct s_pages_map  *get_pages_map( );
ulong                get_pages_map_entries_num( );
ulong                get_total_pages( );

void                 mem_init( );
ulong                mem_get_windows( ulong show );
ulong                mem_get_base_mem_size( );

#endif
