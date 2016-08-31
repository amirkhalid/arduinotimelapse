/**
 *  Name    : Time Lapse Motion xDrive
 *  Author  : Kamrul Ariffin
 *  Created : 
 *  Last Modified   :
 *  
 *  Update History
 *  --------------
 *  Version : 2.2 - Tidy up display text, fine tune speed and length.
 *  Version : 2.1 - To slide 1 meter to right (CW) with 1 variable - time is user input dependent - SUCCESS
 *                  To change direction, just tukar +ve to -ve at myStepper.step(1)
 *
 *  Components:
 *  -----------                    
 *  1.  Arduino UNO
 *  2.  LCD Keypad Shield ver1.0
 *  3.  DVR8225 motor driver
 *  4.  Wantaimotor geared 1:5.18 stepper, 12V 0.4A
 *
 *  Pin and Wiring
 *  --------------
 *  1.  Pin 13 to enable
 *  2.  Pin 3 to step
 *  3.  Pin 2 to dir
 *  4.  GND no connection
 *
 *  Arduino 5V + GND to driver 5V + GND
 *  Power supply 12V + GND to driver VM + GND
 *
 *  Driver to stepper motor : + in middle!
 *  A2      > Coil A-
 *  A1 (+)  > Coil A+
 *  B1 (+)  > Coil B+
 *  B2      > Coil B-
 *
 *
 *  Timelapse Slider Program
 *  ------------------------
 *  Slider length is either 1m or 2m (user selection)
 *  Steps to move 1m is 186480 steps - verified, with some space as buffer
 *  Steps to move 2m is xxxx steps - TBC
 *  Slide direction can be left->right or right-> (user selection)
 *  Slide duration will be set by user
 *
 *  Program aim is to move either 1m or 2m, in selected direction, in set time ("runtime")
 *  so speed is automatically calculated
 *  
 *  The step/flow will be:
 *  ## Welcome Screen (5 seconds)##
 *  
 *  1. Simple vertical menu, user scroll up or down and select:
 *     a. Slide 1 M to left
 *     b. Slide 1 M to right
 *     c. Slide 2 M to left
 *     d. Slide 2 M to right
 *     e. Customize - user set start and end position (length) and direction automatically calculated
 *  2. Adjust carrier position if necessary, move left or right to proper position (mark on slider?)
 *  3. Set slide duration or runtime in multiply of 5 minutes (fine tune of 1 minute necessary)
 *  4.
 *     a. Execute program - slide 50k or 100k steps in xx minutes
 *     b. Display while program run : remaining time to completion
 *  5. Turn motor power off upon completion to save battery, Flash notification upon completion
 *  
 */

#include <Stepper.h>
#include <LiquidCrystal.h>

// Integers and Variables
unsigned long steps = 3200;  // number of steps per rev at motor shaft (200*16th ms)
int pulleyRev = 16576;       // number of steps per rev at pulley - G5.18: 16576; G16.3: 52160
int lcd_key     = 0;
int adc_key_in  = 0;
int endStep = 1;
int minutes = 5;
long runtime = 0;
unsigned long stepDelay = 0;
unsigned long slider = 0;
unsigned long oneMeter = 186480; //steps per one meter slider - G5.18: 186480; G16.3: 489600

// Initialize LCD library, pin used
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Initialize the stepper library, pin used
Stepper myStepper(steps, 3, 2);

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons() {
    adc_key_in = analogRead(0);                 // read the value from the sensor KENA REFINE LAGI
    if (adc_key_in > 1000) return btnNONE;
    if (adc_key_in < 50)   return btnRIGHT;
    if (adc_key_in < 195)  return btnUP;
    if (adc_key_in < 380)  return btnDOWN;
    if (adc_key_in < 555)  return btnLEFT;
    if (adc_key_in < 790)  return btnSELECT;
    return btnNONE;                             // when all others fail, return this...
}

void setup() {
    // Start LCD
    lcd.begin (16, 2);
    lcd.setCursor(0, 0);
    lcd.print("  KAMs-X Slide "); //v04 "Start angle L/R "
    lcd.setCursor(0, 1);
    lcd.print("  Version 2.10 ");
    delay (3000);
    lcd.clear ();
    delay (250);
    // Turn motor ON
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);    // LOW = enable, HIGH = disable
    myStepper.setSpeed(200);  // speed: 200rpm at motor shaft, equals to 38.6 rpm at pulley
}

