#include <stdio.h>
#include "../support/include/perf.h"
#include "../support/include/swap.h"
#include "../support/include/cache.h"
#include "../support/include/exception.h"
#include <spr.h>
#include <vga.h>

#define TASK2_5_2     0x0000u
#define TASK2_5_3     0x0001u
#define TASK2_5_4     0x00E0u

void my_bus_error_handler(void) 
{
    puts("my bus error !");
}
/*
void my_system_call_handler(void) 
{
   uint32_t epc = SPR_READ(SPR_EPC);
   volatile uint32_t *p = (uint32_t *)epc;
   uint32_t inst_prev    = *(p - 1);
   uint16_t sysnum = inst_prev & 0xFFFF;
   printf("syscall number: 0x%04x \n",sysnum);

   if(sysnum == TASK2_5_2)
   {
      printf("0x%08x \n",SPR_READ(SPR_EPC));
   }
   else if(sysnum == TASK2_5_3)
   {
      uint32_t inst_epc     = *p;
      printf("EPC=0x%08x  [EPC]=0x%08x  [EPC-4]=0x%08x\n",
           epc, inst_epc, inst_prev);
   }
   else if(sysnum == TASK2_5_4)
   {
      vga_clear();
   }
   else 
   {
      printf("[syscall] unexpected inst_prev=0x%08X \n", inst_prev);
   }
}
*/

// Task 2.5.6 
void system_call_handler() 
{
   uint32_t epc = SPR_READ(SPR_EPC);
   volatile uint32_t *p = (uint32_t *)epc;
   uint32_t inst_prev    = *(p - 1);
   uint16_t sysnum = inst_prev & 0xFFFF;
   printf("syscall number: 0x%04x \n",sysnum);

   if(sysnum == TASK2_5_2)
   {
      printf("0x%08x \n",SPR_READ(SPR_EPC));
   }
   else if(sysnum == TASK2_5_3)
   {
      uint32_t inst_epc     = *p;
      printf("EPC=0x%08x  [EPC]=0x%08x  [EPC-4]=0x%08x\n",
           epc, inst_epc, inst_prev);
   }
   else if(sysnum == TASK2_5_4)
   {
      vga_clear();
   }
   else 
   {
      printf("[syscall] unexpected inst_prev=0x%08X \n", inst_prev);
   }
}

int main () 
{
   /* enable the caches */
   icache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_2K | CACHE_REPLACE_FIFO );
   dcache_write_cfg( CACHE_FOUR_WAY | CACHE_SIZE_2K | CACHE_REPLACE_LRU | CACHE_WRITE_BACK );
   icache_enable(1);
   dcache_enable(1);

   // Task 2.4 Exception vectors
   _vectors[EXCEPTION_BUS_ERROR] = my_bus_error_handler;

   // Task 2.5 System calls
   // _vectors[EXCEPTION_SYSTEM_CALL] = my_system_call_handler; 
   
   /* 
   Task 2.4 Exception vectors
   int *addr = (int*) 0x00000004;

   //without bus error
   *addr = 100;

   //with bus error
   *addr = 101;

   int data = 5;
   asm volatile ("l.lwz %[d], 0(%[s])" : [d]"=r"(data) :[s]"r"(addr));
   printf("data: %d\n", data);
   */
 
   printf("Task 2.5.2 \n");
   SYSCALL(0x0000);

   printf("Task 2.5.3 \n");
   SYSCALL(0x0001);

   printf("Task 2.5.4 \n");
   SYSCALL(0x00E0);

   printf("unexpected system call number \n");
   SYSCALL(0x00AA);

   return 0;
}