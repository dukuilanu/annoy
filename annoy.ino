#include <avr/sleep.h>
#include <avr/wdt.h> //Needed to enable/disable watch dog timer
#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC
#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR

unsigned long watchdogTimer = 300 / 8; // alarm time - 5 minutes / 8 sec wdt; reset randomly
unsigned long startTime = 0;        // start time
uint8_t mcucr1, mcucr2;
volatile uint32_t toggle_count;

ISR(WDT_vect) {
  //dummy callback, we just want the interrupt
}

void TrinketTone(uint32_t duration)
{
 uint32_t ocr = 250;
 uint8_t prescalarBits = 2;

 // CTC mode; toggle OC1A pin; set prescalar
 TCCR1 = 0x90 | prescalarBits;

 // Calculate note duration in terms of toggle count
 // Duration will be tracked by timer1 ISR
 toggle_count = 8000 * duration / 500;
 OCR1C = ocr-1; // Set the OCR
 bitWrite(TIMSK, OCIE1A, 1); // enable interrupt
}

// Timer1 Interrupt Service Routine:
// Keeps track of note duration via toggle counter
// When correct time has elapsed, counter is disabled
ISR(TIMER1_COMPA_vect)
{
 if (toggle_count != 0) // done yet?
 toggle_count--; // no, keep counting
 else // yes,
 TCCR1 = 0x90; // stop the counter
}

void setup_watchdog(int timerPrescaler) {
  noInterrupts();
  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7; 
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int

  //disable bods while we have the interrupts off
  mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
  mcucr2 = mcucr1 & ~_BV(BODSE);
  MCUCR = mcucr1;
  MCUCR = mcucr2;
  interrupts();
}

void setup()
{
  pinMode(1, OUTPUT);
  adc_disable(); // ADC uses ~320uA
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  delay(150);
  TrinketTone(50);
  delay(1000);
}

void loop()
{
  if (startTime >= watchdogTimer) {
    pinMode(1, OUTPUT);
    delay(500);
    TrinketTone(150);
    delay(250);
    pinMode(1, INPUT);
    startTime = 0;
    watchdogTimer = random(38, 76);
  } else {
    startTime++;
  }
  
  sleep_enable();
  setup_watchdog(9);
  sleep_cpu();
  sleep_disable();
}
