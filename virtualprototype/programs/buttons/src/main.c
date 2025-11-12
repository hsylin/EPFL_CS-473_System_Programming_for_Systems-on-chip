#include <stdio.h>
#include <stdint.h>
#include <cache.h>
#include <vga.h>
#include "../support/include/switches.h"
#include "../support/include/spr.h"
#include <swap.h>
#include <defs.h>
#include <perf.h>

// Constants describing the output device
#define SCREEN_WIDTH 512   //!< screen width
#define SCREEN_HEIGHT 512  //!< screen height
#define JOY_MASK   0x001F  
#define BUTTON_MASK   0x03E0   
// Constants describing the initial view port on the fractal function
const uint32_t FRAC_WIDTH = 0x30000000; 
const uint32_t CX_0 = 0xe0000000;       
const uint32_t CY_0 = 0xe8000000;       
const uint16_t N_MAX = 64;              

// global variables indicating the zoom factor and x- and y- offset for the fractal
uint32_t delta, cxOff, cyOff, redraw;
uint32_t frameBuffer[(SCREEN_WIDTH * SCREEN_HEIGHT)/2];


void drawFractal(uint32_t *frameBuffer) {
  printf("Starting drawing a fractal\n");
  uint32_t color = (2<<16) | N_MAX;
  uint32_t * pixels = frameBuffer;
  asm volatile ("l.nios_crc r0,%[in1],%[in2],0x21"::[in1]"r"(color),[in2]"r"(delta));
  uint32_t cy = CY_0 + cyOff;
  for (int k = 0 ; k < SCREEN_HEIGHT ; k++) {
    uint32_t cx = CX_0 + cxOff;
    for (int i = 0 ; i < SCREEN_WIDTH ; i+=2) {
      asm volatile ("l.nios_rrr %[out1],%[in1],%[in2],0x20":[out1]"=r"(color):[in1]"r"(cx),[in2]"r"(cy));
      *(pixels++) = color;
      cx += delta << 1;
    }
    cy += delta;
  }
  dcache_flush();
  printf("Done\n");
}




int32_t read_bit_position(uint32_t read, uint32_t position){
    if (position > 31){
        return -1;
    }

    

    uint32_t mask = (1 << position);
    uint32_t isolated_bit = read & mask;
    
    // Right-shift the isolated bit back to position 0
    // This gives the actual bit value (0 or 1)
    return (uint32_t)(isolated_bit >> position); 
}


static void  buttons_joystick_handler(void) {
    volatile uint32_t * switches = (uint32_t *)  SWITCHES_BASE_ADDRESS;
  
    uint32_t pressed  = swap_u32(switches[BUTTONS_PRESSED_IRQ_ID]);
    uint32_t events = pressed & BUTTON_MASK;
    if (events) 
    {
        puts("button handler");
        printf("buttons events=0x%04x\n", events >> 5);
    }
    else 
    {
        events = pressed & JOY_MASK ;
        puts("joystick handler");
        printf("buttons events=0x%04x\n", events);
    }
}


static void dipswitch_handler(void) 
{
    volatile uint32_t * switches = (uint32_t *)  SWITCHES_BASE_ADDRESS;
    uint32_t pressed  = swap_u32(switches[DIP_SWITCH_PRESSED_IRQ_ID]);
    uint32_t events = pressed;     
    if (events) 
    {
        puts("dipswitch handler");
        printf("dip events=0x%04x\n", events & 0xFFFF);
    }
}



void external_interrupt_handler(void)
{
   uint32_t picsr = SPR_READ2(2, 0x00004800);


    if (picsr & (1u << 2)) 
    {          
        dipswitch_handler();
    }
    if (picsr & (1u << 3)) 
    {       
        buttons_joystick_handler();
    }
}


