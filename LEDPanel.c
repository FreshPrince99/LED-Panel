#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO
#include "libopencm3/stm32/adc.h" //Needed to convert analogue signals to digital

#define IOPORT GPIOA
#define JOYSTICK_A_PORT GPIOA
#define JOYSTICK_B_PORT GPIOC
#define ADC_REG ADC1
#define BUTTON GPIO4 // --> is this required?? buttons are usually only for digital inputs we are only dealing with analog inputs such as the joysticks
// Define GPIO pins
#define GPIO_LED    GPIOC
#define GPIO_CLOCK  GPIOC
#define GPIO_LATCH  GPIOC
#define GPIO_ROWS   GPIOC
#define INPUT_PIN  GPIO6
#define CLOCK_PIN  GPIO7
#define LATCH_PIN  GPIO8
#define ROWS_MASK  (GPIO2 | GPIO3 | GPIO4 | GPIO5)

// Constants defining the relevant control bits (D12-D15) that are appended to
// both the X and Y-coordinates to facilitate an appropriate transmission to the DAC
#define portA 0xc000
#define portB 0x4000

#define PANEL_WIDTH  16
#define PANEL_HEIGHT 32
#define symbolSegmentLength 5
#define symbolSpacing 4
#define scoresOffset 3
#define scoreBeforeReset (winningScore + 3)
#define batWidth 2
#define batSpacing 2
#define maxPaddleValue 105
#define minPaddleValue 555  // both have to be tested
#define batLength 4
#define ballRadius 2
#define winningScore 5
#define initialSpeed 5
#define speedIncrement 1
#define maxSpeed (batWidth + batSpacing - 3)

// Initialisation and Declaration of Global variables
int ballSpeed;                          // Speed of the ball
int winner;                             // Flag indicating the winner of the match
int leftBatOffset, rightBatOffset;      // Vertical displacement of the bat from the bottom of the zone
int ballX, ballY;                       // The coordinates of the ball
int ballControlX = 1, ballControlY = 1; // Velocity control variables for determining the balls direction
int leftScore, rightScore;              // Respective scores of players 1 and 2
uint32_t leftTop, leftDown, rightTop, rightDown; //Directions for both the paddles

// Function to perform a clock pulse for each binary digit (Mentioned on the coursework website)
void performClockPulses(uint32_t data) {
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);

    // Configure GPIO pins
    setup();

    // Iterate through each bit
    for (int i = 0; i < 32; i++) {
        // Set the input pin to the current bit value
        if ((data >> i) & 1) {
            gpio_set(GPIOC, INPUT_PIN);
        } else {
            gpio_clear(GPIOC, INPUT_PIN);
        }

        // Iterate through each row selection pin
        for (int row = 0; row < 32; row++) {
            // Set the current row selection pin
            if (row == i % 32) {
                gpio_set(GPIOD, ROWS_MASK);
            } else {
                gpio_clear(GPIOD, ROWS_MASK);
            }
        }

        // Simulate a clock pulse by toggling the clock pin
        gpio_set(GPIOC, CLOCK_PIN);
        gpio_clear(GPIOC, CLOCK_PIN);

        // Simulate a latch pulse 
        gpio_set(GPIOC, LATCH_PIN);
        gpio_clear(GPIOC, LATCH_PIN);

        // Simulate a delay
        for (volatile int j = 0; j < 100000; j++) {}
    }
}
// Function to set a specific row and call performClockPulses (Mentioned on the coursework website)
void setRowAndPerformClockPulses(uint32_t data, int row) {
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);

    // Configure GPIO pins
    setup();

    // Set the desired row
    for (int r = 0; r < 32; r++) {
        if (r == row) {
            gpio_set(GPIOD, ROWS_MASK);
        } else {
            gpio_clear(GPIOD, ROWS_MASK);
        }
    }

    // Call the function with the 32-bit integer
    performClockPulses(data);
}
// Function to update the display with red, green, and blue data for each half (Mentioned in on the coursework website)
void updateDisplay(uint32_t redData1, uint32_t greenData1, uint32_t blueData1,
                   uint32_t redData2, uint32_t greenData2, uint32_t blueData2,
                   int row) {
    // Latch before updating
    gpio_set(GPIOC, LATCH_PIN);  // Assuming GPIOC_PIN_8 is the latch pin
    gpio_clear(GPIOC, LATCH_PIN);

    // Update display for the first half
    setRowAndPerformClockPulses(redData1, row);
    setRowAndPerformClockPulses(greenData1, row);
    setRowAndPerformClockPulses(blueData1, row);

    // Update display for the second half
    setRowAndPerformClockPulses(redData2, row);
    setRowAndPerformClockPulses(greenData2, row);
    setRowAndPerformClockPulses(blueData2, row);

    // No need to latch again as it's done once before updating
}

void drawVerticalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x, y + i);
  }
}

// This function draws a horizontal line of length l to the right from (x, y)
void drawHorizontalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x + i, y);
  }
}
// Function to draw a point using GPIO functions
void drawPoint(int x, int y) {
    if (x >= 0 && x < PANEL_WIDTH && y >= 0 && y < PANEL_HEIGHT) {
        // Calculate the GPIO pin corresponding to the x-coordinate
        uint16_t pin = 1 << (x % 16);

        // Set the corresponding pin in the GPIO register for the specified row
        gpio_set(GPIOC, LATCH_PIN);  // Set latch
        gpio_clear(GPIOC, ROWS_MASK); // Clear all rows
        gpio_set(GPIOC, (ROWS_MASK & ~(1 << (y + 2)))); // Set the specific row
        gpio_clear(GPIOC, LATCH_PIN);  // Clear latch

        // Set the pin corresponding to the x-coordinate
        gpio_set(GPIOC, pin);
    }
}

// Function to draw a rectangle using GPIO functions
void drawRect(int x, int y, int length, int height) {
  drawVerticalLine(x, y, height);
  drawVerticalLine(x + length, y, height);
  drawHorizontalLine(x, y, length);
  drawHorizontalLine(x, y + height, length);
}

// Function to draw a circle using GPIO functions (this function has been implemented using MidPoint Theorem)
void drawCircle(int x, int y, int radius) {
    int decision_param = 3 - 2 * radius;
    int i = 0;
    int j = radius;

    while (i <= j) {
        drawPoint(x + i, y + j);
        drawPoint(x - i, y + j);
        drawPoint(x + i, y - j);
        drawPoint(x - i, y - j);
        drawPoint(x + j, y + i);
        drawPoint(x - j, y + i);
        drawPoint(x + j, y - i);
        drawPoint(x - j, y - i);

        i++;

        if (decision_param > 0) {
            j--;
            decision_param = decision_param + 4 * (i - j) + 10;
        } else {
            decision_param = decision_param + 4 * i + 6;
        }

        drawPoint(x + i, y + j);
        drawPoint(x - i, y + j);
        drawPoint(x + i, y - j);
        drawPoint(x - i, y - j);
        drawPoint(x + j, y + i);
        drawPoint(x - j, y + i);
        drawPoint(x + j, y - i);
        drawPoint(x - j, y - i);
    }
}
// This function is responsible for rendering the borders of the game
void drawZone() {
  drawRect(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
}

// This function is responsible for rendering the ball
void drawBall(){
  drawCircle(ballX, ballY, ballRadius); 
}

// This function is responsible for rendering the bats
void drawBats(){ // Needs correction
  drawRect(PANEL_WIDTH - batSpacing - batWidth, rightBatOffset, batWidth, batLength);
  drawRect(batSpacing, leftBatOffset, batWidth, batLength);
}

// This function renders both of the players scores to the top of the display
void drawScores(){
  // Draws the colon which separates the scores
  drawCircle(PANEL_WIDTH/2, PANEL_HEIGHT - scoresOffset - symbolSpacing, 2);
  drawCircle(PANEL_WIDTH/2, PANEL_HEIGHT - scoresOffset - symbolSegmentLength*2 + symbolSpacing, 2);

  // Draws the left player's score (Player 1)
  drawDigit(PANEL_WIDTH/2 - symbolSegmentLength*2 - symbolSpacing*2, PANEL_HEIGHT - symbolSegmentLength*2 - scoresOffset, leftScore/10);
  drawDigit(PANEL_WIDTH/2 - symbolSegmentLength - symbolSpacing, PANEL_HEIGHT - symbolSegmentLength*2 - scoresOffset, leftScore%10);

  // Draws the right player's score (Player 2)
  drawDigit(PANEL_WIDTH/2 + symbolSpacing, PANEL_HEIGHT - symbolSegmentLength*2 - scoresOffset, rightScore/10);
  drawDigit(PANEL_WIDTH/2 + symbolSpacing*2 + symbolSegmentLength, PANEL_HEIGHT - symbolSegmentLength*2 - scoresOffset, rightScore%10);
}


void drawDigit(int x, int y, int n){
  // Ensure that n is within the valid range by assusinmg its unit value
  n %= 10;
  // For each of the 7 segments, determine whether the value of n requires the
  // the segment to produce the digit. If so draw the segment appropriatly.
  if(n != 1 && n != 4)                      drawHorizontalLine(x, y + symbolSegmentLength*2, symbolSegmentLength);
  if(n != 5 && n != 6)                      drawVerticalLine(x + symbolSegmentLength, y + symbolSegmentLength, symbolSegmentLength);
  if(n != 2)                                drawVerticalLine(x + symbolSegmentLength, y, symbolSegmentLength);
  if(n != 1 && n != 4 && n != 7 && n != 9)  drawHorizontalLine(x, y, symbolSegmentLength);
  if(n == 0 || n == 2 || n == 6 || n == 8)  drawVerticalLine(x, y, symbolSegmentLength);
  if(n != 1 && n != 2 && n != 3 && n != 7)  drawVerticalLine(x, y + symbolSegmentLength, symbolSegmentLength);
  if(n != 0 && n != 1 && n != 7)            drawHorizontalLine(x, y + symbolSegmentLength, symbolSegmentLength);
}

// This function is responsible for rendering the gameover state
void drawWinner(){
  // Determine the width of the endgame message
  int messageWidth = (symbolSegmentLength*8 + symbolSpacing*4)/2;

  // Draw the letter P (for 'Player')
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth, PANEL_HEIGHT/2, symbolSegmentLength);
  drawHorizontalLine(PANEL_WIDTH/2 - messageWidth, PANEL_HEIGHT/2, symbolSegmentLength);
  drawHorizontalLine(PANEL_WIDTH/2 - messageWidth, PANEL_HEIGHT/2 + symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength, PANEL_HEIGHT/2, symbolSegmentLength);

  // Display a digit corresponding to the winning player (1 or 2)
  drawDigit(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength + symbolSpacing, PANEL_HEIGHT/2 - symbolSegmentLength, winner);

  // Draw the letter W
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*4 + symbolSpacing, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + (int)(symbolSegmentLength*4.5) + symbolSpacing, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*5 + symbolSpacing, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawHorizontalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*4 + symbolSpacing, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength);

  // Draw the letter I
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + (int)(symbolSegmentLength*5.5) + symbolSpacing*2, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);

  // Draw the letter N
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*6 + symbolSpacing*3, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawHorizontalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*6 + symbolSpacing*3, PANEL_HEIGHT/2 + symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*7 + symbolSpacing*3, PANEL_HEIGHT/2 - symbolSegmentLength, symbolSegmentLength*2);

  // Draw the letter S
  drawDigit(PANEL_WIDTH/2 - messageWidth + symbolSegmentLength*7 + symbolSpacing*4, PANEL_HEIGHT/2 - symbolSegmentLength, 5);
}

