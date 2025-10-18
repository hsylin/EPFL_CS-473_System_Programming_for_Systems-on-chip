#include "../support/include/stdio.h"
#include "../support/include/perf.h"
#include "../support/include/swap.h"
#include "../support/include/cache.h"
#include "../support/include/exception.h"
#include "../support/include/spr.h" 
#include "../support/include/vga.h"



#define TASK2_5_2     0x0000u
#define TASK2_5_3     0x0001u
#define TASK2_5_4     0x00E0u

void system_call_handler(void){
   
   uint32_t epc = SPR_READ(SPR_EPC);                    // exception program counter

   volatile uint32_t *p = (uint32_t *)epc;              // exception program counter address
   uint32_t inst_epc = *p;                              // instruction inside the exception program counter
   volatile uint32_t *epc_prev = (uint32_t *)(epc - 4); // previous program counter address
   uint32_t inst_prev = *epc_prev;                      // instruction inside the previous program counter
                                                        // (this is where the instruction that caused the exception is stored)
   uint16_t sys_call_num = inst_prev & 0xFFFF;          // l.sys K, 16 bits + 16 bits, we want to get the K
   printf("syscall num: 0x%04x\n", sys_call_num);
 
   
   if (sys_call_num == TASK2_5_2) {
      printf("EPC: 0x%08x \n", p);
   }
   else if (sys_call_num == TASK2_5_3){
      printf("[EPC: 0x%08x]=0x%08x\n[EPC-4: 0x%08x]=0x%08x\n", p, inst_epc,  epc_prev, inst_prev);
   }
   else if (sys_call_num == TASK2_5_4){
      vga_clear();
   }
   else {
      printf("sys call error\n");
   }
}



int main () {
   /* enable the caches */
   icache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_2K | CACHE_REPLACE_FIFO );
   dcache_write_cfg( CACHE_FOUR_WAY | CACHE_SIZE_2K | CACHE_REPLACE_LRU | CACHE_WRITE_BACK );
   icache_enable(1);
   dcache_enable(1);
   /* initialize the performance counters */
   //perf_init();
   
   
   // 2.4 Exceptions and bios
   _vectors[11] = system_call_handler; 


   // int *addr = (int*) 0x00000004;
   // *addr = 100;
   // int data;
   // asm volatile ("l.lwz %[d], 0(%[s])" : [d]"=r"(data) :[s]"r"(addr));
   // //asm volatile ("l.lwz %[d], 2(r3)" : [d]"=r"(data) );
   // printf("data: %d\n", data);

   // 2.5 System calls
 
   printf("Task 2.5.2 \n");
   SYSCALL(0x0000);

   printf("Task 2.5.3 \n");
   SYSCALL(0x0001);

   printf("Task 2.5.4 \n");
   SYSCALL(0x00E0);

   printf("unexpected system call number \n");
   SYSCALL(0x00AA);


   printf("Hello, world from %s (%s:%2d)!\n", __func__, __FILE__, __LINE__);

   //printf("no swap: 0x%04x, swap_u16: 0x%04x\n", 0xDEAD, swap_u16(0xDEAD));
   //printf("no swap: 0x%08x, swap_u32: 0x%08x\n", 0xDEADBEEF, swap_u32(0xDEADBEEF));

  // perf_start();

   //while (1) {
    //perf_print_time(PERF_COUNTER_RUNTIME, "runtime");
    //perf_print_cycles(PERF_COUNTER_RUNTIME, "runtime");
    
       
   // for (volatile int i = 0; i < 1000000; ++i) ;
   //}
   //perf_stop();

   return 0;
}