int main() {
  icache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_8K );
  dcache_write_cfg( CACHE_DIRECT_MAPPED | CACHE_SIZE_8K | CACHE_WRITE_BACK );
  icache_enable(1);
  dcache_enable(1);
  perf_init();

  // pointer to the PIO
  volatile unsigned int *vga = (unsigned int *) 0X50000020;
  volatile uint32_t * switches = (uint32_t *)  SWITCHES_BASE_ADDRESS;

  vga_clear();

  /* Enable the vga-controller's graphic mode */
  vga[0] = swap_u32(SCREEN_WIDTH);
  vga[1] = swap_u32(SCREEN_HEIGHT);
  vga[3] = swap_u32((unsigned int)&frameBuffer[0]); // disable the vga controller by commenting this line

  delta = FRAC_WIDTH / SCREEN_WIDTH;
  cxOff = 0;
  cyOff = 0;
  redraw = 1;
  uint32_t dip_before = 0;
  uint32_t dip_after = 0;
  uint32_t button_before = 0;
  uint32_t button_after = 0;
  uint32_t joy_before = 0;
  uint32_t joy_after = 0;

  // enable bit2 (interrupt) in supervision register
  uint32_t supervision_value = SPR_READ(17);
  supervision_value = supervision_value | 0x00000004;
  SPR_WRITE(17, supervision_value);

  // when dip_switch pressed -> IRQ bit2 will be raised
  // when joy stick pressed  -> IRQ bit3 will be raised
  // enable bit2 & bi3 of PICMR 
  uint32_t picmr_value = SPR_READ2(0, 0x00004800);
  picmr_value = picmr_value | 0x0000000C;
  SPR_WRITE2(0, 0x00004800, picmr_value);
  
 

  // enable dip_switch 
    dip_after = swap_u32(*(switches + DIP_SWITCH_PRESSED_IRQ_ID));
  dip_after = dip_after | 0x000000FF;
    
   *(switches + DIP_SWITCH_PRESSED_IRQ_ID) = swap_u32(dip_after);

  // enable joy stick 
    button_after = swap_u32(*(switches + BUTTONS_PRESSED_IRQ_ID));
  button_after = button_after | 0x000003FF;

   *(switches + BUTTONS_PRESSED_IRQ_ID) = swap_u32(button_after);




  do {
    if (redraw == 1) {
      redraw = 0;
      drawFractal(frameBuffer);
    }

    // // dip switches (16 bits)
    // // switch1 --> bit0
    // // ~
    // // switch8 --> bit7 
    // dip_after = swap_u32(*(switches + DIP_SWITCH_STATE_ID));

    // if (dip_before != dip_after) {

    //   // sw1
    //   if (read_bit_position(dip_before, 0) != read_bit_position(dip_after, 0)){
    //     printf("dip sw1: %d -> %d\n", read_bit_position(dip_before, 0), read_bit_position(dip_after, 0));  
    //   }
    //   // sw2
    //   if (read_bit_position(dip_before, 1) != read_bit_position(dip_after, 1)){
    //     printf("dip sw2: %d -> %d\n", read_bit_position(dip_before, 1), read_bit_position(dip_after, 1));  
    //   }
    //   // sw3
    //   if (read_bit_position(dip_before, 2) != read_bit_position(dip_after, 2)){
    //     printf("dip sw3: %d -> %d\n", read_bit_position(dip_before, 2), read_bit_position(dip_after, 2));  
    //   }
    //   // sw4
    //   if (read_bit_position(dip_before, 3) != read_bit_position(dip_after, 3)){
    //     printf("dip sw4: %d -> %d\n", read_bit_position(dip_before, 3), read_bit_position(dip_after, 3));  
    //   }
    //   // sw5
    //   if (read_bit_position(dip_before, 4) != read_bit_position(dip_after, 4)){
    //     printf("dip sw5: %d -> %d\n", read_bit_position(dip_before, 4), read_bit_position(dip_after, 4));  
    //   }
    //   // sw6
    //   if (read_bit_position(dip_before, 5) != read_bit_position(dip_after, 5)){
    //     printf("dip sw6: %d -> %d\n", read_bit_position(dip_before, 5), read_bit_position(dip_after, 5));  
    //   }
    //   // sw7
    //   if (read_bit_position(dip_before, 6) != read_bit_position(dip_after, 6)){
    //     printf("dip sw7: %d -> %d\n", read_bit_position(dip_before, 6), read_bit_position(dip_after, 6));  
    //   }
    //   // sw8
    //   if (read_bit_position(dip_before, 7) != read_bit_position(dip_after, 7)){
    //     printf("dip sw8: %d -> %d\n", read_bit_position(dip_before, 7), read_bit_position(dip_after, 7));  
    //   }
      
    //   dip_before = dip_after;
    //   printf("dip changed\n");
    // }
    // //printf("dip nothing changed: %d <=> %d\n", dip_before, dip_after);
    
    // // buttons and joy sticks (16 bits)
    // // joyWest --> bit0
    // // joySouth--> bit1
    // // joyEast --> bit2
    // // joyNorth--> bit3
    // // center  --> bit4 
    // // But1 --> bit5
    // // But2 --> bit6
    // // But3 --> bit7
    // // But4 --> bit8
    // // But5 --> bit9
    // button_after = swap_u32(*(switches + BUTTONS_STATE_ID));
    // if (button_before != button_after) {
        
    //   // joyWest --> bit0
    //   if (read_bit_position(button_before, 0) != read_bit_position(button_after, 0)){
    //     printf("joyWest: %d -> %d\n", read_bit_position(button_before, 0), read_bit_position(button_after, 0));  
    //   }
    //   // joySouth--> bit1
    //   if (read_bit_position(button_before, 1) != read_bit_position(button_after, 1)){
    //     printf("joySouth: %d -> %d\n", read_bit_position(button_before, 1), read_bit_position(button_after, 1));  
    //   }
    //   // joyEast --> bit2
    //   if (read_bit_position(button_before, 2) != read_bit_position(button_after, 2)){
    //     printf("joyEast: %d -> %d\n", read_bit_position(button_before, 2), read_bit_position(button_after, 2));  
    //   }
    //   // joyNorth--> bit3
    //   if (read_bit_position(button_before, 3) != read_bit_position(button_after, 3)){
    //     printf("joyNorth: %d -> %d\n", read_bit_position(button_before, 3), read_bit_position(button_after, 3));  
    //   }
    //   // center  --> bit4 
    //   if (read_bit_position(button_before, 4) != read_bit_position(button_after, 4)){
    //     printf("center: %d -> %d\n", read_bit_position(button_before, 4), read_bit_position(button_after, 4));  
    //   }
    //   // But1 --> bit5
    //   if (read_bit_position(button_before, 5) != read_bit_position(button_after, 5)){
    //     printf("But1: %d -> %d\n", read_bit_position(button_before, 5), read_bit_position(button_after, 5));  
    //   }
    //   // But2 --> bit6
    //   if (read_bit_position(button_before, 6) != read_bit_position(button_after, 6)){
    //     printf("But2: %d -> %d\n", read_bit_position(button_before, 6), read_bit_position(button_after, 6));  
    //   }
    //   // But3 --> bit7
    //   if (read_bit_position(button_before, 7) != read_bit_position(button_after, 7)){
    //     printf("But3: %d -> %d\n", read_bit_position(button_before, 7), read_bit_position(button_after, 7));  
    //   }
    //   // But4 --> bit8
    //   if (read_bit_position(button_before, 8) != read_bit_position(button_after, 8)){
    //     printf("But4: %d -> %d\n", read_bit_position(button_before, 8), read_bit_position(button_after, 8));  
    //   }
    //   // But5 --> bit9
    //   if (read_bit_position(button_before, 9) != read_bit_position(button_after, 9)){
    //     printf("But5: %d -> %d\n", read_bit_position(button_before, 9), read_bit_position(button_after, 9));  
    //   }
      
    //   button_before = button_after;
    //   printf("button/joystick changed\n");
    // }

    //printf("button/joystick nothing changed: %d <=> %d\n", button_before, button_after);

  } while(1);
}