void batCollision(){
  // If the ball strikes a bat, rebound it in the opposite direction and increase the ball's speed
  if(
    // Check the left bat
    (ballControlX < 0 && ballX - ballRadius < batWidth + batSpacing && ballY > leftBatOffset && ballY < leftBatOffset + batLength) ||
    // Check the right bat
    (ballControlX > 0 && ballX + ballRadius > PANEL_WIDTH - batWidth - batSpacing && ballY > rightBatOffset && ballY < rightBatOffset + batLength)
  )
  {
    ballControlX = -ballControlX;
    increaseSpeed();
  }
}

void increaseSpeed() {
    ballSpeed += speedIncrement;
    if (ballSpeed > maxSpeed){
        ballSpeed = maxSpeed;
    }
}

void pulse(int pin){ //Performs input and a clock pulse
    gpio_set(GPIOC, pin);
    gpio_clear(GPIOC, pin);
    gpio_set(GPIOC, CLOCK_PIN);
    return 0;
}

void set(int pin){
  gpio_clear(GPIOC, CLOCK_PIN);
  gpio_set(GPIOC, pin);
  gpio_set(GPIOC, CLOCK_PIN);
}

void clear(int pin){
  gpio_clear(GPIOC, CLOCK_PIN);
  gpio_clear(GPIOC, pin);
  gpio_set(GPIOC, CLOCK_PIN);
}

void clearAllRows(void){
    gpio_clear(GPIOC, GPIO2);
    gpio_clear(GPIOC, GPIO3);
    gpio_clear(GPIOC, GPIO4);
    gpio_clear(GPIOC, GPIO5);
}


