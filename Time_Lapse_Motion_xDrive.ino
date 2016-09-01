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

String menuItems[] = {"1M Dir RIGHT", "1M Dir LEFT", "2M Dir RIGHT", "2M Dir LEFT", "Customise"};

// Navigation button variables
int readKey;
int savedDistance = 0;

// Menu control variables
int menuPage = 0;
int maxMenuPages = round(((sizeof(menuItems) / sizeof(String)) / 2) + .5);
int cursorPosition = 0;

// Creates 3 custom characters for the menu display
byte downArrow[8] = {
	0b00100, //   *
	0b00100, //   *
	0b00100, //   *
	0b00100, //   *
	0b00100, //   *
	0b10101, // * * *
	0b01110, //  ***
	0b00100  //   *
};

byte upArrow[8] = {
	0b00100, //   *
	0b01110, //  ***
	0b10101, // * * *
	0b00100, //   *
	0b00100, //   *
	0b00100, //   *
	0b00100, //   *
	0b00100  //   *
};

byte menuCursor[8] = {
	B01000, //  *
	B00100, //   *
	B00010, //    *
	B00001, //     *
	B00010, //    *
	B00100, //   *
	B01000, //  *
	B00000  //
};

#include <Wire.h>
#include <LiquidCrystal.h>
#include <Stepper.h>

// integers and variables
unsigned long steps = 3200;  // number of steps per rev at motor shaft (200*16th ms)
int pulleyRev = 16576;        // number of steps per rev at pulley (3200*5.18 gear)
int lcd_key     = 0;
int adc_key_in  = 0;
int endStep = 1;
int minutes = 5;
long runtime = 0;
unsigned long stepDelay = 0;
unsigned long slider = 0;
unsigned long oneMeter = 186480; //steps per one meter slider
//unsigned long twoMeter = 372,960 //steps per two meter slider, NOT CONFIRMED

// Setting the LCD shields pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// initialize the stepper library, pin used
Stepper myStepper(steps, 3, 2);

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons()
{
	adc_key_in = analogRead(0);      // read the value from the sensor KENA REFINE LAGI
	if (adc_key_in > 1000) return btnNONE;
	if (adc_key_in < 50)   return btnRIGHT;
	if (adc_key_in < 195)  return btnUP;
	if (adc_key_in < 380)  return btnDOWN;
	if (adc_key_in < 555)  return btnLEFT;
	if (adc_key_in < 790)  return btnSELECT;
	return btnNONE;  // when all others fail, return this...
}


void setup() {

// Initializes serial communication
//  Serial.begin(9600);

	/* Initializes and clears the LCD screen
	  lcd.begin(16, 2);
	  lcd.clear(); */
//start LCD
	lcd.begin (16, 2);
	lcd.setCursor(0, 0);
	lcd.print(" KAMs-X Slide "); //v04 "Start angle L/R "
	lcd.setCursor(0, 1);
	lcd.print(" Version 2.3");
	delay (3000);
	lcd.clear ();
	delay (250);

	lcd.createChar(0, menuCursor);
	lcd.createChar(1, upArrow);
	lcd.createChar(2, downArrow);
}

void loop() {
	mainMenuDraw();
	drawCursor();
	operateMainMenu();
}

// This function will generate the 2 menu items that can fit on the screen. They will change as you scroll through your menu. Up and down arrows will indicate your current menu position.
void mainMenuDraw() {
	Serial.print(menuPage);
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print(menuItems[menuPage]);
	lcd.setCursor(1, 1);
	lcd.print(menuItems[menuPage + 1]);
	if (menuPage == 0) {
		lcd.setCursor(15, 1);
		lcd.write(byte(2));
	} else if (menuPage > 0 and menuPage < maxMenuPages) {
		lcd.setCursor(15, 1);
		lcd.write(byte(2));
		lcd.setCursor(15, 0);
		lcd.write(byte(1));
	} else if (menuPage == maxMenuPages) {
		lcd.setCursor(15, 0);
		lcd.write(byte(1));
	}
}

// When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.
void drawCursor() {
	for (int x = 0; x < 2; x++) {     // Erases current cursor
		lcd.setCursor(0, x);
		lcd.print(" ");
	}

	// The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
	// in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
	if (menuPage % 2 == 0) {
		if (cursorPosition % 2 == 0) {  // If the menu page is even and the cursor position is even that means the cursor should be on line 1
			lcd.setCursor(0, 0);
			lcd.write(byte(0));
		}
		if (cursorPosition % 2 != 0) {  // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
			lcd.setCursor(0, 1);
			lcd.write(byte(0));
		}
	}
	if (menuPage % 2 != 0) {
		if (cursorPosition % 2 == 0) {  // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
			lcd.setCursor(0, 1);
			lcd.write(byte(0));
		}
		if (cursorPosition % 2 != 0) {  // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
			lcd.setCursor(0, 0);
			lcd.write(byte(0));
		}
	}
}


