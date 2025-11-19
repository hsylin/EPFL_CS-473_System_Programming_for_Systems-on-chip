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


//#define __REALLY_FAST__
#define BURST_SIZE 255

int main() 
{
   volatile unsigned int *vga = (unsigned int *) 0X50000020;
   volatile unsigned int *dma = (unsigned int *) 0x50000040;
   //[0 ~ 255] spm_write_buffer
   //[256~510] spm_dma_buffer
   volatile unsigned int *spm = (unsigned int *) 0xC0000000;  
   volatile unsigned int *spm_write_buffer = (unsigned int *) 0xC0000000;     
   volatile unsigned int *spm_dma_buffer = &spm_write_buffer[256];
    
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
   dma[SPM_ADDRESS_ID] = swap_u32((unsigned int) &spm_write_buffer[0]);
   dma[TRANSFER_SIZE_ID] = swap_u32(256);

   /* Clear screen */
   // for (int i = 0 ; i < SCREEN_WIDTH*SCREEN_HEIGHT ; i++) frameBuffer[i]=0;

   /* If we clear the framebuffer with the CPU like this:
   *   for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) frameBuffer[i] = 0;
   * then D-cache will contain lots of dirty lines whose value is 0.
    * Later, the DMA updates frameBuffer directly in main memory, but the CPU cache
    * does not see those writes. A final dcache_flush() would then write the stale
    * zeros from D-cache back to memory and overwrite the pixels written by the DMA.
    */




   perf_start();
   
   #ifdef __REALLY_FAST__
   int color = (2<<16) | N_MAX;
   asm volatile ("l.nios_crc r0,%[in1],%[in2],0x21"::[in1]"r"(color),[in2]"r"(delta));
   fxpt_4_28 cy = CY_0;
   for (int k = 0 ; k < SCREEN_HEIGHT ; k++) 
   {
     pixel = spm_write_buffer; //[0 ~ 255]
     fxpt_4_28 cx = CX_0;
     for (int i = 0 ; i < SCREEN_WIDTH ; i+=2) 
     {
       asm volatile ("l.nios_rrr %[out1],%[in1],%[in2],0x20":[out1]"=r"(color):[in1]"r"(cx),[in2]"r"(cy));
       *(pixel++) = color;
       cx += delta << 1;
     }
     
     // with two buffers, we don't have to wait until dma-transfer is done
     // while dma is transferrring the data to the main memory, we will
     // already be calculating the second write buffer
     
     // [0 ~ 255] spm_write_buffer is now filled
     // so we swap spm_write_buffer with spm_dma_buffer
     // spm_write_buffer will then be [256 ~ 510] 
     volatile unsigned int *tmp = spm_write_buffer;
     spm_write_buffer = spm_dma_buffer;
     // spm_dma_buffer will then be [0 ~ 255] 
     spm_dma_buffer = tmp;
      
     while (swap_u32(dma[START_STATUS_ID]) & DMA_BUSY_BIT);
      
      // since the spm_dma_buffer address has been changed
      // from [256~510] to [0~255] (which is where the pixels has be written)
      // OR
      // from [0~255] to [256~510] (which is where the pixels has be written)
      // We therefore need to reset the dma[SPM_ADDRESS_ID] to the address
      // that we like to read the written line of pixels from
      dma[SPM_ADDRESS_ID] = swap_u32((unsigned int) &spm_dma_buffer[0]);
      dma[MEMORY_ADDRESS_ID] = swap_u32( (unsigned int) &frameBuffer[k * SCREEN_WIDTH] );
      dma[START_STATUS_ID]   = swap_u32(DMA_FROM_SPM_TO_MEM | BURST_SIZE);
      cy += delta; 
   }
   while (swap_u32(dma[START_STATUS_ID]) & DMA_BUSY_BIT);





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