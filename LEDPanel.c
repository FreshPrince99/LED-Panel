#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO

void set(int pin);
void clear(int pin);
void clock(void);
void setup(void);
void resetrow(void);

void set(int pin){
  gpio_clear(GPIOC, GPIO7);
  gpio_set(GPIOC, pin);
  gpio_set(GPIOC, GPIO7);
}

void clear(int pin){
  gpio_clear(GPIOC, GPIO7);
  gpio_clear(GPIOC, pin);
  gpio_set(GPIOC, GPIO7);
}

void clock(void){
  gpio_clear(GPIOC, GPIO7);
  gpio_set(GPIOC, GPIO7);
}

void resetrow(void){
  clear(GPIO2);
  clear(GPIO3);
  clear(GPIO4);
  clear(GPIO5);
}

void setup(void){
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8); 
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO7);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO6);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO5);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO4);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO3);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO2);
}

int main(void) {
  
  setup();
  
  gpio_clear(GPIOC, GPIO8);
  clear(GPIO2);
  
  for (int i=0; i<2; i++){
    set(GPIO6);
  }
  gpio_set(GPIOC, GPIO8);
  
  
  
}