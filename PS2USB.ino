/*
 * PS2 Keyboard to USB Convertor - Pro Micro 5V 16Mhz
 * This sketch allows to convert PS2 to USB.
 *
 * Country keyboard layouts don't matter, as the keyboard
 * only send keycodes, and those get translated to USB.
 * That work is done on the Host USB device.
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
 *
 * - https://binaryupdates.com/bitwise-operations-in-embedded-programming/ // Bitwise 2
 * - https://wiki.osdev.org/PS/2_Keyboard // Keycodes Template - Scan Code Set 2
 * - https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers // Crappy HID Arduino library
 * - https://github.com/kolsys/PS2USBConverter/blob/master/PS2USBConverter.ino // Keycodes, Scancode "Manager", Loop, Reports
 * - https://www.youtube.com/watch?v=1unTKKGd8qs // Amazing video about how USB HID works - Sparkfun 
 * - https://www.win.tue.nl/~aeb/linux/kbd/scancodes-14.html // USB HID codes
 * - https://web.archive.org/web/20150316084844/http://retired.beyondlogic.org/keyboard/keybrd.htm // AT / PS2 Keyboard documentation
 *
 * Source:
 * - own code
 * - Research from sites used
 * - https://github.com/kolsys/PS2USBConverter/blob/master/PS2USBConverter.ino ("PS2 Library", as there was no good one)
 * 
 */

// Additional Keyboard library is necessary,
// with modification of the Keyboard.h file
#include "Keyboard.h"

// Defining Pins (Data + Clock)
const char dtPin=2;
const char clkPin=3;

uint8_t K[255], KE[255];

#define BUFFER 45

KeyReport report;
static volatile unsigned char buffer[BUFFER];
static volatile unsigned char head = 0, tail = 0;
static volatile unsigned char sendBits = 0, msg, sendBitCount, setBits;
unsigned char LEDs, skip;
boolean spc, rel, sendLedStatus;

///////////
// SETUP //
///////////

void setup() {
	pinMode(dtPin, INPUT);
	pinMode(clkPin, INPUT);
	attachInterrupt(digitalPinToInterrupt(3), ps2Interrupt, FALLING);

	Keyboard.begin();
	setupKeycodes();
	delay(1000); // Give time for the USB HID to initialize
}

//////////
// LOOP //
//////////

void loop() {
	unsigned char scanCode = getScancode(), key2;
	if(scanCode) {
		if(skip) skip--;
		else {
			if(scanCode == 0xE0) spc = true; // Special (Multimedia, etc.) Key
			else if(scanCode == 0xF0) rel = true; // Key Released
			else if(scanCode == 0xFA) { // Acknowledgement of command
				if(sendLedStatus) { // If we wanted to send a LED command, we send it after ACK from Keyboard
					sendLedStatus = false;
					sendMessage(LEDs); // Send current LEDs 
				}
			} else {
				if(scanCode == 0xE1) { // Start Byte of PS2 Scancode for press of Pause key
					key2 = 72; // USB HID scancode for Pause
					skip = 7; // See "PS2 Library" for this solution, allows to skip the useless keycodes
					rel = true;
					reportAdd(key2); // Add pressed Key to report
					Keyboard.sendReport(&report); // send report to Host
				} else {
					key2 = spc ? KE[scanCode] : K[scanCode]; // Getting single scancode
				}

				if(key2) {
					if(rel) {
						reportRemove(key2); // Stop announcing key to Host

						if (key2 == 83 || key2 == 71 || key2 == 57) {
							sendLedStatus = true;

							if(key2 == 83) LEDs ^= 2;		// XOR - Note: "Bit Toggle"
							else if(key2 == 71) LEDs ^= 1;	// XOR
							else if(key2 == 57) LEDs ^= 4;	// XOR

							sendMessage(0xED); // Set LEDs command Host to Keyboard
						}
					} else reportAdd(key2);
					Keyboard.sendReport(&report);
				}

				spc = false;
				rel = false;
			}
		}
	}
}

//////////////////////
// HOST TO KEYBOARD //
//////////////////////

void sendMessage(unsigned char message) {
	noInterrupts(); // Disables Interrupts

	pinMode(clkPin, OUTPUT);
	digitalWrite(clkPin, LOW); // Initiate/Announce Host to Keyboard communication
	delayMicroseconds(60); // Wait for Keyboard to register
	pinMode(clkPin, INPUT); // Give clock back to Keyboard;

	msg = message;
	sendBitCount = 0;
	sendBits = 12;
	setBits = 0;

	pinMode(dtPin, OUTPUT);
	digitalWrite(dtPin, LOW);
	interrupts(); // Wait on first next falling edge
}


//////////////////
// PS2 Scancode //
//////////////////

// Source: PS2 Library: https://github.com/PaulStoffregen/PS2Keyboard/blob/master/PS2Keyboard.cpp

unsigned char getScancode() {
	unsigned char c, i;

	i = tail;
	if(i == head) return 0;
	i++;
	if(i >= BUFFER) i = 0;
	c = buffer[i];
	tail = i;
	Serial.println(c);
	return c;
}

//////////////////////////////
// USB HID Keyboard Reports //
//////////////////////////////

// Source: Keyboard.h file

void reportAdd(unsigned char key) {
	if(key >= 224) report.modifiers |= 1 << (key - 224); // Check if key is a Modifier Key (CTRL, etc.)
        else if (report.keys[0] != key && report.keys[1] != key && // Checking if key isn't already pressed
             report.keys[2] != key && report.keys[3] != key &&
             report.keys[4] != key && report.keys[5] != key) {
		for (int i = 0; i < 6; ++i) {
			if (report.keys[i] == 0) { // Find empty key in report
				report.keys[i] = key;
				break;
			}
		}
	}
}

