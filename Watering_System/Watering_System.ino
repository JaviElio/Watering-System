/**********************************************
  Project: Watering System V1.0       27-12-2016
  JEC
***********************************************/

#include <TinyWireM.h>                      // Attiny i2c Library 
#include <SoftwareSerial.h>                 // Serial communication Library
#include <I2CSoilMoistureSensor.h>          // I2C moisture sensor library

#include <avr/sleep.h>                      // Power managment library
#include <avr/wdt.h>                        // Watchdog library
#include <avr/interrupt.h>                  // Interrupts managment library


#define SENSOR PB3                          // Moisture sensor activaction output
#define E_VALVE PB1                         // Solenoid Valve activation output

// Create a the serial port
SoftwareSerial serialPort(-1, 4);

// New sensor object
I2CSoilMoistureSensor sensor;


// Variables
unsigned int nLight;                      // Measured light value
unsigned int nLightMax = 40000;              // Max light to water
unsigned int nTemp;                       // Measured temp value
unsigned int nTempMin  = 40;              // Min. temp to water
unsigned int nTempMed  = 80;              
unsigned int nMoisture;                   // Measured moist value
unsigned int nMoistureMin = 325;          // Min. moist to water
unsigned int nMoistureMax = 400;          // Max. moist to water
unsigned int nShortSleep = 300;           // Short sleep lenght in seconds 300s=5min
unsigned int nLongSleep = 3600;           // Long sleep lenght in seconds 3600s=1h
unsigned int nWateringTime= 30000;        // Watering time (ms) 

unsigned int nState = 0;                  // "Switch" index
bool bOldState = 0;                       // Previous valve state
unsigned int nWateringTimes = 0;          // watering times
unsigned int nMaxWateringTimes = 5;       // Max. watering times


void setup() {

  configurePins();                        // Configure I/O
  TinyWireM.begin();                      // Start i2c communication
  serialPort.begin(9600);                 // Start serial port
  setupPowerSaving();                     // Setup power save

}


void loop() {

  switch (nState) {

    case 0: // Switch on moist sensor

      serialPort.println("Switching on moist sensor (case 0)");
      digitalWrite(SENSOR, true);
      delay(1000);
      sensor.begin();                                                     // Start sensor
      delay(2000);

      nState = 10;
      break;

    case 10:  // Light and temperature measurement

      serialPort.print("Measuring light (case 10):   ");
      nLight = sensor.getLight(true);
      serialPort.println(nLight);

      serialPort.print("Measuring temperature (case 10):   ");
      nTemp = sensor.getTemperature();
      serialPort.println(nTemp);


      // If "dark" and tempeture is higher that 4ºC then water
      if ((nLight > nLightMax) && nTemp > nTempMed) {
        nState = 20;
      }

      // No watering then go to sleep
      else {
        
        nWateringTimes = 0;
        bOldState = false;
        nState = 200;
        
      }

      break;

    case 20: // Measure moist

      serialPort.print("Measuring moist (case 20):   ");
      nMoisture = sensor.getCapacitance();
      serialPort.println(nMoisture);


      if ((nMoisture < nMoistureMin) || (nMoisture < nMoistureMax) && bOldState) {

        nWateringTimes++;

        if (nWateringTimes > nMaxWateringTimes) {
          serialPort.println("Max watering times reached!");
          nWateringTimes = 0;
          bOldState = false;
          nState = 200;
        }
        else {
          bOldState = true;
          nState = 30;
        }
      }

      if ((nMoisture > nMoistureMax) || (nMoisture > nMoistureMin) && !bOldState) {

        nWateringTimes = 0;
        bOldState = false;
        nState = 200;
      }

      break;

    case 30:  // Water

      serialPort.println("Watering (case 30)");
      digitalWrite(SENSOR, false);                // Switch off moist sensor
      digitalWrite(E_VALVE, true);                // Switch on solenoid valve
      delay(nWateringTime);
      digitalWrite(E_VALVE, false);               // Switch off solenoid valve

      nState = 100;
      break;


    case 100:   // Short sleep

      serialPort.println("Short sleep (case100)");

      digitalWrite(SENSOR, false);                // Switch off sesorr
      sleep(nShortSleep/8);                       // Sleep "nShortSleep" seconds

      nState = 0;
      break;

    case 200:   // Long sleep

      serialPort.println("Long sleep (case200)");

      digitalWrite(SENSOR, false);                // Switch off sensor
      sleep(nLongSleep/8);                        // Sleep "nLongSleep" seconds

      nState = 0;
      break;


    default: nState = 0;

  }

}


void setupWatchdog() {
  cli();                                            // Disable global interrupts
  // Set watchdog timer in interrupt mode
  // allow changes, disable reset
  WDTCR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval
  WDTCR = bit (WDIE) | bit (WDP3) | bit (WDP0);     // set WDIE, and 8 seconds delay
  sei();                                            // Enable global interrupts
}


void sleep(int times) {

  setupWatchdog();                                  // Configure and activate Watchdog

  for (int i = 0; i < times; i++) {

    cli();                                          // Disable interruptions just in case
    sleep_enable();                                 // Enable sleep


    //    sleep_bod_disable();                       // Deshabilitar BOD durante sleep
    //    MCUCR |= _BV(BODS) | _BV(BODSE);           //disable brownout detection during sleep
    //    MCUCR &=~ _BV(BODSE);

    sei();                                          // Enable interruptions
    sleep_cpu();                                    // Go to sleep
    sleep_disable();                                // When cpu is awake, disable sleep

  }

  wdt_disable();                                    // Disable watchdog in order to not have more WDT interruptions
}


void setupPowerSaving() {

  ADCSRA &= ~(1 << ADEN);                           // Disable ADC
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);              // Configure sleep mode
  
}

void configurePins() {

  pinMode(PB0, OUTPUT);                              // i2c SDA
  pinMode(PB1, OUTPUT);                              // E_VALVE
  pinMode(PB2, OUTPUT);                              // i2c SCL
  pinMode(PB3, OUTPUT);                              // SENSOR
  //pinMode(PB4, OUTPUT);                            // Tx
  //pinMode(PB5, OUTPUT);                            // Rx

  digitalWrite(PB0, false);                          // i2c SDA
  digitalWrite(PB1, false);                          // E_VALVE
  digitalWrite(PB2, false);                          // i2c SCL
  digitalWrite(PB3, false);                          // SENSOR
  //digitalWrite(PB4, false);                        // Tx
  //digitalWrite(PB5, false);                        // Rx

}

ISR(WDT_vect) {

  // Nothing to do, just wake up

}
