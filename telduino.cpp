#include "util/delay.h"
#include "avr/io.h"
#include "avr/signal.h"
#include "WProgram.h"

char ctrlz = 26;

void setup();
void loop();
void echo_serial3_serial0();

void setup() {
	
	Serial.begin(9600);
	Serial3.begin(9600);
    
	Serial.print("\r\n");
	Serial.print("\r\n");
	Serial.print("telduino start\r\n");
	
	Serial3.print("AT+CMGF=1\r\n");
	echo_serial3_serial0();

	Serial3.print("AT+CSQ\r\n");
	echo_serial3_serial0();	
	
	Serial.print("message start\r\n");
	Serial3.print("AT+CMGS=8323776861\r\n");
	echo_serial3_serial0();
	Serial3.print("message from columbia telduino ");
	Serial3.println(__TIME__);
	echo_serial3_serial0();
	Serial3.write(ctrlz);
	echo_serial3_serial0();
	
}

void loop() {
}

void echo_serial3_serial0(){
	_delay_ms(250);
	while (Serial3.available()){
		Serial.print(Serial3.read(), BYTE);
	}	
}