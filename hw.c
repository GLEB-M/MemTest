#include "hw.h"
#include "screen.h"

ulong cpu_type = 0;
ulong cpu_temp_sensor = 0;

char *cpus[] = {"Unknown", "Intel", "AMD"}; 

struct s_mem_dev *mem_devs[MAX_DMI_MEMDEVS];
int mem_devs_count = 0;


char *get_tstruct_string( struct s_dmi_header *header, int n )
{
   if (n<1)
      return 0;
   char * a = (char *)header + header->length;
   n--;
   do
   {
      if (!*a)
         n--;
      if (!n && *a)
         return a;
      a++;
   }
   while (!(*a==0 && *(a-1)==0));
   return 0;
}

ulong dmi_init( )
{
   char *dmi = (char *)0xF0000;
   ulong found = 0;
   
   struct s_dmi_entry *entry;
   
   while ( (ulong)dmi < 0x100000 )
   {
      if ( *dmi == '_' && *(dmi + 1) == 'S' && *(dmi + 2) == 'M' && *(dmi + 3) == '_')
      {
         found = 1;
         break;
      }
      dmi += 16;
   }
   
   if ( !found )
      //fatal_error("No DMI!");
      return 0;
   
   entry = (struct s_dmi_entry *)dmi;
   
   uchar checksum = 0;
   ulong len = (ulong)dmi + entry->length;
   while ( (ulong)dmi < len )
      checksum += *dmi++;
   
   if ( checksum )
      //fatal_error("Bad DMI Checksum!");
      return 0;
   
   if ( entry->majorversion < 2 && entry->minorversion < 1 )
       //fatal_error("Bad DMI Version!");
      return 0;
   
   
   len = entry->tableaddress + entry->tablelength;
   dmi = (char *)entry->tableaddress;
   while ( (ulong)dmi < len )
   {
      struct s_dmi_header *header = (struct s_dmi_header *)dmi;
      
      if ( header->type == 17 )
         if ( mem_devs_count < MAX_DMI_MEMDEVS )
            mem_devs[mem_devs_count++] = (struct s_mem_dev *)dmi;
      
      // Need fix (SMBIOS/DDR3)
      /*if ( header->type == 20  )
         md_maps[md_maps_count++] = (struct s_md_map *)dmi;*/
      
      dmi += header->length;
      while( !(*dmi == 0  && *(dmi + 1) == 0 ) )
         dmi++;
      dmi += 2;
   }
   
   ulong i;
   ulong modules = 0;
   //ulong y = 2;
   for( i = 0; i < mem_devs_count; i++ )
   {
      //print_str(" - ");
      // size
      if ( mem_devs[i]->size == 0 ) // empty slot
      {
         //print_str("----");
      }
      else if ( mem_devs[i]->size == 0xFFFF ) // Unknown size
      {
         //print_str("????");
      }
      else
      {
         // slot //
         printf_xy(12, 4 + modules, "%s - ", get_tstruct_string(&(mem_devs[i]->header), mem_devs[i]->dev_locator));
         
         // size //
         ulong size_in_mb = 0x7FFF & mem_devs[i]->size; // reset bit 15
         if ( mem_devs[i]->size & 0x8000 ) // size in KB 
            size_in_mb = size_in_mb << 10;
         print_dec(size_in_mb);
         print_str("M\n");
         
         modules++;
      }
   }
   
   //print_str("\n");
   
   if ( !modules )
      return 0;
   
   return 1;
}

ulong get_cpu( )
{
   volatile ulong _ebx, _edx, _ecx;
   
   __asm volatile("mov eax, 0x0\n\
               cpuid\n\
               mov dword ptr [%0], ebx\n\
               mov dword ptr [%1], edx\n\
               mov dword ptr [%2], ecx\n\
               " :: "m"(_ebx), "m"(_edx), "m"(_ecx));
               
   if ( _ebx == 0x756E6547 && _edx == 0x49656E69 && _ecx == 0x6C65746E )
      return CPU_INTEL;
   
   if ( _ebx == 0x68747541 && _edx == 0x69746E65 && _ecx == 0x444D4163 )
      return CPU_AMD;
               
   return CPU_UNKNOWN;
}