// Source: Keyboard.h file

void reportRemove(unsigned char key) {
	if (key >= 224) report.modifiers &= ~(1 << (key - 224));
	else {
		for (int i = 0; i < 6; ++i) {
			if (report.keys[i] == key) {
				report.keys[i] = 0;
				break;
			}
		}
	}
}

///////////////////
// PS2 Interrupt //
///////////////////

void ps2Interrupt() {
	static unsigned char val=0,bitCount=0;
	static long prevMs=0;
	long nowMs;
	bool recievedBit;



	if(!sendBits) {
		recievedBit = digitalRead(dtPin);

		nowMs = millis();
		if (nowMs - prevMs > 250) {
			bitCount = 0;
			val = 0;
		}
		prevMs = nowMs;

		bitCount++;
		switch(bitCount) {
			// Start
			case 1:
				//Start bit?
				break;
			// Data
			case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
				//First bit?
				val |= (recievedBit << (bitCount-2));
				break;
			// Parity
			case 10:
				break;
			// Stop
			case 11:

				Serial.println(val, HEX);

				unsigned char i = head + 1;
				if(i >= BUFFER) i = 0;
				if(i != tail) {
					buffer[i] = val;
					head = i;
				}

				bitCount = 0;
				val = 0;
				break;
			default:
				bitCount = 0;
				val = 0;
				break;
		}
	} else {
		sendBits--;
		uint8_t b = sendBitCount - 1; // Adjusted Bitcount for binary operations

		if(b == 8) {
			digitalWrite(dtPin, !(setBits & 1)); // Parity bit
		} else if(b == 10) {
			pinMode(dtPin, INPUT); // Accepting data from Keyboard again
		} else if(b < 9) {
			bool bt = (msg >> b) & 1; // Desecting message in bits
			digitalWrite(dtPin, bt); // Sending bit
			setBits += bt;
		}

		sendBitCount++;
	}
}

//////////////////////////////////
// Keycodes - Adapted to AZERTY //
//////////////////////////////////

void setupKeycodes(){
	K[0x1C] = 4;
	K[0x32] = 5;
	K[0x21] = 6;
	K[0x23] = 7;
	K[0x24] = 8;
	K[0x2B] = 9;
	K[0x34] = 10;
	K[0x33] = 11;
	K[0x43] = 12;
	K[0x3B] = 13;
	K[0x42] = 14;
	K[0x4B] = 15;
	K[0x3A] = 16;
	K[0x31] = 17;
	K[0x44] = 18;
	K[0x4D] = 19;
	K[0x15] = 20;
	K[0x2D] = 21;
	K[0x1B] = 22;
	K[0x2C] = 23;
	K[0x3C] = 24;
	K[0x2A] = 25;
	K[0x1D] = 26;
	K[0x22] = 27;
	K[0x35] = 28;
	K[0x1A] = 29;
	K[0x45] = 39;
	K[0x16] = 30;
	K[0x1E] = 31;
	K[0x26] = 32;
	K[0x25] = 33;
	K[0x2E] = 34;
	K[0x36] = 35;
	K[0x3D] = 36;
	K[0x3E] = 37;
	K[0x46] = 38;
	K[0x0E] = 53;
	K[0x4E] = 45;
	K[0x55] = 46;
	K[0x5D] = 49;
	K[0x66] = 42;
	K[0x29] = 44;
	K[0x0D] = 43;
	K[0x58] = 57; // Caps Lock, change to 227 to get a Windows key if you're using a keyboard without one
	K[0x12] = 225;
	K[0x14] = 224;
	K[0x11] = 226;
	K[0x59] = 229;
	K[0x5A] = 40;
	K[0x76] = 41;
	K[0x05] = 58;
	K[0x06] = 59;
	K[0x04] = 60;
	K[0x0C] = 61;
	K[0x03] = 62;
	K[0x0B] = 63;
	K[0x83] = 64;
	K[0x0A] = 65;
	K[0x01] = 66;
	K[0x09] = 67;
	K[0x78] = 68;
	K[0x07] = 69;
	K[0x7E] = 71; // Scroll Lock, change to 58 if you want Caps Lock (if youre using Caps Lock as Windows key)
	K[0x54] = 47;
	K[0x77] = 83;
	K[0x7C] = 85;
	K[0x7B] = 86;
	K[0x79] = 87;
	K[0x71] = 99;
	K[0x70] = 98;
	K[0x69] = 89;
	K[0x72] = 90;
	K[0x7A] = 91;
	K[0x6B] = 92;
	K[0x73] = 93;
	K[0x74] = 94;
	K[0x6C] = 95;
	K[0x75] = 96;
	K[0x7D] = 97;
	K[0x5B] = 48;
	K[0x4C] = 51;
	K[0x52] = 52;
	K[0x41] = 54;
	K[0x49] = 55;
	K[0x4A] = 56;
	K[0x61] = 100;

	KE[0x1F] = 227;
	KE[0x14] = 228;
	KE[0x27] = 231;
	KE[0x11] = 230;
	KE[0x2F] = 101;
	KE[0x7c] = 70;
	KE[0x70] = 73;
	KE[0x6C] = 74;
	KE[0x7D] = 75;
	KE[0x71] = 76;
	KE[0x69] = 77;
	KE[0x7A] = 78;
	KE[0x75] = 82;
	KE[0x6B] = 80;
	KE[0x72] = 81;
	KE[0x74] = 79;
	KE[0x4A] = 84;
	KE[0x5A] = 88;
}
