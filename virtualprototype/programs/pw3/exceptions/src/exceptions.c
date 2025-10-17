#include <stdio.h>
#include <perf.h>
#include <swap.h>
#include <cache.h>
#include "../support/include/exception.h"

exception_handler_t my_bus_error_handler(){
   puts("hope we are done: bus error");
}

exception_handler_t sys_call_example_2(){
   puts("system call example 2");
}



int main () {
   /* enable the caches */
   icache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_2K | CACHE_REPLACE_FIFO );
   dcache_write_cfg( CACHE_FOUR_WAY | CACHE_SIZE_2K | CACHE_REPLACE_LRU | CACHE_WRITE_BACK );
   icache_enable(1);
   dcache_enable(1);
   /* initialize the performance counters */
   perf_init();


   // 2.4 Exceptions and bios
   _vectors[1] = my_bus_error_handler(); 

   int *addr = (int*) 0x00000004;
   *addr = 100;
   int data;
   asm volatile ("l.lwz %[d], 0(%[s])" : [d]"=r"(data) :[s]"r"(addr));
   printf("data: %d\n", data);

   // 2.5 System calls
   //  _vectors[2] = sys_call_example_2(); 
   // SYSCALL(2);

   printf("Hello, world from %s (%s:%2d)!\n", __func__, __FILE__, __LINE__);

   printf("no swap: 0x%04x, swap_u16: 0x%04x\n", 0xDEAD, swap_u16(0xDEAD));
   printf("no swap: 0x%08x, swap_u32: 0x%08x\n", 0xDEADBEEF, swap_u32(0xDEADBEEF));

   perf_start();

   while (1) {
    perf_print_time(PERF_COUNTER_RUNTIME, "runtime");
    perf_print_cycles(PERF_COUNTER_RUNTIME, "runtime");
    
       
    for (volatile int i = 0; i < 1000000; ++i) ;
   }
   perf_stop();

   return 0;
}