void operateMainMenu() {
	int activeButton = 0;
	while (activeButton == 0) {
		int button;
		readKey = analogRead(0);
		if (readKey < 790) {
			delay(100);
			readKey = analogRead(0);
		}
		button = evaluateButton(readKey);
		switch (button) {
		case 0: // When button returns as 0 there is no action taken
			break;
		case 1:  // This case will execute if the "forward" button is pressed
			button = 0;
			switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
			case 0:
				menuItem1();
				break;
			case 1:
				menuItem2();
				break;
			case 2:
				menuItem3();
				break;
			case 3:
				menuItem4();
				break;
			case 4:
				menuItem5();
				break;
				/*          case 5:
				            menuItem6();
				            break;
				          case 6:
				            menuItem7();
				            break;
				          case 7:
				            menuItem8();
				            break;
				          case 8:
				            menuItem9();
				            break;
				          case 9:
				            menuItem10();
				            break; */
			}
			activeButton = 1;
			mainMenuDraw();
			drawCursor();
			break;
		case 2:
			button = 0;
			if (menuPage == 0) {
				cursorPosition = cursorPosition - 1;
				cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
			}
			if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
				menuPage = menuPage - 1;
				menuPage = constrain(menuPage, 0, maxMenuPages);
			}

			if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
				menuPage = menuPage - 1;
				menuPage = constrain(menuPage, 0, maxMenuPages);
			}

			cursorPosition = cursorPosition - 1;
			cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

			mainMenuDraw();
			drawCursor();
			activeButton = 1;
			break;
		case 3:
			button = 0;
			if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
				menuPage = menuPage + 1;
				menuPage = constrain(menuPage, 0, maxMenuPages);
			}

			if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
				menuPage = menuPage + 1;
				menuPage = constrain(menuPage, 0, maxMenuPages);
			}

			cursorPosition = cursorPosition + 1;
			cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
			mainMenuDraw();
			drawCursor();
			activeButton = 1;
			break;
		}
	}
}

// This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
int evaluateButton(int x) {
	int result = 0;
	if (x < 50) {
		result = 1; // right
	} else if (x < 195) {
		result = 2; // up
	} else if (x < 380) {
		result = 3; // down
	} else if (x < 555) {  //790
		result = 4; // left
	} else if (x < 790) {
		result = 5; // SELECT
	}

	return result;
}

// If there are common usage instructions on more than 1 of your menu items you can call this function from the sub
// menus to make things a little more simplified. If you don't have common instructions or verbage on multiple menus
// I would just delete this void. You must also delete the drawInstructions()function calls from your sub menu functions.
void drawInstructions() {
	lcd.setCursor(0, 1); // Set cursor to the bottom line
	lcd.print("Use ");
	lcd.write(byte(1)); // Up arrow
	lcd.print("/");
	lcd.write(byte(2)); // Down arrow
	lcd.print(" buttons");
}

void menuItem1()
{
	// turn motor ON
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);    // LOW = enable, HIGH = disable
	myStepper.setSpeed(200);  // speed: 200rpm at motor shaft, equals to 38.6 rpm at pulley