char *get_cpu_vendor( )
{
   return cpus[get_cpu()];
}


void get_cpu_model( char *buffer )
{
   char name[50];
   name[0] = 0;

   char *n = (char *)name;
   
   __asm volatile("mov eax, 0x80000000\n\
               cpuid\n\
               cmp eax, 0x80000004\n\
               jb not_supported\n\
               mov edi, dword ptr [%0]\n\
               mov eax, 0x80000002\n\
               cpuid\n\
               mov dword ptr[edi], eax\n\
               mov dword ptr[edi + 4], ebx\n\
               mov dword ptr[edi + 8], ecx\n\
               mov dword ptr[edi + 12], edx\n\
               add edi, 16\n\
               mov eax, 0x80000003\n\
               cpuid\n\
               mov dword ptr[edi], eax\n\
               mov dword ptr[edi + 4], ebx\n\
               mov dword ptr[edi + 8], ecx\n\
               mov dword ptr[edi + 12], edx\n\
               add edi, 16\n\
               mov eax, 0x80000004\n\
               cpuid\n\
               mov dword ptr[edi], eax\n\
               mov dword ptr[edi + 4], ebx\n\
               mov dword ptr[edi + 8], ecx\n\
               mov dword ptr[edi + 12], edx\n\
               add edi, 16\n\
               not_supported:\n\
               " :: "m"(n));
               
   // delete spaces //           
   if ( *n )
   {
      int i, n = 0, l = ' ';
      for ( i = 0; name[i]; ++i )
         if ( !(name[i] == ' ' && l == ' ') )
            buffer[n++] = l = name[i];
         buffer[(l != ' ') ? n : n - 1] = '\0';
   }           
}

ulong check_dts_intel( )
{
   volatile ulong _eax;
   __asm volatile("mov eax, 0x6\ncpuid\nmov dword ptr [%0], eax\n" :: "m"(_eax));
   return ( _eax & 0x00000001 );
}

ulong check_dts_amd( )
{
   // stub //
   return 0;
}

ulong get_cpu_heat_intel( ulong *overheat )
{
   volatile ulong _eax;
   ulong heat = 0;
   
   if ( !cpu_temp_sensor )
      return 0;
   
   __asm volatile("xor eax, eax\n\
               xor edx, edx\n\
               mov   ecx, 0x0000019C\n\
               rdmsr\n\
               mov dword ptr [%0], eax\n\
               " :: "m"(_eax));
               
   //if ( _eax & 0x00000020 ) // Critical Temp (bit 5)
   // *overheat = 1;
      
   if ( _eax & 0x80000000 ) // Digital Readout (bit 31)
   {        
      ulong dt = (_eax >> 16) & 0x0000007F;
      heat = (ulong)(100.0f * ((127.0f - dt) / 127.0f));
      
      if ( heat > 100 ) // DTS Incorrect????
         heat = 100;
   }
   
   return heat;
}

ulong get_cpu_heat_amd( ulong *overheat )
{
   // stub //
   return 0;
}

ulong get_cpu_heat( ulong *overheat )
{
   if ( cpu_type == CPU_INTEL )
      return get_cpu_heat_intel(overheat);
   
   if ( cpu_type == CPU_AMD )
      return get_cpu_heat_amd(overheat);
   
   return 0; // unknown cpu
}

void cpu_init( )
{
   cpu_type = get_cpu();
   
   if ( cpu_type == CPU_INTEL )
      cpu_temp_sensor = check_dts_intel();
   
   if ( cpu_type == CPU_AMD )
      cpu_temp_sensor = check_dts_amd();
}
