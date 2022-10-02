#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <bitset>
#include <cmath>
#include <vector>
#include <map>
#include <bits/stdc++.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include "CaenSLC.nsmap"

#include "soapH.h"
#include "soapCaenSLCService.h"
#include "registers/RegisterCommon.h"
#include "registers/RegisterDPP_PSD.h"
#include "registers/RegisterDPP_PHA.h"


struct caenDGTZ{
  int                      handle       ;
  int                      linkNb       ;
  int                      boardNb      ;
  uint32_t                 boardAddress ;
  CAEN_DGTZ_DPPFirmware_t  firmType     ;
  CAEN_DGTZ_BoardInfo_t    boardInfo    ;  
  CAEN_DGTZ_ConnectionType connType     ;
  ns2__returnBoard         rBoard       ;
  caenDGTZ(){;}
  caenDGTZ(int conType,int lNb,int bNb, uint32_t bAdd){
    switch (conType){
    case CAEN_DGTZ_USB:
      connType = CAEN_DGTZ_USB;
      rBoard.connType = "USB Connection";
      std::cout << "USB Connection" << std::endl;
      break;
    case CAEN_DGTZ_OpticalLink:
      connType = CAEN_DGTZ_OpticalLink;
      rBoard.connType = "Optical Link Connection";
      std::cout << "OpticalLink Connection" << std::endl;
      break;
    default:
      std::cout << "Connection type not know (0-USB or 1-OpticalLink)" << std::endl;
    }
    rBoard.linkNumber  = lNb;
    rBoard.boardNumber = bNb;
    rBoard.ModelName   = "";
    linkNb  = lNb;
    boardNb = bNb;
    boardAddress = bAdd;
    handle  = 0;
  };
};

std::map<int,caenDGTZ> caenDgtz;


