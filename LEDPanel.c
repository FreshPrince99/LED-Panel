#include "libopencm3/stm32/rcc.h"   //Needed to enable clocks for particular GPIO ports
#include "libopencm3/stm32/gpio.h"  //Needed to define things on the GPIO
#include "libopencm3/stm32/adc.h" //Needed to convert analogue signals to digital

#define IOPORT GPIOA
#define ADC_REG ADC1

// Define GPIO pins
#define GPIO_LED    GPIOC // for the input pin
#define GPIO_CLOCK  GPIOC // for the clock
#define GPIO_LATCH  GPIOC // for the latch
#define GPIO_ROWS   GPIOC // for the rows
#define INPUT_PIN  GPIO6 
#define CLOCK_PIN  GPIO7
#define LATCH_PIN  GPIO8

// Game interface variables
#define PANEL_WIDTH  16
#define PANEL_HEIGHT 32
#define symbolSegmentLength 5
#define symbolSpacing 4
#define scoresOffset 3
#define scoreBeforeReset (winningScore + 3)
#define batWidth 2
#define batSpacing 2
#define maxPaddleVal 105
#define minPaddleVal 555  // both have to be tested
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
int ballVelocityX = 1, ballVelocityY = 1; // Velocity control variables for determining the balls direction
int leftScore, rightScore;              // Respective scores for both the players
uint32_t leftTop, leftDown, rightTop, rightDown; //Directions for both the paddles

//This function draws a vertical line of length l downwards from (x, y)
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
        gpio_set(GPIO_LATCH, LATCH_PIN);  // Set latch
        gpio_clear(GPIOC, ROW_PINS); // Clear all rows
        gpio_set(GPIOC, (ROW_PINS & ~(1 << (y + 2)))); // Set the specific row
        gpio_clear(GPIO_LATCH, LATCH_PIN);  // Clear latch

        // Set the pin corresponding to the x-coordinate
        gpio_set(GPIO_LED, pin);
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
void drawBats(){ 
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

//Draws a digit to display the player number where n is the number
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
    (ballVelocityX < 0 && ballX - ballRadius < batWidth + batSpacing && ballY > leftBatOffset && ballY < leftBatOffset + batLength) ||
    // Check the right bat
    (ballVelocityX > 0 && ballX + ballRadius > PANEL_WIDTH - batWidth - batSpacing && ballY > rightBatOffset && ballY < rightBatOffset + batLength)
  )
  {
    ballVelocityX = -ballVelocityX;
    increaseSpeed();
  }
}
void moveBall() { // updates the co-ordinates of the ball
  ballX += ballVelocityX * ballSpeed;
  ballY += ballVelocityY * ballSpeed;
}

void increaseSpeed() { // increases the speed of the ball
    ballSpeed += speedIncrement;
    if (ballSpeed > maxSpeed){
        ballSpeed = maxSpeed;
    }
}
void zoneCollision(){
  // Check if the ball exits the game zone by moving right
  if(ballX + ballRadius > PANEL_WIDTH)
  {
    ballVelocityX = -ballVelocityX;
    leftScore++;
    resetBall();
  }
  // Check if the ball exits the game zone by moving left
  else if(ballX - ballRadius < 0)
  {
    ballVelocityX = -ballVelocityX;
    rightScore++;
    resetBall();
  }

  // Check if the ball exits the game zone by moving either up or down
  if(ballY + ballRadius > PANEL_HEIGHT || ballY - ballRadius < 0)
  {
    ballVelocityY = -ballVelocityY;
  }
}

void pulse(int pin){ //Performs input and a clock pulse
    gpio_set(GPIOC, pin);
    gpio_clear(GPIOC, pin);
    gpio_set(GPIOC, CLOCK_PIN);
    return 0;
}

void set(int pin){ // sets the input pin
  gpio_clear(GPIO_CLOCK, CLOCK_PIN);
  gpio_set(GPIO_LED, pin);
  gpio_set(GPIO_CLOCK, CLOCK_PIN);
}

void clear(int pin){ // clears the input pin
  gpio_clear(GPIO_CLOCK, CLOCK_PIN);
  gpio_clear(GPIO_LED, pin);
  gpio_set(GPIO_CLOCK, CLOCK_PIN);
}

void clearAllRows(void){ // clears all the row pins
    gpio_clear(GPIOC, GPIO2);
    gpio_clear(GPIOC, GPIO3);
    gpio_clear(GPIOC, GPIO4);
    gpio_clear(GPIOC, GPIO5);
}


void clock(void){ // sets the clock 
  gpio_clear(GPIO_CLOCK, CLOCK_PIN);
  gpio_set(GPIO_CLOCK, CLOCK_PIN);
}


void setup(void){ //sets all the GPIO pins
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_mode_setup(GPIO_LATCH, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LATCH_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIO_LATCH, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, LATCH_PIN); 
  gpio_mode_setup(GPIO_CLOCK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CLOCK_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIO_CLOCK, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, CLOCK_PIN);
  gpio_mode_setup(GPIO_LED, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, INPUT_PIN);            //GPIO Port Name, GPIO Mode, GPIO Push Up Pull Down Mode, GPIO Pin Number
  gpio_set_output_options(GPIO_LED, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, INPUT_PIN);
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
    setup();
  }
uint32_t channelSetup(int val){ //For setting up channels for each direction
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

  uint32_t temp;
  if(leftTop != 0){
    temp = leftTop;
  }
  else{
    temp = leftDown;
  }
  int leftPaddleVal = (int)temp;
  leftBatOffset = (leftPaddleVal - minPaddleVal)
                  *(PANEL_HEIGHT - batLength)
                  /(maxPaddleVal - minPaddleVal);

  uint32_t temp;
  if (rightTop!= 0){
    temp = rightTop ;
  }
  else{
    temp = rightDown;
  }
  int rightPaddleValue = (int)temp; //does'nt seem like the value is being receieved after the if and else stmns??
  rightBatOffset = (rightPaddleValue - minPaddleVal)
                   *(PANEL_HEIGHT - batLength)
                   /(maxPaddleVal - minPaddleVal);

}

// Handles any calculations prior to rendering of the game interface
update(){
  moveBall();
  zoneCollision();

  if(leftScore >= winningScore){
    winner = 1; // Indicate that player 1 has won the match
    ballVelocityX = 1; // Specify ball Velocity to favour the winner

    if(leftScore >= scoreBeforeReset)
    {
      resetGame();
    }
  }
  else if(rightScore >= winningScore){
    winner = 2;
    ballVelocityX = -1;   

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