void clock(void){
  gpio_clear(GPIOC, CLOCK_PIN);
  gpio_set(GPIOC, CLOCK_PIN);
}


void setup(void){
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LATCH_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, LATCH_PIN); 
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CLOCK_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, CLOCK_PIN);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, INPUT_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, INPUT_PIN);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO5);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO4);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO3);
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO2);
}

// Function to configure GPIO registers
void configRegisters() {
    rcc_periph_clock_enable(RCC_ADC12); //Enable clock for ADC registers 1 and 2

    //Setting up adc register 1 --> we require only one of the registers since both the joysticks are connected to 1 register
    adc_power_off(ADC_REG);  //Turn off ADC register 1 whist we set it up

    adc_set_clk_prescale(ADC_REG, ADC_CCR_CKMODE_DIV1);  //Setup a scaling, none is fine for this
    adc_disable_external_trigger_regular(ADC_REG);   //We don't need to externally trigger the register...
    adc_set_right_aligned(ADC_REG);  //Make sure it is right aligned to get more usable values
    adc_set_sample_time_on_all_channels(ADC_REG, ADC_SMPR_SMP_61DOT5CYC);  //Set up sample time
    adc_set_resolution(ADC_REG, ADC_CFGR1_RES_12_BIT);  //Get a good resolution

    adc_power_on(ADC_REG);  //Finished setup, turn on ADC register 1

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, INPUT_PIN | CLOCK_PIN | LATCH_PIN | ROWS_MASK);
}
uint32_t channelSetup(int val){
  uint8_t channelArray = {val};  //Define a channel that we want to look at
  adc_set_regular_sequence(ADC_REG, 1, channelArray);  //Set up the channel
  adc_start_conversion_regular(ADC_REG);  //Start converting the analogue signal

  while(!(adc_eoc(ADC_REG)));  //Wait until the register is ready to read data

  uint32_t value = adc_read_regular(ADC_REG);  //Read the value from the register and channel
  return value
}
// Function to reset the ball
void resetBall() {
  ballX = PANEL_WIDTH/2;
  ballY = PANEL_HEIGHT/2;
  ballSpeed = initialSpeed;
}
// Function to initialize the game
void resetGame() {
  leftScore = 0; rightScore = 0;
  winner = 0;
  resetBall();
}

// Start of the program
int main(void) { 
  configRegisters();
  clearAllRows();
  clear();
  set();
  setup();

  while (1){
    
    input();
    update();
    render();
  }
  return 0;
}

//Main functions
// Extracts current values of the paddle
input(){
  // Set channels for each direction for each individual joystick
  leftTop = channelSetup(1);
  leftDown = channelSetup(2);
  rightTop = channelSetup(6);
  rightDown = channelSetup(7);
  
  // *ADC_REG = 0x2;
  // while (*ADC_REG & 0x10 != 1);
  if(leftTop != 0){
    uint32_t temp = leftTop;
  }
  else{
    uint32_t temp = leftDown;
  }
  int leftPaddleValue = (int)temp;
  leftBatOffset = (leftPaddleValue - minPaddleValue)
                  *(PANEL_HEIGHT - batLength)
                  /(maxPaddleValue - minPaddleValue);

  // *ADC_REG = 0x2;
  // while(*ADC_REG & 0x20 != 1);
  if (rightTop!= 0){
    int temp = rightTop ;
  }
  else{
    int temp = rightDown;
  }
  int rightPaddleValue = temp; //does'nt seem like the value is being receieved after the if and else stmns??
  rightBatOffset = (rightPaddleValue - minPaddleValue)
                   *(PANEL_HEIGHT - batLength)
                   /(maxPaddleValue - minPaddleValue);

}

// Handles any calculations prior to rendering of the game interface
update(){
  moveBall();
  zoneCollision();

  if(leftScore >= winningScore){
    winner = 1; // Indicate that player 1 has won the match
    ballControlX = 1; // Specify ball Velocity to favour the winner

    if(leftScore >= scoreBeforeReset)
    {
      resetGame();
    }
  }
  else if(rightScore >= winningScore){
    winner = 2;
    ballControlX = -1;   

    if(rightScore >= scoreBeforeReset)
    {
      resetGame();
    }
  }
  else{
    batCollision();
  }
}

//Renders the state of the display
render(){
  if(winner == 0){
    drawZone();
    drawScores();
    drawBats();
    drawBall();
  }
  else{
    drawWinner();
  }
}
