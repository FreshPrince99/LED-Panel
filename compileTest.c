#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO
#include <stdio.h>
#include "libopencm3/stm32/adc.h" //Needed to convert analogue signals to digital

#define IOPORT GPIOA
#define JOYSTICK_A_PORT GPIOA
#define JOYSTICK_B_PORT GPIOC
#define ADC_REG ADC1

#define BUTTON GPIO4
#define LED_ONBOARD GPIO5
#define LED1 GPIO6

int clear_method(void){
    int i;
    
    for(i=0;i<192;i++){
    gpio_clear(GPIOC, GPIO7);  

    gpio_clear(GPIOC, GPIO6);     //Sets latch pin


    gpio_set(GPIOC, GPIO7);
    }

    return 0;
}
int pulse(void){
    gpio_set(GPIOC, GPIO6);

    gpio_set(GPIOC, GPIO7);
    return 0;
}

int clock_loop(void){
    clear_method();
    int i;
    for(i=0;i<96;i++){
    gpio_clear(GPIOC, GPIO7);  

    // gpio_clear(GPIOC, GPIO6);     //Sets latch pin

    pulse();
    }
    return 0;
}

int push_color(int rowno){
    switch(rowno){
        case 2:
            gpio_set(GPIOC, GPIO2);
            break;
        case 3:
            gpio_set(GPIOC, GPIO3);
            break;
        case 4:
            gpio_set(GPIOC, GPIO4);
            break;
        case 5:
            gpio_set(GPIOC, GPIO5);
            break;
    }

    int i;
    for(i=0;i<6;i++) {
        clock_loop();
    }

    return 0;
}



int main(void){

    rcc_periph_clock_enable(RCC_ADC12); //Enable clock for ADC registers 1 and 2

    adc_power_off(ADC1);  //Turn off ADC register 1 whist we set it up

    adc_set_clk_prescale(ADC1, ADC_CCR_CKMODE_DIV1);  //Setup a scaling, none is fine for this
    adc_disable_external_trigger_regular(ADC1);   //We don't need to externally trigger the register...
    adc_set_right_aligned(ADC1);  //Make sure it is right aligned to get more usable values
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_61DOT5CYC);  //Set up sample time
    adc_set_resolution(ADC1, ADC_CFGR1_RES_12_BIT);  //Get a good resolution

    adc_power_on(ADC1);  //Finished setup, turn on ADC register 1

    uint8_t channelArray = {1};  //Define a channel that we want to look at
    adc_set_regular_sequence(ADC1, 1, channelArray);  //Set up the channel
    adc_start_conversion_regular(ADC1);  //Start converting the analogue signal

    while(!(adc_eoc(ADC1)));  //Wait until the register is ready to read data

    uint32_t value = adc_read_regular(ADC1);  //Read the value from the register and channel

    rcc_periph_clock_enable(RCC_GPIOC); //Enable clock for ADC registers 1 and 2
    

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO2);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO3);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO4);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO5);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO6);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO7);   //GPIO Port Name, GPIO Pin Driver Type, GPIO Pin Speed, GPIO Pin Number

    


    gpio_set(GPIOC, GPIO8);     //Sets latch pin

    gpio_clear(GPIOC, GPIO2);  //Clear selectied rows
    gpio_clear(GPIOC, GPIO3);
    gpio_clear(GPIOC, GPIO4);
    gpio_clear(GPIOC, GPIO5);

    push_color(5);
    

    // gpio_clear(GPIOC, GPIO2);
    // // while (1){
    //     // gpio_set(GPIOC, GPIO2);
    //     // gpio_set(GPIOC, GPIO3);
    // gpio_set(GPIOC, GPIO3);
    //     // gpio_set(GPIOC, GPIO5);
    //     // gpio_clear(GPIOC, GPIO2);  //Clear selectied rows
    //     // gpio_clear(GPIOC, GPIO3);
    // // gpio_clear(GPIOC, GPIO3);
    //     // gpio_clear(GPIOC, GPIO5);
    // // }
    // // gpio_set(GPIOC, GPIO3);
    
    


    return 0;
  
}
