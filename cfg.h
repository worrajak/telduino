#ifndef CFG_H
#define CFG_H

#include <avr/eeprom.h>
#include "Circuit/circuit.h"

/** select.h contains lower level hardware configuration information 
 * like how many circuits there are*/
#include "Select/select.h"


/** Definition of serial ports for debug, cpu communication, and telit communication
    @warning ensure consistence with the print, printD, printC, macros.
*/
#define dbg Serial
#define cpu Serial
//#define cpu Serial2
#define mdm Serial3
#define DEBUG_BAUD_RATE 9600
#define SHEEVA_BAUD_RATE 9600
#define TELIT_BAUD_RATE 9600


#define MAINS 0
extern int16_t reportInterval;  /** How often to report in seconds */
extern int8_t mode;             /** 0 emergency, 1 interactive, 2 meter */
#define EMERGENCYMODE 0
#define INTERACTIVEMODE 1
#define METERMODE 2

extern Circuit ckts[NCIRCUITS];
extern Circuit EEMEM cktsSave[NCIRCUITS];
            
extern char serBuff[128];
#endif
