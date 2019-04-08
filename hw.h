#ifndef _HW_H_
#define _HW_H_

#include "common.h"

#define CPU_UNKNOWN     0
#define CPU_INTEL       1
#define CPU_AMD         2

#define MAX_DMI_MEMDEVS 16

struct s_dmi_entry 
{
   uchar  anchor[4];
   uchar  checksum;
   uchar  length;
   uchar  majorversion;
   uchar  minorversion;
   ushort maxstructsize;
   uchar  revision;
   uchar  pad[5];
   uchar  intanchor[5];
   uchar  intchecksum;
   ushort tablelength;
   ulong  tableaddress;
   ushort numstructs;
   uchar  SMBIOSRev;
} __attribute__((packed));

struct s_dmi_header
{
   uchar  type;
   uchar  length;
   ushort handle;
} __attribute__((packed));


struct s_mem_dev 
{
   struct s_dmi_header header;
   ushort pma_handle;
   ushort err_handle;
   ushort tot_width;
   ushort dat_width;
   ushort size;
   uchar  form;
   uchar  set;
   uchar  dev_locator;
   uchar  bank_locator;
   uchar  type;
   ushort typedetail;
   ushort speed;
   uchar  manufacturer;
   uchar  serialnum;
   uchar  asset;
   uchar  partnum;
} __attribute__((packed));

struct s_md_map
{
   struct s_dmi_header header;
   ulong  start;
   ulong  end;
   ushort md_handle;
   ushort mama_handle;
   uchar  row_pos;
   uchar  interl_pos;
   uchar  interl_depth;
   unsigned long long qstart;
   unsigned long long qend;
} __attribute__((packed));


ulong dmi_init( );
char *get_cpu_vendor( );
void get_cpu_model( char *buffer );

ulong get_cpu( );
ulong get_cpu_heat( ulong *overheat );
void cpu_init( );

#endif
