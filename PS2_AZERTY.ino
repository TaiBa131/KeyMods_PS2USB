/*
 * PS2 Keyboard to USB Convertor - Pro Micro 5V 16Mhz
 * This sketch allows to convert PS2 to USB.
 *
 * It's currently on AZERTY, but the PS2Keycodes.h
 * file can be edited to change the keymap
 *
 * The default Data pin is 2, the default Clock pin is 3.
 *
 *
 * It was done in the context of a school project with the Subject:
 *      "Recycling, Upcycling, Zero Waste"
 *
 *
 * Sites used:
 * - https://www.tutorialspoint.com/cprogramming/c_data_types.htm
 * - http://www.pyroelectro.com/tutorials/ps2_keyboard_interface/theory.html //PS2 Data Output
 * - http://www.pyroelectro.com/tutorials/ps2_keyboard_interface/theory_ps2.html
 * - https://techdocs.altium.com/display/FPGA/PS2+Keyboard+Scan+Codes
 * - https://wiki.osdev.org/PS/2_Keyboard
 * - https://cdn.sparkfun.com/assets/f/d/8/0/d/ProMicro16MHzv2.pdf
 * - http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf
 * - http://www.electronics-base.com/general-description/communication/111-the-ps2-protocol-used-by-mousekeyboard
 * - http://www.burtonsys.com/ps2_chapweske.htm
 * - https://github.com/techpaul/PS2KeyAdvanced/blob/master/src/PS2KeyAdvanced.cpp // C switch cases, but not the code i wanted.
 */

// This is later include the PS2 Scan codes to convert scancodes to (special) characters
//#include "PS2Keycodes.h"

// Defining Pins (Data + Clock)
const char dtPin 2;
const char clkPin 3;
uint8_t nextKey=0;

///////////
// SETUP //
///////////

void setup() {
	pinMode(dtPin, INPUT);
	pinMode(clkPin, INPUT);
	attachInterrupt(digitalPinToInterrupt(3), ps2Interrupt, FALLING);

	Serial.begin(9600);
}

//////////
// VOID //
//////////

void loop() {
	if(nextKey) {
	Serial.println(nextKey);
	nextKey = 0;
	}
}

///////////////////
// PS2 Interrupt //
///////////////////

void ps2Interrupt() {
	static uint_8t val=0, bitCount=0;
	bool recievedBit;

	recievedBit = digitalRead(dtPin);

	bitCount++;
	switch(bitCount) {
		// Start
		case 1:
			//Start bit?
			break;
		// Data
		case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
			//First bit?
			val << 1;		// Shift left
			val |= recievedBit;	// OR (val= val | recievedBit)
			break;
		// Parity
		case 10:
			break;
		// Stop
		case 11:
			Serial.print("\n Decimal: ");
			Serial.println(val, DEC) //print scan code in decimal
			Serial.print("Binary: ");
			Serial.println(val, BYTE) //print scan code in binary
			break;
	// put your interrupt code here, runs when PS2 device interrupts
}
