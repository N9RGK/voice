#include <Arduino.h>

#include "wiring_private.h" // pinPeripheral() function

#include <RadioLib.h>

#define USING_TIMER_TCC true

#include <SAMD21turboPWM.h>
#include <TimerInterrupt_Generic.h>
#include <talkie.h>

#include <vocab_us_large.h>

#define NSS 8
#define DIO0 3
#define RST_LoRa 4
#define DIO2 9
#define DIO1 6

#define SCK 24
#define MISO 22
#define MOSI 23

#define TEST 13

TurboPWM pwm;
Talkie talkie;
SX1276 radio = new Module(NSS, -1, RST_LoRa, DIO1);
SAMDTimer timer(TIMER_TCC);

void timer_irq(void);

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial1.setTimeout(5000);

  pinMode(DIO2, OUTPUT);
  pinMode(TEST, OUTPUT);

  pwm.setClockDivider(1, true);
  pwm.timer(1, 4, 256, true);
  pwm.analogWrite(DIO2, 750);

  timer.attachInterrupt(8000, timer_irq);
  timer.enableTimer();

  Serial.println("Testing the PWM");
}
int count = 0;

void timer_irq(void) {
  digitalWrite(TEST, HIGH);
  count++;
  unsigned int audiosample = (unsigned int)talkie.update_8khz();
  audiosample *= 4;
  pwm.analogWrite(DIO2, audiosample);
  digitalWrite(TEST, LOW);
}

void sayLetter(char l) {
  uint8_t *const letters[] = {
      spALPHA,  spBRAVO,   spCHARLIE, spDELTA, spECHO,   spFOXTROT, spGOLF,
      spHOTEL,  spINDIA,   spJULIET,  spKILO,  spLIMA,   spMIKE,    spNOVEMBER,
      spOSCAR,  spPAPA,    spQUEBEC,  spROMEO, spSIERRA, spTANGO,   spUNIFORM,
      spVICTOR, spWHISKEY, spXRAY,    spPAUSE, spZULU};
  // there is no recording of Yankee

  l = toupper(l);
  if (l >= 'A' && l <= 'Z') {
    talkie.say(letters[l - 'A']);
  }
}

void sayNumber(long n) {
  if (n < 0) {
    talkie.say(spMINUS);
    sayNumber(-n);
  } else if (n == 0) {
    talkie.say(spZERO);
  } else {
    if (n >= 1000) {
      int thousands = n / 1000;
      sayNumber(thousands);
      talkie.say(spTHOUSAND);
      n %= 1000;
      //      if ((n > 0) && (n < 100))
      //        talkie.say(spAND);
    }
    if (n >= 100) {
      int hundreds = n / 100;
      sayNumber(hundreds);
      talkie.say(spHUNDRED);
      n %= 100;
      //      if (n > 0)
      //        talkie.say(spAND);
    }
    if (n > 19) {
      int tens = n / 10;
      switch (tens) {
      case 2:
        talkie.say(spTWENTY);
        break;
      case 3:
        talkie.say(spTHIRTY);
        break;
      case 4:
        talkie.say(spFOURTY);
        break;
      case 5:
        talkie.say(spFIFTY);
        break;
      case 6:
        talkie.say(spSIXTY);
        break;
      case 7:
        talkie.say(spSEVENTY);
        break;
      case 8:
        talkie.say(spEIGHTY);
        break;
      case 9:
        talkie.say(spNINETY);
        break;
      }
      n %= 10;
    }
    switch (n) {
    case 1:
      talkie.say(spONE);
      break;
    case 2:
      talkie.say(spTWO);
      break;
    case 3:
      talkie.say(spTHREE);
      break;
    case 4:
      talkie.say(spFOUR);
      break;
    case 5:
      talkie.say(spFIVE);
      break;
    case 6:
      talkie.say(spSIX);
      break;
    case 7:
      talkie.say(spSEVEN);
      break;
    case 8:
      talkie.say(spEIGHT);
      break;
    case 9:
      talkie.say(spNINE);
      break;
    case 10:
      talkie.say(spTEN);
      break;
    case 11:
      talkie.say(spELEVEN);
      break;
    case 12:
      talkie.say(spTWELVE);
      break;
    case 13:
      talkie.say(spTHIRTEEN);
      break;
    case 14:
      talkie.say(spFOURTEEN);
      break;
    case 15:
      talkie.say(spFIFTEEN);
      break;
    case 16:
      talkie.say(spSIXTEEN);
      break;
    case 17:
      talkie.say(spSEVENTEEN);
      break;
    case 18:
      talkie.say(spEIGHTEEN);
      break;
    case 19:
      talkie.say(spNINETEEN);
      break;
    }
  }
}

void sayCall(char *callsign) {
  char *c = callsign;
  while (*c) {
    if (isAlpha(*c)) {
      sayLetter(*c);
    } else if (isdigit(*c)) {
      int i = *c - '0';
      sayNumber(i);
    }
    c++;
  }
}

void sayAltitude(uint32_t altitude) {

  talkie.say(spALTITUDE);
  sayNumber(altitude);
}

int missionStart = 0;
int lastCall = 0;
void loop() {
  char message[80] = {0};
  unsigned long now = millis();
  if (Serial1.available()) {
    size_t messageLength =
        Serial1.readBytesUntil('\n', message, sizeof(message));
    radio.beginFSK();
    radio.transmitDirect();
    delay(10);
    talkie.say(spPAUSE);
    if (messageLength > 0) {
      Serial.printf("received : %s\n", message);
      char *firstWord = strtok(message, " ");
      if (strncmp(firstWord, "altitude", 8) == 0) {
        char *number = strtok(NULL, " ");
        int altitude = atoi(number);
        sayAltitude(altitude);
      } else if (strncmp(firstWord, "launch", 6) == 0) {
        missionStart = millis();
        talkie.say(spLAUNCH);
      } else if (strncmp(firstWord, "apogee", 6) == 0) {
        talkie.say(spHIGH);
      } else if (strncmp(firstWord, "ready", 5) == 0) {
        talkie.say(spREADY);
      } else if (strncmp(firstWord, "fail", 4) == 0) {
        talkie.say(spNEGATIVE);
      } else if (strncmp(firstWord, "call", 4) == 0) {
        char *callsign = strtok(NULL, " ");
        sayCall(callsign);
      } else if (strncmp(firstWord, "touchdown", 10) == 0) {
        talkie.say(spTOUCHDOWN);
      }

    } else {
      Serial.write("No data");
    }

    radio.standby();
  }
}
