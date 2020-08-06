#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <time.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>

typedef enum { RELEASED=0, PRESSED } key_t;

#define o1 PB2
#define o2 PB1
#define o3 PB0
#define KEY0 (1 << PB3)
#define KEY1 (1 << PB4)
#define SLOW 0x20

const uint8_t sProg = 0;
const uint8_t sLock = 1;
const uint8_t sWrong = 2;

//initializes all volatile values
volatile uint8_t history0 = 0;//history is used for button debouncing
volatile uint8_t history1 = 0;
volatile key_t keystate0 = RELEASED;//state the buttons being pressed or released
volatile key_t keystate1 = RELEASED;
volatile uint8_t currentState = 0;// variable that holds the value of the current place in the FSM
volatile uint8_t passcode =0;// holds the passcode to the lock
volatile uint8_t enteredCode =0;// holds the code input during the locked state
volatile uint8_t numInputs =0;// counter used to keep track of how many times a button has been pushed
volatile uint8_t stateChanged =0; //used as a boolean to tell if a button has changed states since alst being polled
volatile uint8_t stateUsed =0;// used to tell if the state variable has already been used. keeps program from viewing putton hold as many button presses
volatile key_t oldState0 = RELEASED; //holds the last polled state of the button
volatile key_t oldState1 = RELEASED;



void progMode();
void lockMode();
void wrongMode();
void green();
void red();
void yellow();
void blue();
void yellow_green();
void ledOff();
ISR(TIMER0_COMPA_vect);




int main (void)
{DDRB= 0x7; //sets pins 0,1,2 to output
  OCR0A = SLOW;
  TCCR0B = 0b101;
  TIMSK = (1 << OCIE0A);


sei();
  while(1){
    switch (currentState) {
      case 0:
        progMode();
        break;
      case 1:
        lockMode();
        break;
      case 2:
        wrongMode();
        break;

    }
}
}

void debounce(){
  history0 = history0 << 1;
  history1 = history1 << 1;

  if ((PINB & KEY0) == 0){ // low if button is pressed!
  history0 = history0 | 0x1;}

  if ((PINB & KEY1) == 0){ // low if button is pressed!
  history1 = history1 | 0x1;}

  if ((history0 & 0b111111) == 0b111111){
  keystate0 = PRESSED;
}
  if ((history1 & 0b111111) == 0b111111){
  keystate1 = PRESSED;}

  if ((history0 & 0b00111111) == 0){
  keystate0 = RELEASED;}
  if ((history1 & 0b00111111) == 0){
  keystate1 = RELEASED;}
}

ISR(TIMER0_COMPA_vect) {
  debounce();//debounces the inputs

  stateChanged = 0;//resets stateChanged

  if(keystate0 != oldState0){//if current button press differs from old button press then state has changed
    stateChanged =1;
  }

  if(keystate1 != oldState1){//if current button press differs from old button press then state has changed
  stateChanged =1;
}

  if(stateChanged &&(keystate0 ==PRESSED || keystate1 == PRESSED) &&stateUsed){ //sets state to 0 if a new button press is detected.
    stateUsed =0;
    blue();//briefly flashes the blue LED to indicate a button press
    _delay_ms(10);
  }

  oldState0 =keystate0;//sets this button press as the old button press
  oldState1 =keystate1;
  OCR0A += SLOW;// increments the vaue of the timer comparator back to the value of SLOW
}

void progMode(){
  yellow();//sets the led to yellow and green. Method runs fast enough that both will appear on due to POV
  _delay_ms(1);//all leds will turn on without this wait due to POV
  if(stateChanged !=0 && stateUsed ==0){ //if the button state has changed since last execution: continue
    stateUsed =1;// the current button state has been used
    passcode = passcode <<1; //shifts over currently entered code
    if (keystate1==PRESSED) { //if keyt1 is pressed then next entry into passcode is a 1
      passcode = passcode | 0x1;
    }
    numInputs +=1; //increments the number of times an input has been recorded

    if(numInputs > 5 ){ //if six inputs have been entered then passcode has been set
      currentState = sLock; // lock the safe
      numInputs =0; //resets numInputs
    }
  }
  green();//sets the led to yellow and green. Method runs fast enough that both will appear on due to POV
  _delay_ms(1);//all leds will turn on without this wait due to POV
}
//state where the safe is locked.
void lockMode(){
  red();//sets the LED status to red to indicate the safe is locked
  if(stateChanged !=0 && stateUsed ==0){  //if the button state has changed since last execution: continue
        stateUsed =1;// the current button state has been used
    enteredCode = enteredCode <<1;//shifts over currently entered code
    if(keystate1 == PRESSED){ //if keyt1 is pressed then next entry into enteredCode is a 1
      enteredCode = enteredCode | 0x1;
    }
    //else the next entry is a 0 and is already there from the shift left
    numInputs +=1;//increments the number of times an input has been recorded
    if(numInputs >5){//if six inputs have been entered then check against passcode
      numInputs =0;//reset numInputs
      if(enteredCode == passcode){//correct passcode has been entered
        currentState = sProg;//unlock the safe
        passcode =0;//reset the passcode
      }
    else if(enteredCode != passcode){//incorrect passcode ahs been entered
      currentState = sWrong;//set the state to indicate the
    }
    enteredCode =0;// resets passcode that was entered when trying to unlock the safe
  }
}

}
//state that indicates a wrong passcode was used. Flashes the Yellow LED on and off three times, and prevents further inputs until the sequence has finished.
void wrongMode(){
  cli();
  ledOff();
  _delay_ms(50);
  yellow();
  _delay_ms(100);
  ledOff();
  _delay_ms(100);
  yellow();
  _delay_ms(100);
  ledOff();
  _delay_ms(100);
  yellow();
  _delay_ms(100);
  ledOff();

  currentState = sLock;
  sei();
}
//sets the red led to on, turns off other leds
void red(){
  PORTB |= (1 << o1);
  PORTB &= ~(1 << o2);
  PORTB &= ~(1 << o3);
}
//sets the green led to on, turns off other leds
void green(){
  PORTB &= ~(1 << o1);
  PORTB &= ~(1 << o2);
  PORTB |= (1 << o3);
}
//sets the blue led to on, turns off other leds
void blue(){
  PORTB &= ~(1 << o1);
  PORTB |= (1 << o2);
  PORTB |= (1 << o3);
}
//sets the yellow led to on, turns off other leds
void yellow(){
  PORTB |= (1 << o1);
  PORTB |= (1 << o2);
  PORTB &= ~(1 << o3);
}
//turns all leds off
void ledOff(){
  PORTB &= ~(1 << o1);
  PORTB &= ~(1 << o2);
  PORTB &= ~(1 << o3);
}
