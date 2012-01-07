#include <avr/eeprom.h>

#include "ReturnCode/returncode.h"
#include "ADE7753/ADE7753.h"
#include "Select/select.h"
#include "Switches/switches.h"
#include "circuit.h"
#include "arduino/HardwareSerial.h"

#define dbg Serial
#define max(X,Y) ((X)>=(Y))?(X):(Y)
#define ERRCHECKRETURN(Cptr) if (_shouldReturn(Cptr)) return;

// TODO make switch wait time a parameter
// TODO Have fault detection code check interrupts for sag detection and frequency variation

int8_t _shouldReturn(Circuit *c) 
{
    ifnsuccess(_retCode) {            
        if (_retCode == COMMERR) {        
            c->status |= COMM;            
            CSselectDevice(DEVDISABLE);    
            return true;                        
        } else if (_retCode == TIMEOUT) {
            //Do Nothing
        } else {                        
            CSselectDevice(DEVDISABLE);    
            return true;                        
        }                                
    }
    return false;
}
/** 
  Updates circuit measured parameters
  @warning a communications error may leave Circuit *c in an inconsisent state.
  @warning The completion time of this function is dependent on the frequency of the line as well as halfCyclesSample. At worst the function will take one minute to return if halfCyclesSample is 1400 and the frequency drops below 40hz.
  @return ARGVALUEERR if the circuitID is invalid
  @return COMMERR if there is a communications error or the ADE is not detected
*/
void Cmeasure(Circuit *c)
{
    int32_t regData;
    int8_t timeout = false;
    CSselectDevice(c->circuitID);                       ERRCHECKRETURN(c);

    //Check for presence and clear the interrupt register
    ADEgetRegister(RSTSTATUS,&regData);                 ERRCHECKRETURN(c);
    c->status &= ~COMM;
    c->status &= 0xFFFF0000;
    c->status |= (0x0000FFFF&regData);

    //Start measuring
    ADEgetRegister(PERIOD,&regData);                    ERRCHECKRETURN(c);
    c->periodus = regData*22/10; //2.2us/bit
    dbg.print("Peruiodus:");dbg.println(c->periodus,DEC);

    uint16_t waitTime = (uint16_t)((regData*22/100)*(c->halfCyclesSample/100));
    waitTime = waitTime + waitTime/2;//Wait at least 1.5 times the amount of time it takes for halfCycleSample halfCycles to occur
    dbg.print("waitTime:"); dbg.println(waitTime,DEC);
    //uint16_t waitTime = 2*1000*c->halfCyclesSample/max(c->frequency,40);

    ADEwaitForInterrupt(CYCEND,waitTime);               ERRCHECKRETURN(c);
    //The failure may have occured because there was no interrupt
    if (_retCode == TIMEOUT) {
        timeout = true;
    }

    if (!timeout) {
        //Apparent power or Volt Amps
        ADEgetRegister(LVAENERGY,&regData);             ERRCHECKRETURN(c);
        c->VA = regData*c->VAEslope/(c->halfCyclesSample/2*c->periodus/1000);  //Watts

        //Active power or watts
        ADEgetRegister(LAENERGY,&regData);              ERRCHECKRETURN(c);
        c->W = regData/(c->halfCyclesSample/2*c->periodus/1000);

        //IRMS
        ADEgetRegister(IRMS,&regData);                  ERRCHECKRETURN(c);
        c->IRMS = regData*c->IRMSslope;

        //VRMS
        ADEgetRegister(VRMS,&regData);                  ERRCHECKRETURN(c);
        c->VRMS= regData*c->VRMSslope;

        //Apparent energy in joules accumulated since last query
        ADEgetRegister(RVAENERGY,&regData);             ERRCHECKRETURN(c);
        c->VAEnergy = regData*c->VAEslope/1000;

        //Actve energy accumulated since last query
        ADEgetRegister(RAENERGY,&regData);              ERRCHECKRETURN(c);
        c->WEnergy = regData*c->VAEslope/1000;

        //Power Factor PF
        if (c->VAEnergy != 0){ 
            c->PF = (uint16_t)(((((uint64_t)c->WEnergy<<16)-c->WEnergy)-c->WEnergy)/c->VAEnergy);
        } else {
            c->PF = 65535;
        }
    } //end if (!timeout)

    CSselectDevice(DEVDISABLE);

    if (timeout) {
        _retCode = TIMEOUT;
    }
}

