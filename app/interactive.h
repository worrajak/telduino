#ifndef INTERACTIVEMODE_H
#define INTERACTIVEMODE_H

//For interactive mode
extern int _testChannel; //This is the input daughter board channel. This should only b

void parseBerkeley();
void displayEnabled(const int8_t enabledC[NSWITCHES]);
int8_t getChannelID();
void printSDCardInfo();

//Hacked up test
extern int32_t switchSec;
extern int32_t testIdx;
void testCircuitPrint();


//IO
void printTableStrings(const char *strs[], int8_t len);
void printIRMSVRMSZX( int channel );
    
//Configuration of Circuits
void setCircuitParameter();

#endif
