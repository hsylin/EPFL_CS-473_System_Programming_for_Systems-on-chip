#include <stdio.h>
#include <stddef.h>
#include <cache.h>
#include <perf.h>
#include <vga.h>
#include <swap.h>
#include <defs.h>
#include <dma.h>
#include <string.h>
#include "fractal_fxpt.h"

rgb565 frameBuffer[SCREEN_WIDTH*SCREEN_HEIGHT];

#define __REALLY_FAST__
#define BURST_SIZE 255

int main() 
{
   volatile unsigned int *vga = (unsigned int *) 0X50000020;
   volatile unsigned int *dma = (unsigned int *) 0x50000040;
   volatile unsigned int *spm = (unsigned int *) 0xC0000000;

   volatile unsigned int reg, hi;
   volatile int *pixel;


   perf_init();
   perf_set_mask(PERF_COUNTER_0, PERF_ICACHE_NOP_INSERTION_MASK | PERF_STALL_CYCLES_MASK);
   perf_set_mask(PERF_COUNTER_1, PERF_BUS_IDLE_MASK);
   perf_set_mask(PERF_COUNTER_2, PERF_ICACHE_MISS_MASK);
   perf_set_mask(PERF_COUNTER_3, PERF_DCACHE_MISS_MASK);

   vga_clear();
   printf("Starting drawing a fractal\n");
   fxpt_4_28 delta = FRAC_WIDTH / SCREEN_WIDTH;

   /* enable the caches */
   icache_write_cfg( CACHE_FOUR_WAY | CACHE_SIZE_8K | CACHE_REPLACE_LRU );
   dcache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_8K | CACHE_WRITE_BACK);
   icache_enable(1);
   dcache_enable(1);

   /* Enable the vga-controller's graphic mode */
   vga[0] = swap_u32(SCREEN_WIDTH);
   vga[1] = swap_u32(SCREEN_HEIGHT);
   vga[3] = swap_u32( (unsigned int) &frameBuffer[0] );
   
   /* Enable DMA from SPM to MEM */
   dma[SPM_ADDRESS_ID] = swap_u32((unsigned int) &spm[0]);
   dma[TRANSFER_SIZE_ID] = swap_u32(256);

   /* Clear screen */
   //for (int i = 0 ; i < SCREEN_WIDTH*SCREEN_HEIGHT ; i++) frameBuffer[i]=0;

   perf_start();
   
   #ifdef __REALLY_FAST__
   int color = (2<<16) | N_MAX;
   asm volatile ("l.nios_crc r0,%[in1],%[in2],0x21"::[in1]"r"(color),[in2]"r"(delta));
   fxpt_4_28 cy = CY_0;
   for (int k = 0 ; k < SCREEN_HEIGHT ; k++) 
   {
     pixel = spm; 
     fxpt_4_28 cx = CX_0;
     for (int i = 0 ; i < SCREEN_WIDTH ; i+=2) 
     {
       asm volatile ("l.nios_rrr %[out1],%[in1],%[in2],0x20":[out1]"=r"(color):[in1]"r"(cx),[in2]"r"(cy));
       *(pixel++) = color;
       cx += delta << 1;
     }

     while (swap_u32(dma[START_STATUS_ID]) & DMA_BUSY_BIT);
      
      dma[MEMORY_ADDRESS_ID] = swap_u32( (unsigned int) &frameBuffer[k * SCREEN_WIDTH] );
      dma[START_STATUS_ID]   = swap_u32(DMA_FROM_SPM_TO_MEM | BURST_SIZE);
      cy += delta;
   }





#else
    draw_fractal(frameBuffer, dma, spm,
                 SCREEN_WIDTH, SCREEN_HEIGHT,
                 &calc_mandelbrot_point_soft, &iter_to_colour,
                 CX_0, CY_0, delta, N_MAX);
#endif
   dcache_flush();
   asm volatile ("l.lwz %[out1],0(%[in1])":[out1]"=r"(pixel):[in1]"r"(frameBuffer)); // dummy instruction to wait for the flush to be finished
   perf_stop();

   printf("Done\n");
   perf_print_cycles( PERF_COUNTER_2 , "I$ misses" );
   perf_print_cycles( PERF_COUNTER_3 , "D$ misses" );
   perf_print_cycles( PERF_COUNTER_0 , "Stall cycles" );
   perf_print_cycles( PERF_COUNTER_1 , "Bus idle cycles" );
   perf_print_cycles( PERF_COUNTER_RUNTIME , "Runtime cycles" );
}