void Cprogram(Circuit *c)
{
    int32_t regData;
    _retCode = SUCCESS;
    CSselectDevice(c->circuitID);                       ERRCHECKRETURN(c);

    ADEreset();

    if (c->sagDurationCycles > 0) { 
        regData = c->sagDurationCycles + 1;
        ADEsetModeBit(DISSAG,false);                    ERRCHECKRETURN(c);
        ADEsetRegister(SAGCYC,&regData);                ERRCHECKRETURN(c);
    } else {
        ADEsetModeBit(DISSAG,true);                     ERRCHECKRETURN(c);
    }
    regData = c->phcal;                                 ERRCHECKRETURN(c);
    ADEsetRegister(PHCAL,&regData);

    ADEsetCHXOS(1,&c->chIint,&c->chIos);                ERRCHECKRETURN(c);
    regData = c->IRMSoffset;
    ADEsetRegister(IRMSOS, &regData);                   ERRCHECKRETURN(c);
    //since this is channel 2 c->chIint is ignored
    ADEsetCHXOS(2,&c->chIint,&c->chVos);                ERRCHECKRETURN(c);
    regData = c->VRMSoffset;
    ADEsetRegister(VRMSOS, &regData);                   ERRCHECKRETURN(c);
    if (c->halfCyclesSample> 0) {
        regData = c->halfCyclesSample;
        ADEsetRegister(LINECYC,&regData);               ERRCHECKRETURN(c);
        ADEsetModeBit(CYCMODE,true);                    ERRCHECKRETURN(c);
    } else {
        ADEsetModeBit(CYCMODE,false);                   ERRCHECKRETURN(c);
    }
    regData = c->chIgainExp | (c->chVgainExp<<5) | (c->chVscale<<3);
    ADEsetRegister(GAIN,&regData);                      ERRCHECKRETURN(c);
    
    CSselectDevice(DEVDISABLE);
}

void CsetOn(Circuit *c, int8_t on) 
{
    if (CisOn(c) != on) {
        ADEwaitForInterrupt(ZX0,10);
    }
    SWset(c->circuitID,on);
}

int8_t CisOn(Circuit *c) 
{
    return SWisOn(c->circuitID);
}

void Cload(Circuit *c, Circuit* addrEEPROM)
{
    eeprom_read_block(c,(uint8_t*)addrEEPROM,sizeof(Circuit));
}
void Csave(Circuit *c, Circuit* addrEEPROM) 
{
    eeprom_update_block(c,(uint8_t*)addrEEPROM,sizeof(Circuit));
}

/** 
    Reasonable default values for Circuit.
    @warning Does not program the ADE7753s
  */
void CsetDefaults(Circuit *c, int8_t circuitID) 
{
    c->circuitID = circuitID;
    c->halfCyclesSample = 120;

    // Current Parameters
    c->chIint = false;//TODO true;
    c->chIos = 0;
    c->chIgainExp = 1;
    c->IRMSoffset = 0;//-2048;//0x01BC;
    c->IRMSslope = 1;//.0010;//164;

    // Voltage Parameters
    c->chVos = 1;//15;
    c->chVgainExp = 4;
    c->chVscale = 0;
    c->VRMSoffset = 2047;//0x07FF;
    c->VRMSslope = .2199;//4700;
    c->sagDurationCycles = 10;
    c->minVSag = 100;

    // Power Parameters
    c->phcal = 11; //Ox0B
    c->VAEslope = 1;//34.2760;//2014/10000.0;
    c->VAoffset = 0;// TODO not used yet
    c->Wslope=1; // TODO not used yet
    c->Woffset=0;// TODO not used yet

    // Current and Voltage
    // Measured
    c->IRMS = 0;
    c->VRMS = 0;
    c->periodus = 1024;
    c->VA = 0;
    c->W = 0;
    c->PF = 65534;
    c->VAEnergy = 0;
    c->WEnergy = 0;

}

void Cprint(HardwareSerial *ser, Circuit *c) 
{

    ser->print("circuitID&");
    ser->println(c->circuitID);
    ser->print("halfCyclesSample&");
    ser->println(c->halfCyclesSample);
    ser->print("chIint&");
    ser->println(c->chIint);
    ser->print("chIgainExp&");
    ser->print(c->chIgainExp);
    ser->print("IRMSOS&");
    ser->println(c->IRMSoffset);
    ser->print("VRMSOS&");
    ser->println(c->VRMSoffset);
    ser->print("IRMS slope&");
    ser->println(c->IRMSslope,4);
    ser->print("VRMS slope&");
    ser->println(c->VRMSslope,4);
    ser->print("VAE slope&");
    ser->println(c->VAEslope,4);

}

void CprintMeas(HardwareSerial *ser, Circuit *c)
{
    ser->print("IRMS&");
    ser->println(c->IRMS);
    ser->print("VRMS&");
    ser->println(c->VRMS);
    ser->print("VA&");
    ser->println(c->VA);
    ser->print("VAEnergy&");
    ser->println(c->VAEnergy);
    ser->print("WEnergy&");
    ser->println(c->WEnergy);
    ser->print("PF&");
    ser->println(c->PF);
    ser->print("periodus&");
    ser->println(c->periodus);
}

/** 
    Attempts to reestablish communications with the ADE.
    1) The SS pin is strobed. 
    2) TODO Then the ADE is reset. 
    3) TODO Finally the ADE is reprogrammed.
    @returns true if a communication was successful.
*/
int8_t CrestoreCommunications(Circuit *c)
{
    CSselectDevice(c->circuitID);
    CSstrobe();
    
}

