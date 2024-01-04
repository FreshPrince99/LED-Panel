#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO

#define IOPORT GPIOA
#define JOYSTICK_A_PORT GPIOA
#define JOYSTICK_B_PORT GPIOC
#define ADC_REG ADC1

#define BUTTON GPIO4
#define LED_ONBOARD GPIO5
#define LED1 GPIO6


void set(int pin);
void clear(int pin);
void clock(void);
void setup(void);
void resetrow(void);

// Function to perform a clock pulse for each binary digit
void performClockPulses(uint32_t data) {
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);

    // Configure GPIO pins
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7 | GPIO8 | GPIO6);
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2 | GPIO3 | GPIO4 | GPIO5);

    // Iterate through each bit
    for (int i = 0; i < 32; i++) {
        // Set the input pin to the current bit value
        if ((data >> i) & 1) {
            gpio_set(GPIOC, GPIO6);
        } else {
            gpio_clear(GPIOC, GPIO6);
        }

        // Iterate through each row selection pin
        for (int row = 0; row < 32; row++) {
            // Set the current row selection pin
            if (row == i % 32) {
                gpio_set(GPIOD, GPIO2 | GPIO3 | GPIO4 | GPIO5);
            } else {
                gpio_clear(GPIOD, GPIO2 | GPIO3 | GPIO4 | GPIO5);
            }
        }

        // Simulate a clock pulse by toggling the clock pin
        gpio_set(GPIOC, GPIO7);
        gpio_clear(GPIOC, GPIO7);

        // Simulate a latch pulse (you may need to customize this based on your specific requirements)
        gpio_set(GPIOC, GPIO8);
        gpio_clear(GPIOC, GPIO8);

        // Simulate a delay (you may need to customize this based on your specific requirements)
        for (volatile int j = 0; j < 100000; j++) {}
    }
}

void pulse(int pin){ //Performs input and a clock pulse
    gpio_set(GPIOC, pin);

    gpio_set(GPIOC, GPIO7);
    return 0;
}

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