uint16_t port =8080;
int samplingClockUnits = 1;
float range_0[6] = {1.25,5,20,80,320,1280};
float range_2[6] = {5,20,80,320,1280,5120};
//std::vector<ns2__returnBoard*> theBoards;

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void SetRegisterSpecificBits(uint16_t board, uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, uint32_t value, int &error) {
  if(bit_lower>bit_upper) {
    printf("Warning bit_lower is smaller than bit_upper ... (%i,%i), I will invert them\n",
	   (int)bit_lower,(int)bit_upper);
    uint8_t tmp = bit_lower;
    bit_lower = bit_upper; bit_upper = tmp;
  }
  uint8_t bit_number = bit_upper - bit_lower + 1;
  if ( (value < ((uint32_t)1<<bit_number)) && bit_upper<32 && bit_lower<=bit_upper) {
    uint32_t reg_data = 0;
    uint32_t reg_mask = 0;
    for (int bit = 0; bit < 32; bit++) { if (bit<bit_lower || bit>bit_upper) { reg_mask+=(1<<bit); } }
    
    error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_add, &reg_data);
    if (error==0) {
      reg_data = (reg_data & reg_mask) + (value << bit_lower);
      error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_add, reg_data);
    }
    error = (int) CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_add, &reg_data);
    if(error==0){
      if((reg_data & ~reg_mask)>>bit_lower != value ) {
	printf("Reg value (after the mask) = %08X while value = %08X\n",
	       (reg_data & ~reg_mask)>>bit_lower,value);
	printf("Error register %04X value not set properly %08X\n",reg_add,reg_data);
	printf("Calculated bitmask = %08X for bits from %i to %i \n",~reg_mask,(int)bit_lower,(int)bit_upper);
      }
    }
  }else {
    printf("I didn't do anything for register %04x and bit_lower/upper = %i,%i\n",reg_add,bit_lower,bit_upper);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

uint32_t GetRegisterSpecificBits(uint16_t board, uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, int &error) {
  uint32_t reg_data = 0;
  uint32_t reg_mask = 0;
  if (bit_upper<32 && bit_lower<=bit_upper) {
    for (int bit = 0; bit < 32; bit++) { if (bit>=bit_lower && bit<=bit_upper) { reg_mask+=(1<<bit); } }
    error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_add, &reg_data);
  }
  return (reg_data & reg_mask) >> bit_lower;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void getParameters(int argc,char* argv[])
{
  int argn ;
  char *cmd;
  int ok;
  int conType = 0 ;
  int linkNb,boardNb;
  uint32_t boardAddress;
  std::vector<std::string> tokens;
  std::string bAddr;
  ok = (argc>1) ? 1:0;
  argn=1;
  while (ok==1 && argn < argc){
    cmd = argv[argn++];
    ok =1 ;
    if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help")) {
      ok = -1;
    }else if (!strcmp(cmd, "-c")) {
      conType = atoi(argv[argn++]);
    } else if (!strcmp(cmd, "-l")) {
      linkNb  = atoi(argv[argn++]);
    } else if (!strcmp(cmd, "-a")) {
      boardAddress = std::stoul(argv[argn++], nullptr, 16);
    } else if (!strcmp(cmd, "-b")) {
      boardNb = atoi(argv[argn++]);
    } else if(!strcmp(cmd,"-p")){
      port = atoi(argv[argn++]);
    } else if(!strcmp(cmd,"-fb")){
      tokens.clear();
      std::cout << "board definition" << std::endl;
      std::stringstream boards(argv[argn++]);
      std::string temp;
      while(getline(boards,temp,'_')){
	std::cout << tokens.size() <<" " <<  temp << std::endl;
	tokens.push_back(temp);
      }
      conType      = atoi(tokens[0].c_str());
      linkNb       = atoi(tokens[1].c_str());
      boardNb      = atoi(tokens[2].c_str());
      boardAddress = std::stoul(tokens[3], nullptr, 16);
      caenDgtz[boardNb] = caenDGTZ(conType,
					     linkNb,
					     boardNb,
					     boardAddress);
      std::cout << "Board defined " << std::endl;
    }

    
  }

  if(ok<0){
    printf("Error during the setup! will exit \n");
    
    printf("Usage: caenSLC_server: \n");
    printf("\t -p  \t\t port for the soap server\n");
    printf("\t -c  \t\t connection type to the CAEN board (0-USB ; 1-Optical Link)\n");
    printf("\t -l  \t\t Link number (default is 0)\n");
    printf("\t -a  \t\t Board address (only for VME)\n");
    printf("\t -b  \t\t Board number (only in the case of chain)\n");
    printf("\t -fb \t\t ConnectionType_LinkNumber_BoardNumber_BoardAddress (to add several boards)");
    
    exit(-10);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  getParameters(argc,argv);

  std::cout << "Parameters read." << std::endl;
  for(auto it = caenDgtz.begin() ; it != caenDgtz.end(); ++it){
    std::cout << "Connecting to the Digitizer..." << std::endl;
    int error = (int)CAEN_DGTZ_OpenDigitizer(it->second.connType, 
					     it->second.linkNb, 
					     it->second.boardNb, 
					     it->second.boardAddress, 
					     &it->second.handle);
    if(error!=0){
      std::cout << "Connection Error: " << error << std::endl;
      return -1;
    }

    std::cout << "Connected." << std::endl;

    uint32_t reg_value;
    uint32_t reg_addr;
    error = (int)CAEN_DGTZ_GetInfo(it->second.handle, &it->second.boardInfo);
    if(error==0){
      printf("ModelName:\t%12s\n" ,it->second.boardInfo.ModelName);
      printf("Model:\t%20i\n"     ,it->second.boardInfo.Model);
      printf("Channels:\t%12i\n"  ,it->second.boardInfo.Channels);
      printf("FormFactor:\t%12i\n",it->second.boardInfo.FormFactor);
      printf("FamilyCode:\t%12i\n",it->second.boardInfo.FamilyCode);
      printf("ROC FW rel:\t%13s\n",it->second.boardInfo.ROC_FirmwareRel);
      printf("AMC FW rel:\t%12s\n",it->second.boardInfo.AMC_FirmwareRel);
      printf("Serial no.:\t%12i\n",it->second.boardInfo.SerialNumber);
      printf("ADC_NBits:\t%12i\n" ,it->second.boardInfo.ADC_NBits);
    }
    std::cout << caenDgtz[0].boardInfo.Channels << std::endl;
    printf("Channels:\t%12i\n"  ,it->second.boardInfo.Channels);
    switch(it->second.boardInfo.FamilyCode){
    case CAEN_DGTZ_XX730_FAMILY_CODE:
      samplingClockUnits = 2;
      break;
    case CAEN_DGTZ_XX725_FAMILY_CODE:
      samplingClockUnits = 4;
      break;
    default:
      printf("  --- Family clock sampling not yet implemented, will use 1 ns sampling period\n");
      samplingClockUnits = 1;
    }
    it->second.rBoard.samplingClock = samplingClockUnits;
    error = (int)CAEN_DGTZ_GetDPPFirmwareType(it->second.handle, &it->second.firmType);
    printf("The Digitizer %s is using the firmware ",it->second.boardInfo.ModelName);
    switch (it->second.firmType){
    case CAEN_DGTZ_DPPFirmware_PHA:
      printf("DPP-Pulse Height Analysis (PHA)\n");
      it->second.rBoard.FWName="DPP-Pulse Height Analysis (PHA)";
      break;
    case CAEN_DGTZ_DPPFirmware_PSD:
      printf("DPP-Pulse Shape Discrimination (PSD)\n");
      it->second.rBoard.FWName="DPP-Pulse Shape Discrimination (PSD)";
      break;
    case CAEN_DGTZ_DPPFirmware_CI:
      printf("DPP-Charge Integration (CI)\n");
      it->second.rBoard.FWName="DPP-Charge Integration (CI)";
      break;
    case CAEN_DGTZ_DPPFirmware_ZLE:
      printf("DPP-Zero Length Encoding (ZLE)\n");
      it->second.rBoard.FWName="DPP-Zero Length Encoding (ZLE)";
      break;
    case CAEN_DGTZ_DPPFirmware_QDC:
      printf("DPP-Charge Integration in High Density Systems (QDC)\n");
      it->second.rBoard.FWName="DPP-Charge Integration in High Density Systems (QDC)";
      break;
    case CAEN_DGTZ_DPPFirmware_DAW:
      printf("DPP-Dynamic Acquisition Window (DAW)\n");
      it->second.rBoard.FWName="DPP-Dynamic Acquisition Window (DAW)";
      break;
    case CAEN_DGTZ_NotDPPFirmware:
      printf("WaveCatcher basic firmware\n");
      it->second.rBoard.FWName="WaveCatcher basic firmware";
      break;
    default:
      printf("Firmware not recognized! Please check all the cases\n");
      return -3;
    }
    it->second.rBoard.fwType = it->second.firmType;
  }
  
  CaenSLCService *service = new CaenSLCService();
  service->soap->send_timeout = service->soap->recv_timeout = 5;
  service->soap->bind_flags = SO_REUSEADDR; // To permit recconection

  while (service->run(port))
    service->soap_stream_fault(std::cerr);
  service->destroy();
  
  
  return 0;
  //  return soap_server(soap_new());
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::getNbBoards(uint32_t &nbBoard)
{
  nbBoard = caenDgtz.size();
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::getBoardDescription(uint16_t board, ns2__returnBoard &rVal)
{
  rVal = caenDgtz[board].rBoard;
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::getFwVersion(uint16_t board, int16_t &fwRel)
{
  CAEN_DGTZ_GetDPPFirmwareType(caenDgtz[board].handle, &caenDgtz[board].firmType);
  fwRel = (int16_t)caenDgtz[board].firmType;
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::getNbChan(uint16_t board, uint16_t &channel){
  std::cout << "Board " << board << "\tChannels " << caenDgtz[board].boardInfo.Channels << "\tBoards " << caenDgtz.size( ) << std::endl;
  channel = caenDgtz[board].boardInfo.Channels;
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::read(uint16_t board, uint32_t address, ns2__readReg &rVal)
{
  rVal.error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, address, &rVal.value);
  if(rVal.error!=0){
    printf("Error (%i) while reading register %04X\n",rVal.error,address);
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::write(uint16_t board, uint32_t address, uint32_t value, int &error)
{
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, address, value);
  if(error!=0){
    printf("Error (%i) while writing register %04X\n",error,address);
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::reset(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_Reset(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while reseting the digitizer\n");    
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::start(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_SWStartAcquisition(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while starting the digitizers acquisition\n");
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::stop(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_SWStopAcquisition(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while stoping the digitizers acquisition\n");
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::clear(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_ClearData(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while clearing the digitizers data buffers\n");
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::swtrg(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_SendSWtrigger(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while sending a software trigger to the digitizer\n");
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::calibrate(uint16_t board, int &error)
{
  error = (int)CAEN_DGTZ_Calibrate(caenDgtz[board].handle);
  if(error!=0){
    printf("Error while calibrating the digitizer's channels\n");
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::boardinfo(uint16_t board, ns2__returnInfo &rVal)
{
  rVal.ModelName       = caenDgtz[board].boardInfo.ModelName;
  rVal.Model           = caenDgtz[board].boardInfo.Model;
  rVal.Channels        = caenDgtz[board].boardInfo.Channels;
  rVal.FormFactor      = caenDgtz[board].boardInfo.FormFactor;
  rVal.FamilyCode      = caenDgtz[board].boardInfo.FamilyCode;
  rVal.ROC_FirmwareRel = caenDgtz[board].boardInfo.ROC_FirmwareRel;
  rVal.AMC_FirmwareRel = caenDgtz[board].boardInfo.AMC_FirmwareRel;
  rVal.SerialNumber    = caenDgtz[board].boardInfo.SerialNumber;
  rVal.ADC_NBits       = caenDgtz[board].boardInfo.ADC_NBits;

  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::channelinfo(uint16_t board, uint16_t channel, ns2__channelInfo &rVal)
{
  uint32_t reg_data;
  uint32_t reg_addr;
  std::bitset<32> valBin ;
  rVal.error = 0;
  rVal.error    = (int)CAEN_DGTZ_GetChannelEnableMask(caenDgtz[board].handle, &reg_data);
  rVal.chEnable = (reg_data>>channel)&1;

  // ADC temperature 
  rVal.error   |= (int)CAEN_DGTZ_ReadTemperature(caenDgtz[board].handle,channel,&rVal.temperature);

  // ADC OFFSET
  rVal.error   |= (int)CAEN_DGTZ_GetChannelDCOffset(caenDgtz[board].handle,channel,&rVal.adcOffset);

  reg_addr  = REG_CHA_STT2 + 0x100*channel; 
  rVal.error   |= (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_addr, &reg_data);
  valBin = std::bitset<32>(reg_data);
  rVal.isSPIbusy = valBin[2];
  rVal.isADCcal  = valBin[3];
  rVal.isADCdown = valBin[8];  
  CAEN_DGTZ_PulsePolarity_t pol;
  rVal.error    |= CAEN_DGTZ_GetChannelPulsePolarity(caenDgtz[board].handle,channel,&pol);
  if(pol==CAEN_DGTZ_PulsePolarityPositive)
    rVal.isPositive=true;
  else
    rVal.isPositive=false;

  reg_addr  = REG_DPP_CTR1 + 0x100*channel; 
  rVal.error   |= (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_addr, &reg_data);
  valBin = std::bitset<32>(reg_data);
  rVal.trEnable = ((valBin[24]==0)?1:0);

  uint32_t value;
  reg_addr = REG_DYN_RANG + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 0,rVal.error);
  rVal.inputRange = value;
  
  // Number of samples (for all channels), must be multiple of 4
  rVal.error    |= CAEN_DGTZ_GetRecordLength(caenDgtz[board].handle,&rVal.nSamples);

  // Trigger Threshold
  switch(caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr  = PHA_TRG_TRSD + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr  = PSD_TRG_TRSD + 0x100*channel;
    break;
  default:
    reg_addr  = PSD_TRG_TRSD + 0x100*channel;
    break;
  }
  rVal.error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_addr, &rVal.triggerThreshold);
  
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::channelpsdsettings(uint16_t board, uint16_t channel, ns2__psdInfo &rVal)
{

  rVal.error = 0;
  
  if( caenDgtz[board].firmType != CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    rVal.error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }

  uint32_t value;
  uint32_t reg_addr;

  // CFD Delay
  reg_addr = 0x103c + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 7,rVal.error);
  rVal.cfdDelay = (value*samplingClockUnits);

  // CFD Fraction
  reg_addr = 0x103c + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,8, 9,rVal.error);
  rVal.cfdFraction = value;

  // CFD Points
  reg_addr = 0x103c + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,10, 11,rVal.error);
  rVal.cfdPoints = value;

  // Charge Threshold
  reg_addr = PSD_ZER_SUPP + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 15,rVal.error);
  rVal.chargeThreshold = value;

  // Short Gate
  reg_addr = PSD_SHO_GATE + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 11,rVal.error);
  rVal.shortGate = (value*samplingClockUnits);

  // Long Gate
  reg_addr = PSD_LON_GATE + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 15,rVal.error);
  rVal.longGate = (value*samplingClockUnits);

  // Gate Offset
  reg_addr = PSD_GAT_OFFS + 0x100*channel;;
  value = GetRegisterSpecificBits(board,reg_addr,0, 7,rVal.error);
  rVal.gateOffset = (value*samplingClockUnits);

  // Shaped Trigger Width
  reg_addr = PSD_TRG_WDTH + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 9,rVal.error);
  rVal.shapedTrgWidth = (value*samplingClockUnits*4);

  // Trigger Hold-Off
  reg_addr = REG_TRG_HDOF + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 15,rVal.error);
  rVal.trgHoldOff = (value*samplingClockUnits*4);

  // PSD Cut
  reg_addr = PSD_PSD_CUTT + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 9,rVal.error);
  rVal.psdCut = value;

  // PUR Threshold
  reg_addr = PSD_PUR_THSD + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 11,rVal.error);
  rVal.purThreshold = value;

  // Charge Sensitivity
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 2,rVal.error);
  rVal.chargeSensitivity = value;

  // Charge Pedestal
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,4, 4,rVal.error);
  if( value ) rVal.isChargePedestal = true;
  else rVal.isChargePedestal = false;

  // Trigger Counting
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,5, 5,rVal.error);
  if( value ) rVal.isTrgCounting = true;
  else rVal.isTrgCounting = false;

  // Discrimination Mode
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,6, 6,rVal.error);
  if( value ) rVal.isDiscrMode = true;
  else rVal.isDiscrMode = false;

  // Pile Up
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,7, 7,rVal.error);
  if( value ) rVal.isPileUp = true;
  else rVal.isPileUp = false;

  // Trigger Mode
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,18, 19,rVal.error);
  rVal.trgMode = value;

  // Baseline Mean
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,20, 22,rVal.error);
  rVal.baselineMean = value;

  // Charge Cut
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,25, 25,rVal.error);
  if( value ) rVal.isChargeCut = true;
  else rVal.isChargeCut = false;

  // PUR
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,26, 26,rVal.error);
  if( value ) rVal.isPUR = true;
  else rVal.isPUR = false;

  // Over Range Rejection
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,29, 29,rVal.error);
  if( value ) rVal.isOverRangeReject = true;
  else rVal.isOverRangeReject = false;

  // Trigger Hysteresis
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,30, 30,rVal.error);
  if( value ) rVal.isTrgHysteresis = true;
  else rVal.isTrgHysteresis = false;
  
  // Local Shaped Trigger
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 2, 2, rVal.error);
  if( value ) rVal.localShapedTrg = true;
  else rVal.localShapedTrg = false;

  // Local Shaped Trigger Mode
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 0, 1, rVal.error);
  rVal.localShapedTrgMode = value;

  // Local Trigger Validation
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 6, 6, rVal.error);
  if( value ) rVal.localTrgVal = true;
  else rVal.localTrgVal = false;

  // Local Trigger Validation Mode
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 4, 4, rVal.error);
  rVal.localTrgValMode = value;

  // Extras
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 8, 10, rVal.error);
  rVal.extras = value;
  
  std::cout << "### DPP-PSD settings for channel " << channel << std::endl << std::endl;
  
  std::cout << "CFD Delay: "                             << rVal.cfdDelay              << std::endl;
  std::cout << "CFD Fraction: "                          << rVal.cfdFraction           << std::endl;
  std::cout << "CFD Points: "                            << rVal.cfdPoints             << std::endl;
  std::cout << "Charge Threshold: "                      << rVal.chargeThreshold       << std::endl;
  std::cout << "Short Gate: "                            << rVal.shortGate             << std::endl;
  std::cout << "Long Gate: "                             << rVal.longGate              << std::endl;
  std::cout << "Gate Offset: "                           << rVal.gateOffset            << std::endl;
  std::cout << "Shaped Trigger Width: "                  << rVal.shapedTrgWidth        << std::endl;
  std::cout << "Trigger Hold-Off: "                      << rVal.trgHoldOff            << std::endl;
  std::cout << "PSD Cut: "                               << rVal.psdCut                << std::endl;
  std::cout << "PUR Threshold: "                         << rVal.purThreshold          << std::endl;
  std::cout << "Charge Sensitivity: "                    << rVal.chargeSensitivity     << std::endl;
  std::cout << "Charge Pedestal: "                       << rVal.isChargePedestal      << std::endl;
  std::cout << "Trigger Counting: "                      << rVal.isTrgCounting         << std::endl;
  std::cout << "Discrimination Mode: "                   << rVal.isDiscrMode           << std::endl;
  std::cout << "Pile Up: "                               << rVal.isPileUp              << std::endl;
  std::cout << "Trigger Mode: "                          << rVal.trgMode               << std::endl;
  std::cout << "Baseline Mean: "                         << rVal.baselineMean          << std::endl;
  std::cout << "Charge Cut: "                            << rVal.isChargeCut           << std::endl;
  std::cout << "PUR: "                                   << rVal.isPUR                 << std::endl;
  std::cout << "Over Range Rejection: "                  << rVal.isOverRangeReject     << std::endl;
  std::cout << "Trigger Hysteresis: "                    << rVal.isTrgHysteresis       << std::endl;
  std::cout << "Local Shaped Trigger: "                  << rVal.localShapedTrg        << std::endl;
  std::cout << "Local Shaped Trigger Mode: "             << rVal.localShapedTrgMode    << std::endl;
  std::cout << "Local Trigger Validation: "              << rVal.localTrgVal           << std::endl;
  std::cout << "Local Trigger Validation Mode: "         << rVal.localTrgValMode       << std::endl;
  std::cout << "Extras: "                                << rVal.extras                << std::endl;
  
  
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::channelphasettings(uint16_t board, uint16_t channel, ns2__phaInfo &rVal)
{

  rVal.error = 0;
  
  if( caenDgtz[board].firmType != CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    rVal.error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }

  uint32_t value;
  uint32_t reg_addr;

  // Input Rise Time
  reg_addr = PHA_INP_RTIM + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 7,rVal.error);
  rVal.inputRiseTime = (value*samplingClockUnits*4);

  // TRPZ Rise Time
  reg_addr = PHA_TRZ_RTIM + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0,11,rVal.error);
  rVal.riseTime = (value*samplingClockUnits*4);

  // TRPZ Flat Top
  reg_addr = PHA_TRZ_FTOP + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0,11,rVal.error);
  rVal.flatTop = (value*samplingClockUnits*4);

  // TRPZ Decay Time
  reg_addr = PHA_TRZ_DTIM + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0,15,rVal.error);
  rVal.decayTime = (value*samplingClockUnits*4);
  
  // TRPZ Peaking Time
  reg_addr = PHA_TRZ_PTIM + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0,11,rVal.error);
  rVal.peakingTime = (value*samplingClockUnits*4);

  // TRPZ Peak Mean
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,12,13,rVal.error);
  rVal.peakMean = value;

  // Rise Time Validation Window
  reg_addr = PHA_VAL_WIND + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 9,rVal.error);
  rVal.riseTimeValWindow = (value*samplingClockUnits);

  // Peaking Hold-Off
  reg_addr = PHA_PEK_HOFF + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 9,rVal.error);
  rVal.peakHoldOff = (value*samplingClockUnits*4);

  // Trapezoidal Rescaling
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 5,rVal.error);
  rVal.trpzRescaling = value;

  // Decimation
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,8, 9,rVal.error);
  rVal.decimation = value;

  // Decimation Gain
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,10,11,rVal.error);
  rVal.decimationGain = value;

  // Fine Gain
  reg_addr = PHA_FIN_GAIN + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0,15,rVal.error);
  rVal.fineGain = value;

  // Roll-Over Flag
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,26,26,rVal.error);
  if( value ) rVal.roFlag = true;
  else rVal.roFlag = false;

  // Pile-Up Flag
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,27,27,rVal.error);
  if( value ) rVal.puFlag = true;
  else rVal.puFlag = false;

  // RC-CR2
  reg_addr = PHA_MOV_AVRG + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,0, 5,rVal.error);
  rVal.rccr2 = value;

  // Extras2
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 8,10, rVal.error);
  rVal.extras2 = value;

  // Trigger Mode
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,18,19, rVal.error);
  rVal.trgMode = value;

  // Baseline Average
  reg_addr = REG_DPP_CTR1 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr,20,22, rVal.error);
  rVal.baselineAvgWindow = value;

  // Local Shaped Trigger
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 2, 2, rVal.error);
  if( value ) rVal.localShapedTrg = true;
  else rVal.localShapedTrg = false;

  // Local Shaped Trigger Mode
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 0, 1, rVal.error);
  rVal.localShapedTrgMode = value;
  
  // Local Trigger Validation
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 6, 6, rVal.error);
  if( value ) rVal.localTrgVal = true;
  else rVal.localTrgVal = false;

  // Local Trigger Validation Mode
  reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  value = GetRegisterSpecificBits(board,reg_addr, 4, 5, rVal.error);
  rVal.localTrgValMode = value;
  
  std::cout << "### DPP-PHA settings for channel " << channel << std::endl << std::endl;
  
  std::cout << "Input Rise Time: "               << rVal.inputRiseTime         << std::endl;
  std::cout << "Rise Time: "                     << rVal.riseTime              << std::endl;
  std::cout << "Falt Top: "                      << rVal.flatTop               << std::endl;
  std::cout << "Decay Time: "                    << rVal.decayTime             << std::endl;
  std::cout << "Peaking Time: "                  << rVal.peakingTime           << std::endl;
  std::cout << "Peaking Hold-Off: "              << rVal.peakHoldOff           << std::endl;
  std::cout << "Peak Mean: "                     << rVal.peakMean              << std::endl;
  std::cout << "Rise Time Vlaidation Window: "   << rVal.riseTimeValWindow     << std::endl;
  std::cout << "TRPZ Rescaling: "                << rVal.trpzRescaling         << std::endl;
  std::cout << "Decimation: "                    << rVal.decimation            << std::endl;
  std::cout << "Decimation Gain: "               << rVal.decimationGain        << std::endl;
  std::cout << "Fine Gain: "                     << rVal.fineGain              << std::endl;
  std::cout << "Roll-Over Flag: "                << rVal.roFlag                << std::endl;
  std::cout << "Pile-Up Flag: "                  << rVal.puFlag                << std::endl;
  std::cout << "RC-CR2: "                        << rVal.rccr2                 << std::endl;
  std::cout << "Extras2: "                       << rVal.extras2               << std::endl;
  std::cout << "Trigger Mode: "                  << rVal.trgMode               << std::endl;
  std::cout << "Baseline Average: "              << rVal.baselineAvgWindow     << std::endl;
  std::cout << "Local Shaped Trigger: "          << rVal.localShapedTrg        << std::endl;
  std::cout << "Local Shaped Trigger Mode: "     << rVal.localShapedTrgMode    << std::endl;
  std::cout << "Local Trigger Validation: "      << rVal.localTrgVal           << std::endl;
  std::cout << "Local Trigger Validation Mode: " << rVal.localTrgValMode       << std::endl;
  
  
  return SOAP_OK;
}

