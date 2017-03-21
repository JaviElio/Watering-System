/**********************************************
  Project: Watering System V1.0       27-12-2016
  JEC
***********************************************/

#include <TinyWireM.h>                      // Attiny i2c Library 
#include <SoftwareSerial.h>                 // Libreria para crear puerto serie por software
#include <I2CSoilMoistureSensor.h>          // Libreria para comunicación i2c con sensor humedad

#include <avr/sleep.h>                      // Libreria para gestion de energia
#include <avr/wdt.h>                        // Libreria para uso del watchdog
#include <avr/interrupt.h>                  // Libreria para interrupciones

#define SENSOR PB3                          // Activacion sensor humedad
#define E_VALVE PB1                         // Activacion electrovalvula


// Crear un puerto serie
SoftwareSerial puertoSerie(-1, 4);

// Crear objeto sensor
I2CSoilMoistureSensor sensor;


// Variables
unsigned int nLight;                      // Valor de luz medido
unsigned int nLightMax = 40000;           // Valor de luz maximo para regar
unsigned int nTemp;                       // Valor de temperatura medido
unsigned int nTempMin  = 40;              // Valor de temperatura mínimo para regar
unsigned int nTempMed  = 80;              // Valor de temperatura "regar con luz"
unsigned int nMoisture;                   // Valor de humedad medido
unsigned int nMoistureMin = 325;          // Valor de humedad mínimo para regar
unsigned int nMoistureMax = 400;          // Valor de humedad máximo para regar
unsigned int nSegundosSleepCorto = 300;    // Duración de sleep "corto" en segundos 5min=300s
unsigned int nSegundosSleepLargo = 3600;     // Duración de sleep "largo" en segundos 1h=3600s
unsigned int nWateringTime= 30000;         // Watering time (ms) 

unsigned int nState = 0;                  // indice "Switch"
bool bOldValvula = 0;                     // Estado anterior de la válvula
unsigned int nRepeticiones = 0;           // Veces que se ha regado
unsigned int nRepeticionesMax = 5;        // Repeticiones máximas


void setup() {

  configurePins();                   // Configurar pines

  TinyWireM.begin();                 // Iniciar comunicacion i2c
  puertoSerie.begin(9600);           // Iniciar puerto serie

  setupPowerSaving();                // Configurar ahorro de energia

}


void loop() {

  switch (nState) {

    case 0: // Encender sensor

      puertoSerie.println("Encendiendo sensor (case 0)");
      digitalWrite(SENSOR, true);
      delay(1000);
      sensor.begin();                                         // Iniciar sensor
      delay(2000);

      nState = 10;
      break;

    case 10:  // Medir luz

      puertoSerie.print("Midiendo luz (case 10):   ");
      nLight = sensor.getLight(true);
      puertoSerie.println(nLight);

      puertoSerie.print("Midiendo temperatura (case 10):   ");
      nTemp = sensor.getTemperature();
      puertoSerie.println(nTemp);


      // Si "oscuro" y temperatura mayor que 4º se riega
      if ((nLight > nLightMax) && nTemp > nTempMed) {
        nState = 20;
      }

      // Si "luz" y temperatura entre 4ºC y 8ºC se riega
      else if ((nLight < nLightMax) && (nTemp > nTempMin && nTemp <= nTempMed)) {
        nState = 20;
      }

      else {
        // No regar -> sleep
        nRepeticiones = 0;
        bOldValvula = false;
        nState = 200;
      }

      break;

    case 20: // Leer humedad

      puertoSerie.print("Midiendo humedad (case 20):   ");
      nMoisture = sensor.getCapacitance();
      puertoSerie.println(nMoisture);


      if ((nMoisture < nMoistureMin) || (nMoisture < nMoistureMax) && bOldValvula) {

        nRepeticiones++;

        if (nRepeticiones > nRepeticionesMax) {
          puertoSerie.println("Max repeticiones alcanzadas!!!!");
          nRepeticiones = 0;
          bOldValvula = false;
          nState = 200;
        }
        else {
          bOldValvula = true;
          nState = 30;
        }
      }

      if ((nMoisture > nMoistureMax) || (nMoisture > nMoistureMin) && !bOldValvula) {

        nRepeticiones = 0;
        bOldValvula = false;
        nState = 200;
      }

      break;

    case 30:  // Regar

      puertoSerie.println("Regando (case 30)");
      digitalWrite(SENSOR, false);    // Apagar sensor
      digitalWrite(E_VALVE, true);    // Encender electroValvula
      delay(nWateringTime);
      digitalWrite(E_VALVE, false);   // Apagar electrovalvula

      nState = 100;
      break;


    case 100:   // Sleep "corto"

      puertoSerie.println("Sleep corto (case100)");

      digitalWrite(SENSOR, false);    // Apagar sensor
      sleep(nSegundosSleepCorto/8);                       // dormir 1x8segundos = 8 segundos

      nState = 0;
      break;

    case 200:   // Sleep "largo"

      puertoSerie.println("Sleep largo (case200)");

      digitalWrite(SENSOR, false);    // Apagar sensor
      sleep(nSegundosSleepLargo/8);                       // dormir 2x8segundo = 16 segundos

      nState = 0;
      break;


    default: nState = 0;

  }

}


void setupWatchdog() {
  cli();
  // Set watchdog timer in interrupt mode
  // allow changes, disable reset
  WDTCR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval
  WDTCR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
  sei(); // Enable global interrupts
}


void sleep(int times) {

  setupWatchdog();                                // Configurar y activar WDT

  for (int i = 0; i < times; i++) {

    cli();                                       // Deshabilitar las interrupciones por si acaso
    sleep_enable();                              // Habilitar sleep


    //    sleep_bod_disable();                       // Deshabilitar BOD durante sleep
    //    MCUCR |= _BV(BODS) | _BV(BODSE);           //disable brownout detection during sleep
    //    MCUCR &=~ _BV(BODSE);

    sei();                                       // Habilitar interrupciones
    sleep_cpu();                                 // Dormir....
    sleep_disable();                             // Deshabilitar sleep una vez despierto.

  }

  wdt_disable();                                // Deshabilitar WDT para no más interrupciones
}


void setupPowerSaving() {

  // Desconectar ADC
  ADCSRA &= ~(1 << ADEN);

  // Modo "sleep"
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Desconectar modulos Timer1, adc
  // PRR=B1001;
}

void configurePins() {

  pinMode(PB0, OUTPUT);               // i2c SDA
  pinMode(PB1, OUTPUT);                 // E_VALVE
  pinMode(PB2, OUTPUT);              // i2c SCL
  pinMode(PB3, OUTPUT);                 // SENSOR
  //pinMode(PB4, OUTPUT);               // Tx
  //pinMode(PB5, OUTPUT);               // Rx

  digitalWrite(PB0, false);           // i2c SDA
  digitalWrite(PB1, false);             // E_VALVE
  digitalWrite(PB2, false);           // i2c SCL
  digitalWrite(PB3, false);             // SENSOR
  //digitalWrite(PB4, false);           // Tx
  //digitalWrite(PB5, false);           // Rx

}

ISR(WDT_vect) {

  // Nada

}