void loop() {
    lcd.setCursor(0, 0);                   // setting up starting position.
    lcd.print(" START POSITION ");         // user use keypad to slide left/right to
    lcd.setCursor(0, 1);                   // bring platform to end of slider.
    lcd.print("L/R/Up/Dn -> SEL");         // have no effect on TL runtime program

    while (endStep) {
        lcd_key = read_LCD_buttons();
        switch (lcd_key) {
            case btnRIGHT: {
                lcd.setCursor(0, 0);
                lcd.print("    L/R/Up/Dn   ");
                lcd.setCursor(0, 1);
                lcd.print(" SEL to confirm ");
                myStepper.setSpeed(200);        // laju for fast movement, dir right (CW)
                myStepper.step(800);            // big movement per button push,
                delay(0);                       // continuous button push will result
                break;                          // in continuous movement
            }
            case btnUP: {
                lcd.setCursor(0, 0);            // ditto as above, but at slower speed
                lcd.print("    L/R/Up/Dn   ");  // and smaller movement
                lcd.setCursor(0, 1);            // for fine tuning position (so tak langgar bumpstop)
                lcd.print(" SEL to confirm ");
                myStepper.setSpeed(60);
                myStepper.step(400);
                delay(0);
                break;
            }
            case btnLEFT: {
                lcd.setCursor(0, 0);                // Fast movement, left (CCW)
                lcd.print("    L/R/Up/Dn   ");
                lcd.setCursor(0, 1);
                lcd.print(" SEL to confirm ");
                myStepper.setSpeed(200);
                myStepper.step(-800);
                delay(0);
                break;
            }
            case btnDOWN: {
                lcd.setCursor(0, 0);                // Slow left
                lcd.print("    L/R/Up/Dn   ");
                lcd.setCursor(0, 1);
                lcd.print(" SEL to confirm ");
                myStepper.setSpeed(60);
                myStepper.step(-400);
                delay(0);
                break;
            }
            case btnSELECT: {
                endStep = 0;
                delay(250);
                break;
            }
            case btnNONE: {
                break;
            }
        }
    }

    endStep = 1;                              // user set TL duration
    lcd.setCursor(0, 0);
    lcd.print("  Set Duration  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    while (endStep) {
        lcd_key = read_LCD_buttons();
        lcd.setCursor(0, 1);
        lcd.print("       minutes");
        lcd.setCursor(2, 1);
        lcd.print(minutes);
        delay(50);
        switch (lcd_key) {
            case btnRIGHT: {
                if (minutes < 30) {                // +5 minutes per button push
                    minutes = minutes + 5;
                } else if (minutes < 120) {        // +15 minutes per button push
                    minutes = minutes + 15;
                } else if (minutes < 480) {        // +30 minutes per button push
                    minutes = minutes + 30;
                }
                delay(250);
                break;
            }
            case btnLEFT: {
                if (minutes > 0 && minutes < 35) {       // ditto as above, but minus
                    minutes = minutes - 5;
                } else if (minutes > 0 && minutes < 150) {
                    minutes = minutes - 15;
                } else if (minutes > 0 && minutes < 485) {
                    minutes = minutes - 30;
                }
                delay(250);
                break;
            }
            case btnUP: {
                if (minutes < 30) {                      // +1 for fine tuning time e.g nak buat 51 minutes
                    minutes = minutes + 1;               // means 9x btnRIGHT + 1x btnUP
                } else if (minutes < 120) {
                    minutes = minutes + 1;
                } else if (minutes < 720) {
                    minutes = minutes + 1;
                }
                delay(250);
                break;
            }
            case btnDOWN: {
                if (minutes > 0 && minutes < 35) {
                    minutes = minutes - 1;
                } else if (minutes > 0 && minutes < 150) {
                    minutes = minutes - 1;
                } else if (minutes > 0 && minutes < 725) {
                    minutes = minutes - 1;
                }
                delay(250);
                break;
            }
            case btnSELECT: {
                endStep = 0;
                delay(250);
                break;
            }
            case btnNONE: {
                break;
            }
        }
    }
  
    runtime = (minutes * 60000);        // TL runtime program
    lcd.setCursor(0,0);
    lcd.print("Time Remaining: ");
    lcd.setCursor(0,1);
    lcd.print("        Minutes ");

    slider = (oneMeter);              // slide length. replace with twoMeter for 2 meters slide
    stepDelay = (runtime / slider);   // speed. calculate delay between steps by dividing time/length
    while (slider) {
        myStepper.setSpeed(200);        // maybe not necessary, but i put it here just in case
        myStepper.step(1);              // direction of slide. +ve for right (CW); -ve for left (CCW) everything else sama
        runtime = (runtime - stepDelay);
        lcd.setCursor(2, 1);
        lcd.print((runtime / 60000));
        slider--;
        delay(stepDelay);
    }
    endStep = 0;
    // turn motor OFF 
    // still tak jadi. motor still enabled. need to turn off to save battery
    pinMode(13,OUTPUT);
    digitalWrite(13,LOW);
    
    while (1) {
        lcd.setCursor(0, 0);              // end of TL notification
        lcd.print(" Timelapse Done! ");   // flash berganti-ganti dengan line bawah
        lcd.setCursor(0, 1);
        lcd.print("                 ");
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("                 ");
        lcd.setCursor(0, 1);
        lcd.print("RESET to restart ");
        delay(500);
    }
}