// Creates the byte for the 3 custom characters
	lcd.setCursor(0, 0);                   // setting up starting position.
	lcd.print(" START POSITION ");         // user use keypad to slide left/right to
	lcd.setCursor(0, 1);                   // bring platform to end of slider.
	lcd.print("L/R/Up/Dn -> SEL");         // have no effect on TL runtime program

	while (endStep)
	{
		lcd_key = read_LCD_buttons();
		switch (lcd_key)
		{
		case btnRIGHT:
		{
			lcd.setCursor(0, 0);
			lcd.print("    L/R/Up/Dn   ");
			lcd.setCursor(0, 1);
			lcd.print(" SEL to confirm ");
			myStepper.setSpeed(200);        // laju for fast movement, dir right (CW)
			myStepper.step(400);            // big movement per button push,
			delay(0);                       // continuous button push will result
			break;                          // in continuous movement
		}
		case btnUP:
		{
			lcd.setCursor(0, 0);            // ditto as above, but at slower speed
			lcd.print("    L/R/Up/Dn   ");  // and smaller movement
			lcd.setCursor(0, 1);            // for fine tuning position (so tak langgar bumpstop)
			lcd.print(" SEL to confirm ");
			myStepper.setSpeed(60);
			myStepper.step(400);
			delay(0);
			break;
		}
		case btnLEFT:
		{
			lcd.setCursor(0, 0);                // Fast movement, left (CCW)
			lcd.print("    L/R/Up/Dn   ");
			lcd.setCursor(0, 1);
			lcd.print(" SEL to confirm ");
			myStepper.setSpeed(200);
			myStepper.step(-400);
			delay(0);
			break;
		}
		case btnDOWN:
		{
			lcd.setCursor(0, 0);                // Slow left
			lcd.print("    L/R/Up/Dn   ");
			lcd.setCursor(0, 1);
			lcd.print(" SEL to confirm ");
			myStepper.setSpeed(60);
			myStepper.step(-400);
			delay(0);
			break;
		}
		case btnSELECT:
		{
			endStep = 0;
			delay(250);
			break;
		}
		case btnNONE:
		{
			break;
		}
		}
	}

	endStep = 1;                              // user set TL duration
	lcd.setCursor(0, 0);
	lcd.print("  Set Duration  ");
	lcd.setCursor(0, 1);
	lcd.print("                ");

	while (endStep)
	{
		lcd_key = read_LCD_buttons();
		lcd.setCursor(0, 1);
		lcd.print("       minutes");
		lcd.setCursor(2, 1);
		lcd.print(minutes);
		delay(50);
		switch (lcd_key)
		{
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
		case btnSELECT:
		{
			endStep = 0;
			delay(250);
			break;
		}
		case btnNONE:
		{
			break;
		}
		}
	}

	runtime = (minutes * 60000);        // TL runtime program
	lcd.setCursor(0, 0);
	lcd.print("Time Remaining: ");
	lcd.setCursor(0, 1);
	lcd.print("        Minutes ");

	slider = (oneMeter);              // slide length. replace with "twoMeter" for 2 meters slide
	stepDelay = (runtime / slider);   // speed. calculate delay between steps by dividing time/length
	while (slider)
	{
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
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);

	while (1)
	{
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

void menuItem2() { // Function executes when you select the 2nd item from main menu
	int activeButton = 0;

	lcd.clear();
	lcd.setCursor(3, 0);
	lcd.print("Sub Menu 2");

	while (activeButton == 0) {
		int button;
		readKey = analogRead(0);
		if (readKey < 790) {
			delay(100);
			readKey = analogRead(0);
		}
		button = evaluateButton(readKey);
		switch (button) {
		case 4:  // This case will execute if the "back" button is pressed
			button = 0;
			activeButton = 1;
			break;
		}
	}
}

void menuItem3() { // Function executes when you select the 3rd item from main menu
	int activeButton = 0;

	lcd.clear();
	lcd.setCursor(3, 0);
	lcd.print("Sub Menu 3");

	while (activeButton == 0) {
		int button;
		readKey = analogRead(0);
		if (readKey < 790) {
			delay(100);
			readKey = analogRead(0);
		}
		button = evaluateButton(readKey);
		switch (button) {
		case 4:  // This case will execute if the "back" button is pressed
			button = 0;
			activeButton = 1;
			break;
		}
	}
}

void menuItem4() { // Function executes when you select the 4th item from main menu
	int activeButton = 0;

	lcd.clear();
	lcd.setCursor(3, 0);
	lcd.print("Sub Menu 4");

	while (activeButton == 0) {
		int button;
		readKey = analogRead(0);
		if (readKey < 790) {
			delay(100);
			readKey = analogRead(0);
		}
		button = evaluateButton(readKey);
		switch (button) {
		case 4:  // This case will execute if the "back" button is pressed
			button = 0;
			activeButton = 1;
			break;
		}
	}
}

void menuItem5() { // Function executes when you select the 5th item from main menu
	int activeButton = 0;

	lcd.clear();
	lcd.setCursor(3, 0);
	lcd.print("Sub Menu 5");

	while (activeButton == 0) {
		int button;
		readKey = analogRead(0);
		if (readKey < 790) {
			delay(100);
			readKey = analogRead(0);
		}
		button = evaluateButton(readKey);
		switch (button) {
		case 4:  // This case will execute if the "back" button is pressed
			button = 0;
			activeButton = 1;
			break;
		}
	}
}

/*   5 item je
void menuItem6() { // Function executes when you select the 6th item from main menu
  int activeButton = 0;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 6");
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
void menuItem7() { // Function executes when you select the 7th item from main menu
  int activeButton = 0;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 7");
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
void menuItem8() { // Function executes when you select the 8th item from main menu
  int activeButton = 0;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 8");
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
void menuItem9() { // Function executes when you select the 9th item from main menu
  int activeButton = 0;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 9");
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
void menuItem10() { // Function executes when you select the 10th item from main menu
  int activeButton = 0;
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 10");
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
*/