int CaenSLCService::generalsettings(uint16_t board, ns2__generalInfo &rVal)
{

  rVal.error = 0;

  if( caenDgtz[board].firmType == CAEN_DGTZ_DPPFirmware_PHA ){
    rVal.isPHA = true;
    rVal.isPSD = false;
  }
  else if( caenDgtz[board].firmType == CAEN_DGTZ_DPPFirmware_PSD ){
    rVal.isPHA = false;
    rVal.isPSD = true;
  }
  else{
    std::cerr << "Firmware Error: not recognized!" << std::endl;
    rVal.error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }

  uint32_t value;
  uint32_t reg_addr;

  // Automatic Data Flush
  reg_addr = REG_BRD_CONF;
  value = GetRegisterSpecificBits(board,reg_addr,0, 0,rVal.error);
  if( value ) rVal.isAutoDataFlush = true;
  else rVal.isAutoDataFlush = false;

  // Trigger Propagation
  reg_addr = REG_BRD_CONF;
  value = GetRegisterSpecificBits(board,reg_addr,1, 1,rVal.error);
  if( value ) rVal.isTrgPropagation = true;
  else rVal.isTrgPropagation = false;

  // Waveform Enabled
  reg_addr = REG_BRD_CONF;
  value = GetRegisterSpecificBits(board,reg_addr,16, 16,rVal.error);
  if( value ) rVal.isTrace = true;
  else rVal.isTrace = false;
  
  // Dual Trace
  reg_addr = REG_BRD_CONF;
  value = GetRegisterSpecificBits(board,reg_addr,11, 11,rVal.error);
  if( value ) rVal.isDualTrace = true;
  else rVal.isDualTrace = false;
  
  // Extras Word
  reg_addr = REG_BRD_CONF;
  value = GetRegisterSpecificBits(board,reg_addr,17, 17,rVal.error);
  if( value ) rVal.isExtras = true;
  else rVal.isExtras = false;

  if( rVal.isPHA ){

    // Analog Probe 1
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,12, 13,rVal.error);
    rVal.analogProbe1PHA = value;

    // Analog Probe 2
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,14, 15,rVal.error);
    rVal.analogProbe2PHA = value;

    // Digital Probe 1
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,20, 23,rVal.error);
    rVal.digitalProbe1PHA = value;

    // Digital Probe 2
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,26, 28,rVal.error);
    rVal.digitalProbe2PHA = value;

  }

  if( rVal.isPSD ){

    // Analog Probe
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,12, 13,rVal.error);
    rVal.analogProbePSD = value;

    // Digital Probe 1
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,23, 25,rVal.error);
    rVal.digitalProbe1PSD = value;

    // Digital Probe 2
    reg_addr = REG_BRD_CONF;
    value = GetRegisterSpecificBits(board,reg_addr,26, 28,rVal.error);
    rVal.digitalProbe2PSD = value;
    
  }
  
  std::cout << "### General board settings" << std::endl << std::endl;

  std::cout << "Input Range: "                        << rVal.inputRange              << std::endl;
  std::cout << "Automatic Data Flush: "               << rVal.isAutoDataFlush         << std::endl;
  std::cout << "Trigger Propagation: "                << rVal.isTrgPropagation        << std::endl;
  std::cout << "Extras Word: "                        << rVal.isExtras                << std::endl;
  std::cout << "Dual Trace: "                         << rVal.isDualTrace             << std::endl;
  if( rVal.isPHA ){
    std::cout << "Analog Probe 1: "                   << rVal.analogProbe1PHA         << std::endl;
    std::cout << "Analog Probe 2: "                   << rVal.analogProbe2PHA         << std::endl;
    std::cout << "Digital Probe 1: "                  << rVal.digitalProbe1PHA        << std::endl;
    std::cout << "Digital Probe 2: "                  << rVal.digitalProbe2PHA        << std::endl;
  }
  if( rVal.isPSD ){
    std::cout << "Analog Probe: "                     << rVal.analogProbePSD          << std::endl;
    std::cout << "Digital Probe 1: "                  << rVal.digitalProbe1PSD        << std::endl;
    std::cout << "Digital Probe 2: "                  << rVal.digitalProbe2PSD        << std::endl;
  }
  
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::acqmode(uint16_t board, uint32_t mode, uint32_t saveVal, int &error)
{
  CAEN_DGTZ_DPP_AcqMode_t modeW;
  CAEN_DGTZ_DPP_SaveParam_t params;
  error = (int)CAEN_DGTZ_SetDPPAcquisitionMode(caenDgtz[board].handle,
					       (CAEN_DGTZ_DPP_AcqMode_t)mode,
					       (CAEN_DGTZ_DPP_SaveParam_t)saveVal);
  error = (int)CAEN_DGTZ_GetDPPAcquisitionMode(caenDgtz[board].handle,
					       &modeW,&params);
  if(modeW!=(CAEN_DGTZ_DPP_AcqMode_t)mode) printf("Error setting the acquisition mode\n");
  if(params!=(CAEN_DGTZ_DPP_SaveParam_t)saveVal) printf("Error setting the saved parameters\n");
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::setmaxaggr(uint16_t board, uint32_t numAggr, int &error)
{
  error = (int)CAEN_DGTZ_SetMaxNumEventsBLT(caenDgtz[board].handle,numAggr);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::cfddelay(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  // TRUE ONLY FOR THE x730 and x725 DPP-PSD 
  uint32_t reg_addr = 0x103c+0x100*channel;
  value /= samplingClockUnits;
  SetRegisterSpecificBits(board,reg_addr,0, 7,value,error);

  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::cfdfraction(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  // TRUE ONLY FOR THE x730 and x725 DPP-PSD 
  uint32_t reg_addr = 0x103c+0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,8, 9,value,error);

  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::cfdinterpolation(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  // TRUE ONLY FOR THE x730 and x725 DPP-PSD 
  uint32_t reg_addr = 0x103c+0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,10, 11,value,error);

  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::cfddump(uint16_t board, uint16_t channel, ns2__cfdInfo &rVal)
{
  uint32_t reg_addr =  0x103c+0x100*channel;
  uint32_t reg_data=0;
  rVal.error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle, reg_addr, &reg_data);
  rVal.cfdDelay         = (uint16_t) (reg_data & 0xFF);
  rVal.cfdFraction      = (uint16_t) ((reg_data&0x300) >>  8);
  rVal.cfdInterpolation = (uint16_t) ((reg_data&0xC00) >> 10);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::chenable(uint16_t board, uint16_t channel, bool value, int &error)
{
  
  uint32_t reg_data;
  uint32_t bitmask = 1<<channel;
  error =  CAEN_DGTZ_GetChannelEnableMask(caenDgtz[board].handle, &reg_data);
  if(error ==0 ){
    reg_data = (reg_data & (~bitmask)) | (value<<channel & bitmask);
    error = CAEN_DGTZ_SetChannelEnableMask(caenDgtz[board].handle, reg_data);
  }else {
    printf("Error (%i) while setting the channel enabled",error);
  }
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::enaettt(uint16_t board, int &error)
{
  uint32_t reg_addr = REG_FPI_CTRL;
  SetRegisterSpecificBits(board,reg_addr, 20,22, 2, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// DPP-PSD related

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::tracelength(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = REG_REC_LENG + 0x100*channel;
  // Following CAEN documentations for the DPP-PSD and DPP-PSA 
  // the register value is multiplied by 8 to get the proper number of samples
  // so here we divide by 8 for the user
  value /= 8 ;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x3fff);

  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::inputrange(uint16_t board, uint16_t channel, bool value, int &error){

  uint32_t reg_addr = REG_DYN_RANG + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x1);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::evtaggregate(uint16_t board, uint16_t channel, uint16_t value, int &error){
  uint32_t reg_addr = REG_EVT_AGGR + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x3ff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::pretrigger(uint16_t board, uint16_t channel, uint16_t value, int &error){
  uint32_t reg_addr = REG_PRE_TRIG + 0x100*channel;
  // Following CAEN documentations for the DPP-PSD and DPP-PSA 
  // the register value is multiplied by 4 to get the proper number of samples
  // so here we divide by 4 for the user
  value /= 4 ;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x1ff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::forceflush(uint16_t board, uint16_t channel, int&error)
{
  uint32_t reg_addr;
  switch(caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DAT_FLUS + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DAT_FLUS + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DAT_FLUS + 0x100*channel;
  }
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, 0x1);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::zerosupp(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_ZER_SUPP + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xffff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::shortgate(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_SHO_GATE + 0x100*channel;
  value /= samplingClockUnits;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xfff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::longgate(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_LON_GATE + 0x100*channel;
  value /= samplingClockUnits;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xffff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::gateoffset(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_GAT_OFFS + 0x100*channel;
  value /= samplingClockUnits;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trgthres(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr ;
  switch(caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr  = PHA_TRG_TRSD + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr  = PSD_TRG_TRSD + 0x100*channel;
    break;
  default:
    reg_addr  = PSD_TRG_TRSD + 0x100*channel;
    break;
  }
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x3fff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::fixbaseline(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_FIX_BSLE + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x3fff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::shtrgwidth(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr;
  switch(caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_TRG_WDTH + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_TRG_WDTH + 0x100*channel;
    break;
  default:
    reg_addr = PSD_TRG_WDTH + 0x100*channel;
    break;
  }
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0x3ff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trgholdoff(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = REG_TRG_HDOF + 0x100*channel;
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xffff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::cutpsdthres(uint16_t board, uint16_t channel, float value, int &error)
{
  uint32_t reg_addr = PSD_PSD_CUTT + 0x100*channel;
  // Following CAEN documentation the threshold has to be multiplied by 1024
  uint16_t valInt = floor(value*1024);
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, valInt&0x3ff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::purthres(uint16_t board, uint16_t channel, uint16_t value, int &error)
{
  uint32_t reg_addr = PSD_PUR_THSD + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle, reg_addr, value&0xfff);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::chargesense(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 0, 2, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::chargepedestal(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 4, 4, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trgcounting(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 5, 5, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::discritype(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 6, 6, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::purenable(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 7, 7, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::adcsigsource(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 8, 8, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::adctestrate(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 9,10, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::bslrecalc(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,15,15, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common to DPP-PSD and DPP-PHA
int CaenSLCService::adcpolarity(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,16,16, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common to DPP-PSD and DPP-PHA
int CaenSLCService::trgmode(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,18,19, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common to DPP-PSD and DPP-PHA
int CaenSLCService::bslcalc(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,20,22, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common to DPP-PSD and DPP-PHA
int CaenSLCService::selftrgena(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,24,24, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Only for DPP-PSD
int CaenSLCService::chargereject(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,25,25, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Only for DPP-PSD
int CaenSLCService::purreject(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,26,26, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY FOR DPP-PSD
int CaenSLCService::psdcutlow(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,27,27, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::psdcutabv(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,28,28, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::overrange(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,29,29, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trghysteresis(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,30,30, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::opppolarity(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,31,31, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common DPP-PSD and DPP-PHA
int CaenSLCService::localtrgmode(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }
  SetRegisterSpecificBits(board,reg_addr, 0, 1, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common DPP-PSD and DPP-PHA
int CaenSLCService::enablelocaltrg(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }

  SetRegisterSpecificBits(board,reg_addr, 2, 2, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common DPP-PSD and DPP-PHA
int CaenSLCService::localtrgval(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr ;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }
  SetRegisterSpecificBits(board,reg_addr, 4, 5, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common DPP-PSD and DPP-PHA
int CaenSLCService::enablelocalval(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr ;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }
  SetRegisterSpecificBits(board,reg_addr, 6, 6, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Common DPP-PSD and DPP-PHA
int CaenSLCService::confextra(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }
  SetRegisterSpecificBits(board,reg_addr, 8,10, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PSD
int CaenSLCService::usesmooth(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }

  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,11,11, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PSD
int CaenSLCService::confsmooth(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }

  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,12,15, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// COMMON DPP-PSD and DPP-PHA
int CaenSLCService::conftrgcr(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr;
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  }
  SetRegisterSpecificBits(board,reg_addr,16,17, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// COMMON DPP-PSD and DPP-PHA
int CaenSLCService::confveto(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr;
  int binMin(0),binMax(0);
  switch (caenDgtz[board].firmType){
  case CAEN_DGTZ_DPPFirmware_PHA:
    reg_addr = PHA_DPP_CTR2 + 0x100*channel;
    binMin = 14 ; binMax = 15;
    break;
  case CAEN_DGTZ_DPPFirmware_PSD:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    binMin = 18 ; binMax = 19;
    break;
  default:
    reg_addr = PSD_DPP_CTR2 + 0x100*channel;
    binMin = 18 ; binMax = 19;
  }
  SetRegisterSpecificBits(board,reg_addr,binMin,binMax, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PHA
int CaenSLCService::continuusbsl(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,18,18, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PHA
int CaenSLCService::coincflag(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,19,19, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY PSD
int CaenSLCService::enablesat(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,24,24, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PSD
int CaenSLCService::localtrgopt(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,25,26, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PSD
int CaenSLCService::vetomode(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,27,27, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PSD
int CaenSLCService::rststampveto(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
   if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PSD){
    std::cerr << "Error: This register is only availble with the PSD firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PSD_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,28,28, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// ONLY DPP-PHA
int CaenSLCService::bsropt(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
    if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_DPP_CTR2 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,29,29, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::adcoffset(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_ADC_OFFS + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,15, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::indsofttrg(uint16_t board, uint16_t channel, int &error)
{
  uint32_t reg_addr = PSD_IND_SFTR + 0x100*channel;
  error = (int)CAEN_DGTZ_WriteRegister(caenDgtz[board].handle,reg_addr,0x1);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::vetowidth(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_VET_WDTH + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 0,15, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::vetosteps(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_ADC_OFFS + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,16,17, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::bslfreeze(uint16_t board, uint16_t channel, uint32_t value, int &error)
{
  uint32_t reg_addr = PSD_BSL_FRTM + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr, 0, 9,value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::autoflush(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,0, 0, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trgprop(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr, 2, 2, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::dualtracemode(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,11,11, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::analogtracemode(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,12,13, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::analogtrace2mode(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,14,15, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::stcena(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,16,16, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::extraena(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,17,17, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::digprobe1PHA(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,20,23, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::digprobe1PSD(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,23,25, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::digprobe2(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,26,28, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::enadigprobe(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_BRD_CONF;
  SetRegisterSpecificBits(board,reg_addr,31,31, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::aggrorg(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_AGG_ORGA;
  SetRegisterSpecificBits(board,reg_addr,0,3, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::swtchon(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_CHA_SHTD;
  SetRegisterSpecificBits(board,reg_addr, 0, 0, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::acqdump(uint16_t board, ns2__acqInfo &rVal)
{
  uint32_t reg_data;
  uint32_t reg_addr = REG_ACQ_STAT;

  rVal.error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle,reg_addr,&reg_data);
  std::bitset<32>valBin = std::bitset<32>(reg_data);
  rVal.acqStatus   = valBin[ 2];
  rVal.evtReady    = valBin[ 3];  
  rVal.evtFull     = valBin[ 4];  
  rVal.clockSource = valBin[ 5]; 
  rVal.pllUnlock   = valBin[ 7];  
  rVal.boardReady  = valBin[ 8];  
  rVal.chShutDown  = valBin[19];
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::glbtrginfo(uint16_t board, ns2__glbtrgInfo &rVal)
{
  uint32_t reg_data;
  uint32_t reg_addr = REG_TRG_MASK;

  rVal.error = (int)CAEN_DGTZ_ReadRegister(caenDgtz[board].handle,reg_addr,&reg_data);
  std::bitset<32>valBin = std::bitset<32>(reg_data);
  rVal.couple0  = valBin[ 0];
  rVal.couple1  = valBin[ 1];
  rVal.couple2  = valBin[ 2];
  rVal.couple3  = valBin[ 3];
  rVal.couple4  = valBin[ 4];
  rVal.couple5  = valBin[ 5];
  rVal.couple6  = valBin[ 6];
  rVal.couple7  = valBin[ 7];
  rVal.lvdsTrg  = valBin[29];  
  rVal.extTrg   = valBin[30];
  rVal.sftTrg   = valBin[31];

  rVal.coincWidth = (uint16_t)((reg_data & 0xF00000)>>20);
  rVal.coincLvl   = (uint16_t)((reg_data & 0x7000000)>>24);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::enablecouple(uint16_t board, uint32_t couple, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_TRG_VLDM;
  SetRegisterSpecificBits(board,reg_addr, couple, couple, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::coinctw(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_TRG_MASK;
  SetRegisterSpecificBits(board,reg_addr,20,23, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::coinclvl(uint16_t board, uint32_t value, int &error)
{
  uint32_t reg_addr = REG_TRG_MASK;
  SetRegisterSpecificBits(board,reg_addr,24,26, value, error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// DPP-PHA RELATED REGISTERS
int CaenSLCService::acqstart(uint16_t board, uint32_t channel, uint32_t value, int&error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
        return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_ACQ_CTRL + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,0,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::finegain(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_FIN_GAIN + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,15,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::rccr2smooth(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  uint32_t reg_addr = PHA_MOV_AVRG + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0, 5,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::inprisetime(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  value /= (samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_INP_RTIM + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0, 7,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trarisetime(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_TRZ_RTIM + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,11,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::traflattop(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_TRZ_FTOP + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,11,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trapeakingtime(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_TRZ_PTIM + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,11,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::tradecaytime(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_TRZ_DTIM + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0,15,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::risetimevalidation(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // Taking care of the ns -> sampling clock units
  value /=samplingClockUnits;
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_VAL_WIND + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0, 9,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::peakholdoff(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  // following the rev.1 this time is expressed in steps of 
  // 16 ns for the XX725
  //  8 ns for the XX730
  // so 4*samplingClockUnits
  value /=(samplingClockUnits*4);
  value = (value > 0 ) ? value : 1 ;
  uint32_t reg_addr = PHA_PEK_HOFF + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0, 9,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::trarescaling(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,0, 5,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::decimation(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,8, 9,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::decimationgain(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,10,11,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::peakmean(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,12,13,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::rollover(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,26,26,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::puflag(uint16_t board, uint32_t channel, uint32_t value, int &error)
{
  if( caenDgtz[board].firmType!= CAEN_DGTZ_DPPFirmware_PHA){
    std::cerr << "Error: This register is only availble with the PHA firmware!" << std::endl;
    error = (int)CAEN_DGTZ_FunctionNotAllowed;
    return SOAP_DATAENCODINGUNKNOWN;
  }
  
  uint32_t reg_addr = REG_DPP_CTR1 + 0x100*channel;
  SetRegisterSpecificBits(board,reg_addr,27,27,value,error);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::isena(uint16_t board, uint16_t channel, bool& value)
{
  uint32_t reg_data;
  uint32_t reg_addr;
  CAEN_DGTZ_GetChannelEnableMask(caenDgtz[board].handle, &reg_data);
  value = (reg_data>>channel)&1;
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::setExtTrgLevel(uint16_t board, int level, int& error)
{
  if(level!=0 && level != 1) {
    return SOAP_OK;
    error = 17;
  }  
  error = (int)CAEN_DGTZ_SetIOLevel(caenDgtz[board].handle, (CAEN_DGTZ_IOLevel_t)level);
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

int CaenSLCService::getExtTrgLevel(uint16_t board, int &level)
{
  CAEN_DGTZ_IOLevel_t lev;
  int error = (int)CAEN_DGTZ_GetIOLevel(caenDgtz[board].handle, &lev);
  level = (int)lev;
  return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////










			       
