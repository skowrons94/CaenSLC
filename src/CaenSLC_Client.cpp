#include <bitset>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/json.h>

#include <CAENDigitizerType.h>

#include "CaenSLC.nsmap"

#include "soapCaenSLCProxy.h"

#include "registers/RegisterDPP_PHA.h"
#include "registers/RegisterDPP_PSD.h"

const char server[]="http://smluna/CaenSLC.cgi";
uint16_t nbChannel;

void EmitError(int error)
{
  printf("CAEN Digitizer error code recieved %i --> ",error);
  switch (error){
  case CAEN_DGTZ_Success                 : printf("Operation completed successfully\n");                   break ;
  case CAEN_DGTZ_CommError               : printf("Communication error \n");                               break ;
  case CAEN_DGTZ_GenericError            : printf("Unspecified error\n");                                  break ;
  case CAEN_DGTZ_InvalidParam            : printf("Invalid parameter\n");                                  break ;
  case CAEN_DGTZ_InvalidLinkType         : printf("Invalid Link Type\n");                                  break ;
  case CAEN_DGTZ_InvalidHandle           : printf("Invalid device handle\n");                              break ;
  case CAEN_DGTZ_MaxDevicesError         : printf("Maximum number of devices exceeded\n");                 break ;
  case CAEN_DGTZ_BadBoardType            : printf("The operation is not allowed on this type of board\n"); break ;
  case CAEN_DGTZ_BadInterruptLev         : printf("The interrupt level is not allowed\n");                 break ;
  case CAEN_DGTZ_BadEventNumber          : printf("The event number is bad\n");                            break ;
  case CAEN_DGTZ_ReadDeviceRegisterFail  : printf("Unable to read the registry\n");                        break ;
  case CAEN_DGTZ_WriteDeviceRegisterFail : printf("Unable to write into the registry\n");                  break ;
  case CAEN_DGTZ_InvalidChannelNumber    : printf("The channel number is invalid\n");                      break ;
  case CAEN_DGTZ_ChannelBusy             : printf("The Channel is busy\n");                                break ;
  case CAEN_DGTZ_FPIOModeInvalid         : printf("Invalid FPIO Mode\n");                                  break ;
  case CAEN_DGTZ_WrongAcqMode            : printf("Wrong acquisition mode\n");                             break ;
  case CAEN_DGTZ_FunctionNotAllowed      : printf("This function is not allowed for this module\n");       break ;
  case CAEN_DGTZ_Timeout                 : printf("Communication Timeout\n");                              break ;
  case CAEN_DGTZ_InvalidBuffer           : printf("The buffer is invalid\n");                              break ;
  case CAEN_DGTZ_EventNotFound           : printf("The event is not found\n");                             break ;
  case CAEN_DGTZ_InvalidEvent            : printf("The event is invalid\n");                               break ;
  case CAEN_DGTZ_OutOfMemory             : printf("Out of memory\n");                                      break ;
  case CAEN_DGTZ_CalibrationError        : printf("Unable to calibrate the board\n");                      break ;
  case CAEN_DGTZ_DigitizerNotFound       : printf("Unable to open the digitizer\n");                       break ;
  case CAEN_DGTZ_DigitizerAlreadyOpen    : printf("The Digitizer is already open\n");                      break ;
  case CAEN_DGTZ_DigitizerNotReady       : printf("The Digitizer is not ready to operate\n");              break ;
  case CAEN_DGTZ_InterruptNotConfigured  : printf("The Digitizer has not the IRQ configured\n");           break ;
  case CAEN_DGTZ_DigitizerMemoryCorrupted: printf("The digitizer flash memory is corrupted\n");            break ;
  case CAEN_DGTZ_DPPFirmwareNotSupported : printf("DPP firmware not supported\n");                         break ;
  case CAEN_DGTZ_InvalidLicense          : printf("Invalid Firmware License\n");                           break ;
  case CAEN_DGTZ_InvalidDigitizerStatus  : printf("The digitizer is found in a corrupted status\n");       break ;
  case CAEN_DGTZ_UnsupportedTrace        : printf("The given trace is not supported by the digitizer\n");  break ;
  case CAEN_DGTZ_InvalidProbe            : printf("Invalid probe\n");                                      break ;
  case CAEN_DGTZ_UnsupportedBaseAddress  : printf("The Base Address is not supported\n");                  break ;
  case CAEN_DGTZ_NotYetImplemented       : printf("The function is not yet implemented\n");                break ;
  default: printf("Unknown error message !\n");
  }
}

uint32_t readRegister(uint16_t boardID, CaenSLCProxy *serv, uint32_t regAddress)
{
  ns2__readReg rVal;
  if(serv->read(boardID,regAddress,rVal)==SOAP_OK){
    if(rVal.error==0){
      std::cout << "Register 0x" << std::hex << std::setw(4) << std::setfill('0') << regAddress
		<< " is set to 0x" << std::hex << std::setw(8) << std::setfill('0') << rVal.value << std::dec
		<< " --> " ;
      
      std::bitset<32> valBin = std::bitset<32>(rVal.value);
      for(int32_t j = 31 ; j >= 0 ; j--){
	std::cout << valBin[j] ;
	if((j+3)%4==3) std::cout << " ";
      }
      std::cout << std::endl;
      printf("TBU: %04X %08X\n",regAddress,rVal.value);
    } else(EmitError(rVal.error));
  }else{
    printf("SOAP: Communication error! \n");
  }
  return rVal.value;
}

std::string getRegisterCommentPHA(std::string comment,uint32_t regAddr, uint32_t regValue, 
				  ns2__returnInfo rInfo, ns2__returnBoard rBoard,
				  float lsb2mV){
  std::ostringstream regComment;
  regComment.str("");
  uint16_t steps = rBoard.samplingClock*4; std::string sunit="ns";
  switch(regAddr){
  case PHA_REC_LGTH:
    regComment << comment << " set to " << regValue*8*rBoard.samplingClock <<" ns";
    break;
  case PHA_INP_RANG:
    regComment << comment << " set to " << ((regValue==0) ? 2 : 0.5) <<" Vpp";
    break;
  case PHA_EVT_AGGR:
    regComment << comment << " set to " << regValue <<" event(s)";
    break;
  case PHA_PRE_TRGE:
    regComment << comment << " set to " << regValue*4*rBoard.samplingClock <<" ns";
    break;
  case PHA_MOV_AVRG:
    regComment << comment << " set to " ;
    switch(regValue){
    case 0x0  : regComment << " disabled"   ;break;
    case 0x1  : regComment << " 2 samples"  ;break;
    case 0x2  : regComment << " 4 samples"  ;break;
    case 0x4  : regComment << " 8 samples"  ;break;
    case 0x8  : regComment << " 16 samples" ;break;
    case 0x10 : regComment << " 32 samples" ;break;
    case 0x20 : regComment << " 64 samples" ;break;
    case 0x3F : regComment << " 128 samples";break;
    default: regComment << " not foreseen";
    }
    break;
  case PHA_INP_RTIM:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns (should be equal or 50% larger than the signal rise time)";
    break;
  case PHA_TRZ_RTIM:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_TRZ_FTOP:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_TRZ_PTIM:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns after the trigger";
    break;
  case PHA_TRZ_DTIM:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_TRG_TRSD:
    regComment << comment << " set to " << regValue*lsb2mV <<" mV";
    break;
  case PHA_VAL_WIND:
    if(regValue>0)
      regComment << comment << " set to " << regValue*rBoard.samplingClock <<" ns";
    else
      regComment << comment << " is disabled";
    break;
  case PHA_TRG_HOFF:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_PEK_HOFF:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_DPP_CTR1:
    break;
  case PHA_TRG_WDTH:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PHA_BAS_OFFS:
    regComment << comment << " set to " << regValue/655.35<< " % of the dynamic range (" << regValue << ")";
    break;
  case PHA_VET_WDTH:

    switch((regValue&0x30000) >> 16){
    case 0: steps = rBoard.samplingClock*4  ; sunit = " ns"; break;
    case 1: steps = rBoard.samplingClock    ; sunit = " us"; break;
    case 2: steps = rBoard.samplingClock*262; sunit = " us"; break;
    case 3: steps = rBoard.samplingClock*66 ; sunit = " ms"; break;
    default: steps = rBoard.samplingClock; sunit=" ns";
    }
    regComment << comment << " set to " << (regValue&0xFFFF)*steps << sunit ;
    break;
  default:
    regComment << "No comment yet" << std::endl;
  }
  
  return regComment.str();
}

void readAllRegistersPHA(uint16_t boardID, CaenSLCProxy *serv, std::string fileName)
{
  std::map<uint16_t, std::string> map_register;
  map_register[PHA_REC_LGTH] = "Record Length"                        ;
  map_register[PHA_INP_RANG] = "Input Dynamic Range"                  ;
  map_register[PHA_EVT_AGGR] = "Number of Events per Aggregate"       ;
  map_register[PHA_PRE_TRGE] = "Pre trigger"                          ;
  map_register[PHA_MOV_AVRG] = "RC-CR2 Smoothing Factor"              ;
  map_register[PHA_INP_RTIM] = "Input rise time"                      ;
  map_register[PHA_TRZ_RTIM] = "Trapezoid Rise Time"                  ;
  map_register[PHA_TRZ_FTOP] = "Trapezoid Flat top"                   ;
  map_register[PHA_TRZ_PTIM] = "Trapezoid Peaking time"               ;
  map_register[PHA_TRZ_DTIM] = "Trapezoid Decay time"                 ;
  map_register[PHA_TRG_TRSD] = "Trigger threshold"                    ;
  map_register[PHA_VAL_WIND] = "Rise Time Validation Window"          ;
  map_register[PHA_TRG_HOFF] = "Trigger Hold-off Width"               ;
  map_register[PHA_PEK_HOFF] = "Peak Hold-off"                        ;
  map_register[PHA_DPP_CTR1] = "DPP Algorithm Control"                ;
  map_register[PHA_TRG_WDTH] = "Shaped Trigger Width"                 ;
  map_register[PHA_BAS_OFFS] = "DC Offset"                            ;
  map_register[PHA_DPP_CTR2] = "DPP Algorithm Control 2"              ;
  map_register[PHA_FIN_GAIN] = "Fine Gain"                            ;
  map_register[PHA_VET_WDTH] = "Veto Width"                           ;
  map_register[PHA_BRD_CONF] = "Board Configuration"                  ;
  map_register[PHA_BRD_AGGR] = "Aggregate Organization"               ;
  map_register[PHA_ACQ_CTRL] = "Acquisition Control"                  ;
  map_register[PHA_TRG_MASK] = "Global Trigger Mask"                  ;
  map_register[PHA_TRG_OUT1] = "Front Panel TRG-OUT (GPO) Enable mask";
  map_register[PHA_PAN_IOCT] = "Front Panel I/O Control"              ;
  map_register[PHA_BRD_CHAN] = "Channel Enable Mask"                  ;
  map_register[PHA_TRG_VALM] = "Trigger Validation Mask"              ;
  map_register[PHA_VET_DELY] = "Extended Veto Delay"                  ;
  map_register[PHA_RDO_CTRL] = "Readout Control"                      ;
  map_register[PHA_BRD_IDEN] = "Board ID"                             ;
  map_register[PHA_AGG_XBLT] = "Aggregate Number per BLT"             ;
  
  Json::Value regList ;
  Json::Value dgtList;
  Json::StreamWriterBuilder builder;
  builder["commentStyle"]         = "None";
  builder["indentation"]          = "  "  ;
  builder["dropNullPlaceholders"] = true  ;

  ns2__returnInfo rInfo;
  if(serv->boardinfo(boardID,rInfo)!=SOAP_OK){
    printf("SOAP: Communication error! \n");
    return;
  }  
  ns2__returnBoard rBoard;
  if(serv->getBoardDescription(boardID,rBoard)!=SOAP_OK){
    printf("SOAP: Communication error! \n");
    return;
  }  
  dgtList["BoardName"]      = rInfo.ModelName.c_str();
  dgtList["Model"]          = rInfo.Model;
  dgtList["NbChannels"]     = rInfo.Channels;
  dgtList["SerialNumber"]   = rInfo.SerialNumber;
  dgtList["LinkNb"]         = rBoard.linkNumber;
  dgtList["BoardNb"]        = rBoard.boardNumber;
  dgtList["ConnectionType"] = rBoard.connType.c_str();
  dgtList["Firmware"]       = rBoard.FWName.c_str();
  
  Json::Value dgtz;
  dgtz["dgtzs"]=dgtList;
  float lsb2mV = 2/16.384;
  std::ostringstream regName;
  std::ostringstream regAddr;
  std::ostringstream regVals;
  std::string regComment;
  uint32_t regValue;
  uint32_t registerAddr;
  for (auto it=map_register.begin(); it!= map_register.end() ; ++it){
    regName.str("");regAddr.str("");regVals.str("");
    regName << "reg_" << std::hex << it->first;
    regAddr << "0x" << std::hex << it->first;
    registerAddr = std::stoul(regAddr.str(),nullptr,16);
    regValue = readRegister(boardID,serv,registerAddr);
    regVals << "0x" << std::hex << regValue;
    //Trying to get a clear and understandable comment ...
    regComment = getRegisterCommentPHA(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
    regList[regName.str()]["name"]   = it->second;
    regList[regName.str()]["channel"]= 0;
    regList[regName.str()]["address"]= regAddr.str();
    regList[regName.str()]["comment"]= regComment ;
    regList[regName.str()]["value"]  = regVals.str();

    //std::cout << regName.str() << " " << regAddr.str() << " " << regVals.str() << std::endl;
    if(it->first == 0x8180){
      uint16_t maxOffset = 0x4*rInfo.Channels/2;
      for(uint16_t offset = 0x4 ; offset < maxOffset ; offset += 0x4){ // Channel depend register
	regName.str("");regAddr.str("");regVals.str("");
	regName << "reg_" << std::hex << it->first+offset;
	regAddr << "0x" << std::hex << it->first+offset;
	registerAddr = std::stoul(regAddr.str(),nullptr,16);
	regVals << "0x" << std::hex << readRegister(boardID,serv,registerAddr);
	regComment = getRegisterCommentPHA(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
	regList[regName.str()]["name"]   =it->second;
	regList[regName.str()]["channel"]=offset/0x4;
	regList[regName.str()]["address"]=regAddr.str();
	regList[regName.str()]["comment"]=regComment ;
	regList[regName.str()]["value"]  =regVals.str();
      }
    }
    else if(it->first > 0x2000) continue;
    else {
      uint16_t maxOffset = 0x100*rInfo.Channels;
      for(uint16_t offset = 0x100 ; offset < maxOffset ; offset += 0x100){ // Channel depend register
	regName.str("");regAddr.str("");regVals.str("");
	regName << "reg_" << std::hex << it->first+offset;
	regAddr << "0x" << std::hex << it->first+offset;
	registerAddr = std::stoul(regAddr.str(),nullptr,16);
	regVals << "0x" << std::hex << readRegister(boardID,serv,registerAddr);
	//	  std::cout << regName.str() << " " << (offset>>8) << std::endl;
	regComment = getRegisterCommentPHA(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
	regList[regName.str()]["name"]   =it->second;
	regList[regName.str()]["channel"]=offset>>8;
	regList[regName.str()]["address"]=regAddr.str();
	regList[regName.str()]["comment"]=regComment ;
	regList[regName.str()]["value"]  =regVals.str();
	
      }
    }
    
  }

  dgtz["registers"]=regList;
  std::ofstream file; file.open(fileName.c_str());
  file << dgtz ;
  file.close();
  
}

std::string getRegisterCommentPSD(std::string comment,uint32_t regAddr, uint32_t regValue, 
				  ns2__returnInfo rInfo, ns2__returnBoard rBoard,
				  float lsb2mV){
  std::ostringstream regComment;
  regComment.str("");
  uint16_t steps = rBoard.samplingClock*4; std::string sunit="ns";
  switch(regAddr){
  case PSD_REC_LGTH:
    regComment << comment << " set to " << regValue*8*rBoard.samplingClock <<" ns";
    break;
  case PSD_INP_RANG:
    regComment << comment << " set to " << ((regValue==0) ? 2 : 0.5) <<" Vpp";
    break;
  case PSD_EVT_AGGR:
    regComment << comment << " set to " << regValue <<" event(s)";
    break;
  case PSD_PRE_TRGE:
    regComment << comment << " set to " << regValue*4*rBoard.samplingClock <<" ns";
    break;
  case PSD_SHO_GATE:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PSD_LON_GATE:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PSD_TRG_TRSD:
    regComment << comment << " set to " << regValue*lsb2mV <<" mV";
    break;
  case PSD_TRG_HOFF:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PSD_DPP_CTRL:
    break;
  case PSD_TRG_WDTH:
    regComment << comment << " set to " << regValue*rBoard.samplingClock*4 <<" ns";
    break;
  case PSD_DCU_OFFS:
    regComment << comment << " set to " << regValue/655.35<< " % of the dynamic range (" << regValue << ")";
    break;
  case PSD_VET_WDTH:

    switch((regValue&0x30000) >> 16){
    case 0: steps = rBoard.samplingClock*4  ; sunit = " ns"; break;
    case 1: steps = rBoard.samplingClock    ; sunit = " us"; break;
    case 2: steps = rBoard.samplingClock*262; sunit = " us"; break;
    case 3: steps = rBoard.samplingClock*66 ; sunit = " ms"; break;
    default: steps = rBoard.samplingClock; sunit=" ns";
    }
    regComment << comment << " set to " << (regValue&0xFFFF)*steps << sunit ;
    break;
  default:
    regComment << "No comment yet" << std::endl;
  }
  
  return regComment.str();
}


void readAllRegistersPSD(uint16_t boardID, CaenSLCProxy *serv, std::string fileName)
{
  std::map<uint16_t, std::string> map_register;
  map_register[PSD_REC_LGTH] = "Record Length"                        ;
  map_register[PSD_INP_RANG] = "Input Dynamic Range"                  ;
  map_register[PSD_EVT_AGGR] = "Number of Events per Aggregate"       ;
  map_register[PSD_PRE_TRGE] = "Pre trigger"                          ;
  map_register[PSD_CFD_SETT] = "CFD Settings"                         ;
  map_register[PSD_ZER_SUPP] = "Charge Zero Suppression Threshold"    ;
  map_register[PSD_SHO_GATE] = "Short Gate Width"                     ;
  map_register[PSD_LON_GATE] = "Long Gate Width"                      ;
  map_register[PSD_GAT_OFFS] = "Gate Offset"                          ;
  map_register[PSD_TRG_TRSD] = "Trigger threshold"                    ;
  map_register[PSD_FIX_BSLE] = "Fixed baseline"                       ;
  map_register[PSD_TRG_LATE] = "Trigger latency"                      ;
  map_register[PSD_TRG_WDTH] = "Shaped Trigger Width"                 ;
  map_register[PSD_TRG_HOFF] = "Trigger Hold-Off Width"               ;
  map_register[PSD_PSD_CUTT] = "Threshold for the PSD cut"            ;
  map_register[PSD_PUR_THSD] = "PUR-GAP Threshold"                    ;
  map_register[PSD_DPP_CTRL] = "DPP Algorithm Control"                ;
  map_register[PSD_DPP_CTR2] = "DPP Algorithm Control 2"              ;
  map_register[PSD_DCU_OFFS] = "DC Offset"                            ;
  map_register[PSD_VET_WDTH] = "Veto Width"                           ;
  map_register[PSD_BSL_FRTM] = "Early Baseline Freeze"                ;
  map_register[PSD_BRD_CONF] = "Board Configuration"                  ;
  map_register[PSD_BRD_AGGR] = "Aggregate Organization"               ;
  map_register[PSD_ACQ_CTRL] = "Acquisition Control"                  ;
  map_register[PSD_TRG_MASK] = "Global Trigger Mask"                  ;
  map_register[PSD_TRG_OUT1] = "Front Panel TRG-OUT (GPO) Enable mask";
  map_register[PSD_PAN_IOCT] = "Front Panel I/O Control"              ;
  map_register[PSD_BRD_CHAN] = "Channel Enable Mask"                  ;
  map_register[PSD_TRG_VALM] = "Trigger Validation Mask"              ;
  map_register[PSD_VET_DELY] = "Extended Veto Delay"                  ;
  map_register[PSD_RDO_CTRL] = "Readout Control"                      ;
  map_register[PSD_BRD_IDEN] = "Board ID"                             ;
  map_register[PSD_AGG_XBLT] = "Aggregate Number per BLT"             ;
  
  Json::Value regList ;
  Json::Value dgtList;
  Json::StreamWriterBuilder builder;
  builder["commentStyle"]         = "None";
  builder["indentation"]          = "  "  ;
  builder["dropNullPlaceholders"] = true  ;

  ns2__returnInfo rInfo;
  if(serv->boardinfo(boardID,rInfo)!=SOAP_OK){
    printf("SOAP: Communication error! \n");
    return;
  }  
  ns2__returnBoard rBoard;
  if(serv->getBoardDescription(boardID,rBoard)!=SOAP_OK){
    printf("SOAP: Communication error! \n");
    return;
  }  
  dgtList["BoardName"]      = rInfo.ModelName.c_str();
  dgtList["Model"]          = rInfo.Model;
  dgtList["NbChannels"]     = rInfo.Channels;
  dgtList["SerialNumber"]   = rInfo.SerialNumber;
  dgtList["LinkNb"]         = rBoard.linkNumber;
  dgtList["BoardNb"]        = rBoard.boardNumber;
  dgtList["ConnectionType"] = rBoard.connType.c_str();
  dgtList["Firmware"]       = rBoard.FWName.c_str();
  
  Json::Value dgtz;
  dgtz["dgtzs"]=dgtList;
  float lsb2mV = 2/16.384;
  std::ostringstream regName;
  std::ostringstream regAddr;
  std::ostringstream regVals;
  std::string regComment;
  uint32_t regValue;
  uint32_t registerAddr;
  for (auto it=map_register.begin(); it!= map_register.end() ; ++it){
    regName.str("");regAddr.str("");regVals.str("");
    regName << "reg_" << std::hex << it->first;
    regAddr << "0x" << std::hex << it->first;
    registerAddr = std::stoul(regAddr.str(),nullptr,16);
    regValue = readRegister(boardID,serv,registerAddr);
    regVals << "0x" << std::hex << regValue;
    //Trying to get a clear and understandable comment ...
    regComment = getRegisterCommentPSD(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
    regList[regName.str()]["name"]   = it->second;
    regList[regName.str()]["channel"]= 0;
    regList[regName.str()]["address"]= regAddr.str();
    regList[regName.str()]["comment"]= regComment ;
    regList[regName.str()]["value"]  = regVals.str();

    //std::cout << regName.str() << " " << regAddr.str() << " " << regVals.str() << std::endl;
    if(it->first == 0x8180){
      uint16_t maxOffset = 0x4*rInfo.Channels/2;
      for(uint16_t offset = 0x4 ; offset < maxOffset ; offset += 0x4){ // Channel depend register
	regName.str("");regAddr.str("");regVals.str("");
	regName << "reg_" << std::hex << it->first+offset;
	regAddr << "0x" << std::hex << it->first+offset;
	registerAddr = std::stoul(regAddr.str(),nullptr,16);
	regVals << "0x" << std::hex << readRegister(boardID,serv,registerAddr);
	regComment = getRegisterCommentPSD(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
	regList[regName.str()]["name"]   =it->second;
	regList[regName.str()]["channel"]=offset/0x4;
	regList[regName.str()]["address"]=regAddr.str();
	regList[regName.str()]["comment"]=regComment ;
	regList[regName.str()]["value"]  =regVals.str();
      }
    }
    else if(it->first > 0x2000) continue;
    else {
      uint16_t maxOffset = 0x100*rInfo.Channels;
      for(uint16_t offset = 0x100 ; offset < maxOffset ; offset += 0x100){ // Channel depend register
	regName.str("");regAddr.str("");regVals.str("");
	regName << "reg_" << std::hex << it->first+offset;
	regAddr << "0x" << std::hex << it->first+offset;
	registerAddr = std::stoul(regAddr.str(),nullptr,16);
	regVals << "0x" << std::hex << readRegister(boardID,serv,registerAddr);
	//	  std::cout << regName.str() << " " << (offset>>8) << std::endl;
	regComment = getRegisterCommentPSD(it->second,it->first,regValue,rInfo,rBoard,lsb2mV);
	regList[regName.str()]["name"]   =it->second;
	regList[regName.str()]["channel"]=offset>>8;
	regList[regName.str()]["address"]=regAddr.str();
	regList[regName.str()]["comment"]=regComment ;
	regList[regName.str()]["value"]  =regVals.str();
	
      }
    }
    
  }

  dgtz["registers"]=regList;
  std::ofstream file; file.open(fileName.c_str());
  file << dgtz ;
  file.close();
  
}

void loadRegisterFile(uint16_t boardID, CaenSLCProxy *serv, std::string file){
  std::ifstream inFile ; inFile.open(file.c_str());
  if(!inFile.is_open()){
    printf("Error: Cannot open the file %s",file.c_str());
  }
  std::string aLine;
  std::string regAddr,regValue;
  
  int error = 0;
  uint32_t regadd, regval;
  while(std::getline(inFile,aLine)){
    if(aLine.empty()) continue;
    if(aLine[0]=='#') continue;
    std::stringstream oneLine (aLine);
    oneLine >> regAddr >> regValue;
    regadd = std::stoul(regAddr,nullptr,16); 
    regval = std::stoul(regValue,nullptr,16); 
    if(serv->write(boardID,regadd,regval,error)==SOAP_OK){ 
      if(error==0){
	//	printf("Register %04X written\n",regadd);
      }else EmitError(error) ;
    }else{
      	printf("SOAP: Communication error! \n");
    }
  }
}

void displayBoardInfo(uint16_t boardID, CaenSLCProxy *serv){

  ns2__returnInfo rVal;
  if(serv->boardinfo(boardID,rVal)==SOAP_OK){
    printf("===== BOARD INFO =====\n");
    printf("ModelName:\t%12s\n" ,(rVal.ModelName).c_str());
    printf("Model:\t%20i\n"     ,rVal.Model);
    printf("Channels:\t%12i\n"  ,rVal.Channels);
    printf("FormFactor:\t%12i\n",rVal.FormFactor);
    printf("FamilyCode:\t%12i\n",rVal.FamilyCode);
    printf("ROC FW rel:\t%13s\n",(rVal.ROC_FirmwareRel).c_str());
    printf("AMC FW rel:\t%12s\n",(rVal.AMC_FirmwareRel).c_str());
    printf("Serial no.:\t%12i\n",rVal.SerialNumber);
    printf("ADC_NBits:\t%12i\n" ,rVal.ADC_NBits);
    printf("===== BOARD INFO =====\n");
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void displaySingleChannelInfo(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch)
{
  ns2__channelInfo rVal;
  if(serv->channelinfo(boardID,ch,rVal)==SOAP_OK){
    if(rVal.error==0){
      printf("===== CHANNEL %3i PARAMETERS =====\n",ch);
      if(rVal.chEnable==1){
	printf("\t Channel \t enabled \n");
      }else{
	printf("\t Channel \t disabled \n");
      }
      printf("==================================\n");
    }else{EmitError(rVal.error);}
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setcfdDelay(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value){
  int error=0;
  if(serv->cfddelay(boardID,ch,value,error)==0){
    if(error==0){
      printf(" -- CFD Delay of channel %i set to %u samples\n",ch,value);
    }else {EmitError(error);}
  }else{
    printf("SOAP: Communication error! \n");
  }   
}

void setcfdFraction(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value){
  int error=0;
  int fracVal[4]={25,50,75,100};
  if(serv->cfdfraction(boardID,ch,value,error)==0){
    if(error==0){
      printf(" -- CFD Fraction of channel %i set to %u %% \n",ch,fracVal[value]);
    }else {EmitError(error);}
  }else{
    printf("SOAP: Communication error! \n");
  }   
}

void setcfdInterpol(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value){
  int error;
  std::string interVal[4] = {"first","second","third","fourth"};
  if(serv->cfdinterpolation(boardID,ch,value,error)==0){
    if(error==0){
      printf(" -- CFD Interpolation of channel %i will use the %s sample before and after the zero-crossing\n",ch,interVal[value].c_str());
    }else {EmitError(error);}
  }else{
    printf("SOAP: Communication error! \n");
  }
    
}

void getcfdDump(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch)
{
  ns2__cfdInfo rVal;
  int fracVal[4]={25,50,75,100};
  std::string interVal[4] = {"first","second","third","fourth"};
  if(serv->cfddump(boardID,ch,rVal)==0){
    if(rVal.error==0){
      printf(" -- CH%.2u CFD Delay         %3u \n",(uint32_t)ch,(uint32_t)rVal.cfdDelay);
      printf(" -- CH%.2u CFD Fraction      %3u%% \n",(uint32_t)ch,fracVal[(uint32_t)rVal.cfdFraction]);
      printf(" -- CH%.2u CFD Interpolation %s samples \n",(uint32_t)ch,interVal[(uint32_t)rVal.cfdInterpolation].c_str());
    }else {EmitError(rVal.error);}
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setchEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, bool value)
{
  int error;
  if(serv->chenable(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value)
	printf(" -- Channel %2u Channel enabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Channel disabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settraceLength(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->tracelength(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Trace length set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setadcrange(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, bool value)
{
  int error;
  if(serv->inputrange(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value)
	printf(" -- Channel %2u ADC Range set to 2.0 Vpp\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u ADC Range set to 0.5 Vpp\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setevtAggregate(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->evtaggregate(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Number of events per aggregate set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpreTrg(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->pretrigger(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Number of pre-trigger samples in the short trace set to to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setforcedFlush(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch)
{
  int error;
  if(serv->forceflush(boardID,ch,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Forced flush of the data done\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setzeroSuppThr(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->zerosupp(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Zero suppresion Qthr set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setshortGate(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->shortgate(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Short gate intergration set %u samples \n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setlongGate(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->longgate(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Long gate integration set to %u samples\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setgateOffset(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->gateoffset(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Integration gate offset before the trigger set to %u samples\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgThreshold(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->trgthres(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Trigger threshold set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setfixedBaseline(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->fixbaseline(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Fixed baseline set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgWidth(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->shtrgwidth(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Shaped trigger width for the coincidences set to %u\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgHoldOff(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->trgholdoff(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Trigger hold off set to %u time samples",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpsdcutThres(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, float value)
{
  int error;
  if(serv->cutpsdthres(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u PSD Threshold set to %f\n",(uint32_t)ch, value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpurThres(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint16_t value)
{
  int error;
  if(serv->purthres(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Pile-up rejection Peak-Valley set to %u LSB\n",(uint32_t)ch,(uint32_t) value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setchargeSensitivity(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value){
  int error;
  std::string charge_0[6] = {"5 fC", "20 fC", "80 fC",
			     "320 fC", "1.28 pC", "5.12 pC"};
  if(serv->chargesense(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Charge Sensitivity set to %s\n",(uint32_t)ch,charge_0[value].c_str());
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setchargePedestal(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->chargepedestal(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value)
	printf(" -- Channel %2u Charge pedestal disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Charge pedestal enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgPropagation(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->trgcounting(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value)
	printf(" -- Channel %2u Propagation of rejected events enabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Propagation of rejected events disabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setdiscriType(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->discritype(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Discrimination using a Leading Edge\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Discrimination using a CFD\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpurPropEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->purenable(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Pile-Up events are counted as trigger\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Pile-Up events are not counted as trigger\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setsigSource(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->adcsigsource(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Internal test pulse disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Internal test pulse enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settestRate(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  int rates[4]= {1,10,100,1000};
  if(serv->adctestrate(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Internal pulseer set to %i kHz\n",(uint32_t)ch,rates[value]);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setbaselineRecalc(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->bslcalc(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Recalculation of the baseline after the long gate disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Recalculation of the baseline after the long gate enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}
void setsigPolarity(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->adcpolarity(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Positive input pulse\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Negative input pulse\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgMode(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  std::string mode[4]={"Each channel can self-trigger independently from the other channels.",
		       "Coincidence mode",
		       " ",
		       "An -coincidence mode"};
  if(serv->trgmode(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Trigger mode set to %s\n",(uint32_t) ch,mode[value].c_str());
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void setbaselineCalc(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  std::string mode[5]={"Fixed: the baseline value is fixed",
		       "16 samples",
		       "64 samples",
		       "256 samples",
		       "1024 samples"};
  if(serv->bslcalc(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %2u Calculation of the baseline is %s",(uint32_t) ch,mode[value].c_str()); 
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void setselftrgEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->selftrgena(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Self trigger disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Self trigger enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void setqthrEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->chargereject(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Zero suppresion using the long gate is disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Zero suppresion using the long gate is enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpurEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->purreject(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Pile-Up rejection is disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Pile-Up rejection is enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpsdcutLow(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->psdcutlow(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u PSD cut on gamma disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u PSD cut on gamma enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setpsdcutAbv(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->psdcutabv(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u PSD cut on neutron disabled \n",(uint32_t)ch);
      else
	printf(" -- Channel %2u PSD cut on neutron enabled \n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setoverrangeEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->overrange(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Over-range rejection disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Over-range rejection enabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgHysteresis(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->trghysteresis(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Trigger hysteresis disabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Trigger hysteresis enabled \n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void settrgOppPolarity(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->opppolarity(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Channel %2u Inhibit of CFD on opposite polarity signals enabled\n",(uint32_t)ch);
      else
	printf(" -- Channel %2u Inhibit of CFD on opposite polarity signals disabled\n",(uint32_t)ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setLocalShMode(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;

  if(serv->localtrgmode(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      switch (value){
      case 0 : printf(" -- Local shaped trigger set to the AND of the two channels\n"); break;
      case 1 : printf(" -- Local shaped trigger set to the even channel\n"); break;
      case 2 : printf(" -- Local shaped trigger set to the odd  channel\n"); break;
      case 3 : printf(" -- Local shaped trigger set to the OR of the two channels\n"); break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setLocalShEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->enablelocaltrg(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==0)
	printf(" -- Local shaped trigger of channel %u is disabled\n",ch);
      else
	printf(" -- Local shaped trigger of channel %u is enabled\n",ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setLocalShOptions(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->localtrgopt(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      switch (value){
      case 0: printf("Using the option defined by the local trigger mode\n");break;
      case 1: printf("validation from paired channel AND mother board\n");break;
      case 2: printf("validation from paired channel OR  mother board\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setLocalValMode(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->localtrgval(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      switch (value){
      case 1: printf("val0 = val1 = signal from mother-board mask\n");break;
      case 2: printf("AND (val0 = val1 = trg0 AND trg1)\n");break;
      case 3: printf("OR (val0 = val1 = trg0 OR trg1)\n");break; 
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setLocalValEnable(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->enablelocalval(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if (value==0)
	printf(" -- Channel %u local trigger validation is disabled\n",ch);
      else
	printf(" -- Channel %u local trigger validation is enabled\n",ch);      
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setExtraConfiguration(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->confextra(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      switch (value){
      case 0: printf("bits[31:16] = Extended Time Stamp, bits[15:0] = Baseline * 4;\n");break;
      case 1: printf("bits[31:16] = Extended Time Stamp, bits[15:0] = Flags;\n");break;
      case 2: printf("bits[31:16] = Extended Time Stamp, bits[15:10] = Flags, bits[9:0] = Fine Time Stamp;\n");break;
      case 4: printf("bits[31:16] = Lost Trigger Counter, bits[15:0] = Total Trigger Counter;\n");break;
      case 5: printf("bits[31:16] = Positive Zero Crossing, bits[15:0] = Negative Zero Crossing;\n");break;
      case 7: printf("Fixed value = 0x12345678 (debug use only).\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setEnableSmoothing(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->usesmooth(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      switch (value){
      case 0: printf(" -- Channel %u smoothing is disabled \n",ch);break;
      case 1: printf(" -- Channel %u smoothing is enabled \n",ch);break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setConfSmoothing(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->confsmooth(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u smoothing is using %u samples\n",ch,(uint32_t)(std::pow(2,value)));
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTriggerCounterMode(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->conftrgcr(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u step of the trigger counter set to ", ch);
      switch (value){
      case 0: printf("1024 \n");break;
      case 1: printf(" 128 \n");break;
      case 2: printf("8192 \n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setConfVeto(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->localtrgopt(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u veto source set to ",ch);
      switch (value){
      case 0: printf("disabled\n");break;
      case 1: printf("the veto signal is common among all channels\n");break;
      case 2: printf("the veto signal is individually set for the couple of channels\n");break;
      case 3: printf("the veto signal comes from events saturating inside the gate\n");break; 
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setMarkSaturation(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->enablesat(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u saturation mark is ",ch);
      switch (value){
      case 0: printf("disabled\n");break;
      case 1: printf("enable\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setVetoMode(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->vetomode(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u veto operating mode set to ",ch);
      switch (value){
      case 0: printf("the Veto is used after the charge integration to discard the event data\n");break;
      case 1: printf("the Veto is used to inhibit the self-triggers\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setEnableTSReset(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->rststampveto(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u time stamp reset trough external veto (TRG-IN) is ",ch);
      switch (value){
      case 0: printf("disabled\n");break;
      case 1: printf("enabled\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setADCOffset(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->adcoffset(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u ADC offset set to %u \n",ch,value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setIdvSoftTrg(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch)
{
  int error;
  if(serv->indsofttrg(boardID,ch,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u software trigger sent \n",ch);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void setVetoWidth(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->vetowidth(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u veto signal width set to %u\n",ch,value);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setVetoSteps(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->vetosteps(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u steps of the veto signals set to ",ch);
      switch (value){
      case 0: printf("  16 ns\n");break;
      case 1: printf("   4 us\n");break;
      case 2: printf("1048 us\n");break;
      case 3: printf(" 264 ms\n");break;
      }
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setBaselineFreeze(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->bslfreeze(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Channel %u baseline freeze time set to %u ns\n",ch,value*8);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}



void setCoupleTrgEna(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  ch = ch/2;
  if(serv->enablecouple(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      if(value==1)
	printf(" -- Local trigger of channels %u and %u participate to the global trigger \n",ch, ch+1);
      else
	printf(" -- Local trigger of channels %u and %u participate to the global trigger \n",ch, ch+1);
    }else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


void setTriggerShapingTime(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->inprisetime(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Time constant of the PHA fast discriminator of channel %i set to %i ns\n",ch, value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setRCCR2Smoothing(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->rccr2smooth(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Moving average for the RC-CR2 signal set to %i samples for channel %i\n",value*2,ch);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapFineGain(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->finegain(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Fine gain of the trapezoidal filter of channel %i set to %i\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapRiseTime(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->trarisetime(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Shaping time of the energy filter of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapFlatTop(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->traflattop(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Float Top length of the energy filter of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapPeakTime(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->trapeakingtime(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Peaking time of the energy filter of channel %i set to %i ns after trigger\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapDecayTime(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->tradecaytime(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Decay time of the signal of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setRiseTimeValidationWind(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->risetimevalidation(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Rise time rejection windows of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setPeakHoldOff(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->peakholdoff(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Peak Hold-off window of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapRescaling(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->trarescaling(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Trapezoidal rescaling of channel %i set to 2^%i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setDecimationFactor(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->decimation(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Averaging of the input signal of channel %i set to %i samples\n",ch,value*2);
    } else{
      EmitError(error);
    } 
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setDecimationGain(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  if(serv->decimationgain(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Decimation gain of channel %i set to %i ns\n",ch,value);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}

void setTrapAveragingWindow(uint16_t boardID, CaenSLCProxy *serv, uint16_t ch, uint32_t value)
{
  int error;
  int realVal[4]={1,4,16,64};
  if(serv->peakmean(boardID,ch,value,error)==SOAP_OK){
    if(error==0){
      printf(" -- Averaging on the flat top for channel %i set to %i samplesns\n",ch,realVal[value]);
    } else{
      EmitError(error);
    }
  }else{
    printf("SOAP: Communication error! \n");
  }
}


int main(int argc, char **argv)
{
  namespace bpo = boost::program_options;
  CaenSLCProxy *serv = new CaenSLCProxy();
  serv->soap->connect_timeout = 10; // connect within 10s
  serv->soap->send_timeout = 5;     // send timeout is 5s
  serv->soap->recv_timeout = 5;     // receive timeout is 5s
  
  try{
    //input parameters
    int16_t                  chNum;
    std::string              regAddress ;
    std::vector<std::string> writePars;
    int  cfdDly   ;
    int  cfdFrac  ;
    int  cfdInt   ;
    bool chEna    ;

    uint16_t stclen;
    bool adcRange  ;
    uint16_t evtAggr;
    uint16_t preTrg;
    uint16_t boardID = 0;
    uint16_t chargeThr, shortGate, longGate, gateOffs, trgThres,
      fixBsl, trgWidth, trgHold, purCut;
    float psdCut;
    // variables for the register 0x1n80
    uint32_t adcGain, adcPede, trgCnt, trgType, purEna, adcSig, testRte, bslRec, adcPol,  
      trgMode, bslSpl, selfTrg, qrejEna, purrejEna, psdClow, psdCabv, stcEna,
      orrejEna, trgHys, trgOpp;

    uint32_t ltrgMode, enaltrg, ltrgOpt, lvalMode, enalval, cfextra, enaextra, enaSmoo, cfSmoo, trgcrConf, 
      vetoConf, vetoMode, vetoWdth, vetoStps, satMark, tstamprst, adcOffs, bslFreeze, autoFlsh, trgProp, dtMode, algMode,   
      digPbe1, digPbe2, digPbena, aggrOrg, swOnCh, trgEna, coincWdt, coincLvl;
   
    uint32_t trgShtime,trgRcCr,trzFineGain,trzRiseTime,trzFlatTop,trzPeakTime,trzDecayTime,rTimeValWind,peakHoldOff,trzRescaling,decFactor,decGain,trzEneAver;

    std::vector<uint32_t> acqMode;
    uint32_t maxAggr;
    uint32_t idlePer;
    bool idleEna;
    bpo::options_description desc("Allowed options");
    std::string fileName;
    desc.add_options()
      ("help,h"                              , "print usage message")
      // GLOBAL PARAMETERS
      ("load,l"      , bpo::value(&fileName) , "Load a list of register value from a file")
      ("info,i"                              , "get board info")
      ("board"       , bpo::value(&boardID)  , "board id (default is 0)")
      ("chdump"                              , "display channel info")
      ("channel,c"   , bpo::value(&chNum)    ,"select the channel to apply the modification to")
      ("enable,e"                            , "enable digitizer acquisition")
      ("disable,d"                           , "disable digitizer acquisition")
      ("reset,r"                             , "resets the digitizer" )
      ("clear"                               , "clear digitizer buffers")
      ("softtrg,s"                           , "sends a software trigger")
      ("readallpha"  , bpo::value(&fileName) , "read all pha registers of the board and dump into file")
      ("readallpsd"  , bpo::value(&fileName) , "read all psd registers of the board and dump into file")
      ("calibrate"   , "calibrate the ADCs")
      ("acqmode"     , bpo::value< std::vector<uint32_t> > (&acqMode)->multitoken()
      ,"Set acquistion mode (0) and saved parameters (2)")
      ("setmaxaggr"  , bpo::value(&maxAggr)  ,"Set maximum number of aggregate per block transfer")
      ("enaettt"                             , "Enable the extended time tag")
      // GENERAL ACCESS TO REGISTERS
      ("read"        , bpo::value(&regAddress)
       ,"get the value of the specified register (in hex)")
      ("write,w"     , bpo::value< std::vector<std::string> > (&writePars)->multitoken()
       ,"set the value of the specified register (both in hex)")
      // CFD PARAMETERS
      ("cfddelay"    , bpo::value(&cfdDly)   ,"Set CFD delay in time sample unit (2 ns for 73)")
      ("cfdfraction" , bpo::value(&cfdFrac)  ,"Set CFD Fraction (0 - 25% ; 1 - 50% ; 2 - 75%; 3- 100%)")
      ("cfdinterpol" , bpo::value(&cfdInt)   ,"Set CFD Interpolation (0- sample before and after ; 1- second saples ; 2 -third ; 3- fourth)")
      ("cfddump"     , "get the CFD settings")
      // CHANNEL CONTROL
      ("chena"       , bpo::value(&chEna)    , "enable/disable a channel")
      //DPP-PSD specific
      ("psddump"                             , "Read all the register related to the PSD")
      ("stclen"      , bpo::value(&stclen)   , "Set short traces length")
      ("stcena"      , bpo::value(&stcEna)   , "Enable the short traces recording")
      ("adcrange"    , bpo::value(&adcRange) , "Set range to 2 Vpp (0) or 0.5 Vpp (1)")
      ("evtaggr"     , bpo::value(&evtAggr)  , "Set number of events per aggregate")
      ("pretrg"      , bpo::value(&preTrg)   , "Set number of samples before the trigger")
      ("flush"                               , "Forced flush of the data in memory")
      ("thrcharge"   , bpo::value(&chargeThr), "Set the threshold the charge zero suppression")
      ("shortgate"   , bpo::value(&shortGate), "Set the length of the short gate integration")
      ("longgate"    , bpo::value(&longGate) , "Set the length of the long gate integration")
      ("gateoffs"    , bpo::value(&gateOffs) , "Set the offset for the integration gate to start before the trigger")
      ("trgthres"    , bpo::value(&trgThres) , "Set the trigger threshold")
      ("fixbsl"      , bpo::value(&fixBsl)   , "Set a fix value for the baseline")
      ("trgwidth"    , bpo::value(&trgWidth) , "Set the logical trigger width for internal the coincidences")
      ("trghold"     , bpo::value(&trgHold)  , "Set the trigger Hold off width to block the triggering")
      ("thrpsd"      , bpo::value(&psdCut)   , "Set the PSD cut (value is between 0 and 1)")
      ("thrpur"      , bpo::value(&purCut)   , "Set the PUR cut")
      ("adcgain"     , bpo::value(&adcGain)  , "Set the charge sensitivity \n\t 0: 5fC \n\t 1: 20fC \n\t 2: 80fC \n\t 3: 320fC \n\t 4: 1.28pC \n\t 5: 5.12pC")
      ("adcpedestal" , bpo::value(&adcPede)  , "Set an offset of 1024 to the charge")
      ("trgrejprop"  , bpo::value(&trgCnt)   , "Enable/disable the propagation of rejected events to the global trigger")
      ("discri"      , bpo::value(&trgType)  , "Set the local trigger to LE (0) or CFD (1)")
      ("purena"      , bpo::value(&purEna)   , "Enable the pile-rejection" )
      ("adcsigsource", bpo::value(&adcSig)   , "Set the adc signal to the intenal on (1) or external (0)")
      ("adctestrate" , bpo::value(&testRte)  , "Set the rate of the internal pulse emulator")
      ("bslrecalc"   , bpo::value(&bslRec)   , "Enable/disable the recalculation of the baseline after the long gate")
      ("sigpol"      , bpo::value(&adcPol)   , "Set the signal polarity to positive (0) or negative (1)")
      ("trgmode"     , bpo::value(&trgMode)  , "Set the trigger mode, 0 - self trigger, 1 - coincidence, 3 - veto")
      ("bslsamples"  , bpo::value(&bslSpl)   , "Set the number of samples used for the baseline calculation")
      ("selftrg"     , bpo::value(&selfTrg)  , "Enable/disable the channel self-triggering")
      ("qrejena"     , bpo::value(&qrejEna)  , "Enable/disable the zero suppression based on the charge")
      ("purrejena"   , bpo::value(&purrejEna), "Enable/disable the rejection based on the pile-up")
      ("psdcutlow"   , bpo::value(&psdClow)  , "Enable/disable PSD cut below threshold (to cut on gammas)")
      ("psdcutabv"   , bpo::value(&psdCabv)  , "Enable/disable PSD cut above threshold (to cut on neutrons)")
      ("orrejena"    , bpo::value(&orrejEna) , "Enable/disable the rejection based on the over range in the long gate integration")
      ("trghys"      , bpo::value(&trgHys)   , "Enable/disable the trigger hysteresis (inhibit trigger on the tail)")
      ("trgopppol"   , bpo::value(&trgOpp)   , "Enable/disable the detection of signals of opposite polarity to inhibit the zero crossing on CFD")
      ("localtrgmode", bpo::value(&ltrgMode) , "Local Trigger mode \n\t 0: AND \n\t 1: Even only \n\t 2: Odd only \n\t 3: OR")
      ("localtrgena" , bpo::value(&enaltrg)  , "Enable/disable the local shaped trigger")
      ("localtrgopt" , bpo::value(&ltrgOpt)  , "Local trigger validation option")
      ("localvalmode", bpo::value(&lvalMode) , "Local validation mode \n\t 1: MB \n\t 2: AND \n\t 3: OR")
      ("localvalena" , bpo::value(&enalval)  , "Enable/disable the local validation")
      ("extraconf"   , bpo::value(&cfextra)  , "Configuration of the extra word")
      ("extraena"    , bpo::value(&enaextra) , "Enable/disable Extra word")
      ("smoothena"   , bpo::value(&enaSmoo)  , "Enable smoothing of the signal")
      ("smoothconf"  , bpo::value(&cfSmoo)   , "Input smoothing factor (2^parameter sample)")
      ("trgcrconf"   , bpo::value(&trgcrConf), "Select the step for the trigger counter rate")
      ("vetoconf"    , bpo::value(&vetoConf) , "Defines the veto source")
      ("vetomode"    , bpo::value(&vetoMode) , "Veto signal operating mode")
      ("vetowidth"   , bpo::value(&vetoWdth) , "Veto signal width")
      ("vetosteps"   , bpo::value(&vetoStps) , "Veto signal steps")
      ("satmark"     , bpo::value(&satMark)  , "Mark saturated pulses")
      ("tstamprst"   , bpo::value(&tstamprst), "Reset TimeStamp on external signal")
      ("adcoffset"   , bpo::value(&adcOffs)  , "Set ADC offset")
      ("indsfttrg"                           , "Send individual software trigger if channel is specified")
      ("bslfreeze"   , bpo::value(&bslFreeze), "Baseline freeze time after the beginning of the integration gate")
      ("autoflush"   , bpo::value(&autoFlsh) , "Enable disble the auto flush time")
      ("trgprop"     , bpo::value(&trgProp)  , "Trigger propagation")
      ("dtmode"      , bpo::value(&dtMode)   , "Dual Trace mode")
      ("algmode"     , bpo::value(&algMode)  , "Analog Probe selection")
      ("digpbe1"     , bpo::value(&digPbe1)  , "Digital virtual probe 1 selection")
      ("digpbe2"     , bpo::value(&digPbe2)  , "Digital virtual probe 2 selection")
      ("digpbeena"   , bpo::value(&digPbena) , "Enable/disable digital virtual probe")
      ("aggrorg"     , bpo::value(&aggrOrg)  , "Set number of aggregates in memory")
      ("swchon"      , bpo::value(&swOnCh)   , "Switch on all the channels that have been switched off by the temperature control")
      ("adcdump"                             , "Dump ADC parameters")
      ("gltrgdump"                           , "Dump Global Trigger parameters")
      ("acqdump"                             , "Dump ACQ parameters")
      ("trgena"      , bpo::value(&trgEna)   , "Enable/Disable trigger for a specified channel (works by pair of successive channels)")
      ("coincwidth"  , bpo::value(&coincWdt) , "Set coincidence time window width")
      ("coinclvl"    , bpo::value(&coincLvl) , "Set minimum fold for the coincidence trigger")
      ("idleperiod"  , bpo::value(&idlePer)  , "Set the idle period (in ms)")
      ("idleena"     , bpo::value(&idleEna)  , "Enable/Disable the idle cycle")
      //DPP-PHA specific
      ("trgshtime"   , bpo::value(&trgShtime), "Input rise time (in ns)")
      ("trgrc"       , bpo::value(&trgRcCr)  , "Number of samples of a moving average for the trigger:\n\t 0: disable \n\t 1: 2 samples \n\t 2: 4 samples \n\t 4: 8 samples \n\t 8: 16 samples \n\t 16: 32 samples \n\t 32: 64 samples \n\t 63: 128 samples")
      ("trzfinegain" , bpo::value(&trzFineGain) ,"Adjust trapezoidal fine gain")
      ("trzrisetime" , bpo::value(&trzRiseTime) ,"Trapezoidal rise time (in ns)")
      ("trzflattop"  , bpo::value(&trzFlatTop)  ,"Trapezoidal flat top (in ns)")
      ("trzpeaktime" , bpo::value(&trzPeakTime) ,"Peaking time with respect to the trigger (in ns)")
      ("trzdecaytime", bpo::value(&trzDecayTime),"Decay time (should correspond to the one of the pre-amp, in ns)")
      ("rtimeval"    , bpo::value(&rTimeValWind),"Rise time validation window length (in ns)")
      ("peakhoff"    , bpo::value(&peakHoldOff) ,"Minimal distance between two trapezoids (in ns)")
      ("trzrescaling", bpo::value(&trzRescaling),"Trapezoid rescaling")
      ("decimfactor" , bpo::value(&decFactor)   ,"Decimation factor for the energy filter")
      ("decimgain"   , bpo::value(&decGain)     ,"Decimation gain")
      ("trzeneavg"   , bpo::value(&trzEneAver)  ,"Number of samples to average for the energy calculation")

      ;
    
    //used variables
    uint32_t registerAddr;
    uint32_t regNewValue;
    uint32_t regOldValue;

    int error;
    uint32_t regValue = 0 ;
    bpo::variables_map vm;
    bpo::store(parse_command_line(argc, argv, desc, bpo::command_line_style::unix_style ^ bpo::command_line_style::allow_short), vm);
    bpo::notify(vm);
   
    if(serv->getNbChan(boardID,nbChannel)!=SOAP_OK){ 
      printf("SOAP: Communication error - not able to get the number of channels! \n");
      return -2;
    }
    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 0;
    }
    else if (vm.count("load")){
      loadRegisterFile(boardID,serv,fileName);
    }
    else if (vm.count("info")){
      displayBoardInfo(boardID,serv);
    }
    else if (vm.count("chdump")){
      // check if options contains the specific channel otherwise we do all of them
      if(vm.count("channel")){
	displaySingleChannelInfo(boardID,serv,chNum);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  displaySingleChannelInfo(boardID,serv,chAll);
	}
      }
    } 
    else if (vm.count("enable")){
      if(serv->start(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer acquisition enabled\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("disable")){
      if(serv->stop(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer acquisition disabled\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("reset")){
      if(serv->reset(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer reset to default settings\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("clear")){
      if(serv->clear(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer data buffers cleared\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("softtrg")){
      if(serv->swtrg(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer software trigger sent\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("calibrate")){
      if(serv->calibrate(boardID,error)==SOAP_OK){
	if(error==0){
	  printf("Digitizer calibration done\n");
	}else(EmitError(error));
      }else{
	printf("SOAP: Communication error! \n");
      }
    }
    else if (vm.count("read")){
      registerAddr = std::stoul(regAddress,nullptr,16);
      printf("Trying to read register 0x%04X\n",registerAddr);
      readRegister(boardID,serv,registerAddr);
    }
    else if(vm.count("readallpha")){
      readAllRegistersPHA(boardID,serv,fileName);
    }
    else if(vm.count("readallpsd")){
      readAllRegistersPSD(boardID,serv,fileName);
    }
    else if (vm.count("write")){
      // first we read the initial value;
      if(writePars.size()<2){
	printf("Error: You need to specify the register address and value\n");
	return -2;
      }
      registerAddr = std::stoul(writePars[0],nullptr,16);
      regNewValue = std::stoul(writePars[1],nullptr,16);
      regOldValue = readRegister(boardID,serv,registerAddr);
      
      if(serv->write(boardID,registerAddr,regNewValue,error)==SOAP_OK){
      	if(error==0){
      	  printf("Register %04X written will check the new value\n",registerAddr);
      	}else(EmitError(error));
      }else{
      	printf("SOAP: Communication error! \n");
      }
      regNewValue = readRegister(boardID,serv,registerAddr);
      if(regNewValue == std::stoul(writePars[1],nullptr,16)){
	printf("Register %04X set to the desired value (0x%08X) \n", registerAddr,regNewValue);
      }else{
	printf("Error: The desired value was not set 0x%08X instead of 0x%08X \n",regNewValue,std::stoul(writePars[1],nullptr,16));
	return -3;
      }
    }
    else if(vm.count("acqmode")){
      uint32_t modeAcq = 0 ;
      uint32_t savePar = 2 ;
      if(acqMode.size()<2)
	printf("Warning: acqmode requires two parameters -- will asume the default ones (0 and 2)");
      else{
	modeAcq = acqMode[0]; savePar=acqMode[1];
      }
      if(serv->acqmode(boardID,modeAcq,savePar,error)==SOAP_OK){
	if(error==0)
	  printf(" -- Acquisition mode set properly\n");
	else
	  EmitError(error);
      }else{
	printf("SOAP: Communication error!\n");
      }
    }
    else if(vm.count("enaettt")){
      if(serv->enaettt(boardID,error)==SOAP_OK){
	if(error==0)
	  printf(" -- Extended time tag enabled\n");
	else EmitError(error);
      }else
	printf("SOAP: Communication error \n");
    }
    else if(vm.count("setmaxaggr")){
      if(serv->setmaxaggr(boardID,maxAggr,error)==SOAP_OK){
	if(error==0)
	  printf(" -- Maximum number of aggregate per block transfer set to 2\n");
	else
	  EmitError(error);
      }else{
	printf("SOAP: Communication error!\n");
      }
    }
    else if(vm.count("cfddelay")){
      if(vm.count("channel")){
	setcfdDelay(boardID,serv,chNum,cfdDly);
      }else{
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setcfdDelay(boardID,serv,chAll,cfdDly);
	}
      }
    }
    else if(vm.count("cfdfraction")){
      if(vm.count("channel")){
	setcfdFraction(boardID,serv,chNum,cfdFrac);
      }else{
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setcfdFraction(boardID,serv,chAll,cfdFrac);
	}
      }
    }
    else if(vm.count("cfdinterpol")){
      if(vm.count("channel")){
	setcfdInterpol(boardID,serv,chNum,cfdInt);
      }else{
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setcfdInterpol(boardID,serv,chAll,cfdInt);
	}
      }
    }
    else if(vm.count("cfddump")){
      if(vm.count("channel")){
	getcfdDump(boardID,serv,chNum);
      }else{
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  getcfdDump(boardID,serv,chAll);
	}
      }
    }
    else if(vm.count("chena")){
      if(vm.count("channel")){
	setchEnable(boardID,serv,chNum,chEna);
      }else{
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setchEnable(boardID,serv,chAll,chEna);
	}
      }
    }
    else if(vm.count("psddump")){
      if(vm.count("channel")){
	//	getPSDInfo(boardID,serv,chNum);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  //	  getPSDInfo(boardID,serv,chAll);
	}
      }
    }

				  
    else if(vm.count("stclen")){
      if(vm.count("channel")){
	settraceLength(boardID,serv,chNum,stclen);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settraceLength(boardID,serv,chAll,stclen);
	}
      }
    }
    else if(vm.count("adcrange"    )){
      if(vm.count("channel")){
	setadcrange(boardID,serv,chNum,adcRange);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setadcrange(boardID,serv,chAll,adcRange);
	}
      }
    }
    else if(vm.count("evtaggr"     )){
      if(vm.count("channel")){
	setevtAggregate(boardID,serv,chNum,evtAggr);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setevtAggregate(boardID,serv,chAll,evtAggr);
	}
      }
    }
    else if(vm.count("pretrg"      )){
      if(vm.count("channel")){
	setpreTrg(boardID,serv,chNum,preTrg);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpreTrg(boardID,serv,chAll,preTrg);
	}
      }
    }
    else if(vm.count("flush"       )){
      if(vm.count("channel")){
	setforcedFlush(boardID,serv,chNum);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setforcedFlush(boardID,serv,chAll);
	}
      }
    }
    else if(vm.count("thrcharge"   )){
      if(vm.count("channel")){
	setzeroSuppThr(boardID,serv,chNum,chargeThr);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setzeroSuppThr(boardID,serv,chAll,chargeThr);
	}
      }
    }
    else if(vm.count("shortgate"   )){
      if(vm.count("channel")){
	setshortGate(boardID,serv,chNum,shortGate);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setshortGate(boardID,serv,chAll,shortGate);
	}
      }
    }
    else if(vm.count("longgate"    )){
      if(vm.count("channel")){
	setlongGate(boardID,serv,chNum,longGate);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setlongGate(boardID,serv,chAll,longGate);
	}
      }
    }
    else if(vm.count("gateoffs"    )){
      if(vm.count("channel")){
	setgateOffset(boardID,serv,chNum,gateOffs);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setgateOffset(boardID,serv,chAll,gateOffs);
	}
      }
    }
    else if(vm.count("trgthres"    )){
      if(vm.count("channel")){
	settrgThreshold(boardID,serv,chNum,trgThres);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgThreshold(boardID,serv,chAll,trgThres);
	}
      }
    }
    else if(vm.count("fixbsl"      )){
      if(vm.count("channel")){
	setfixedBaseline(boardID,serv,chNum,fixBsl);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setfixedBaseline(boardID,serv,chAll,fixBsl);
	}
      }
    }
    else if(vm.count("trgwidth"    )){
      if(vm.count("channel")){
	settrgWidth(boardID,serv,chNum,trgWidth);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgWidth(boardID,serv,chAll,trgWidth);
	}
      }
    }
    else if(vm.count("trghold"     )){
      if(vm.count("channel")){
	settrgHoldOff(boardID,serv,chNum,trgHold);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgHoldOff(boardID,serv,chAll,trgHold);
	}
      }
    }
    else if(vm.count("thrpsd"      )){
      if(vm.count("channel")){
	setpsdcutThres(boardID,serv,chNum,psdCut);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpsdcutThres(boardID,serv,chAll,psdCut);
	}
      }
    }
    else if(vm.count("thrpur"      )){
      if(vm.count("channel")){
	setpurThres(boardID,serv,chNum,purCut);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpurThres(boardID,serv,chAll,purCut);
	}
      }
    }
    else if(vm.count("adcgain"     )){
      if(adcGain > 5) {printf("Error: ADC Gain has to be <= 4\n"); return -10;}
      if(vm.count("channel")){
	setchargeSensitivity(boardID,serv,chNum,adcGain);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setchargeSensitivity(boardID,serv,chAll,adcGain);
	}
      }
    }
    else if(vm.count("adcpedestal" )){
      if(adcPede>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setchargePedestal(boardID,serv,chNum,adcPede);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setchargePedestal(boardID,serv,chAll,adcPede);
	}
      }
    }
    else if(vm.count("trgrejprop"  )){
      if(trgCnt>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	settrgPropagation(boardID,serv,chNum,trgCnt);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgPropagation(boardID,serv,chAll,trgCnt);
	}
      }
    }
    else if(vm.count("discri"      )){
      if(trgType>1) {printf("Error: Parameter is 0 or 1 \n");return -2;}
      if(vm.count("channel")){
	setdiscriType(boardID,serv,chNum,trgType);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setdiscriType(boardID,serv,chAll,trgType);
	}
      }
    }
    else if(vm.count("purena"      )){
      if(purEna>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setpurPropEnable(boardID,serv,chNum,purEna);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpurPropEnable(boardID,serv,chAll,purEna);
	}
      }
    }
    else if(vm.count("adcsigsource")){
      if(adcSig>1) {printf("Error: Parameter is 0 or 1\n");return -2;}
      if(vm.count("channel")){
	setsigSource(boardID,serv,chNum,adcSig);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setsigSource(boardID,serv,chAll,adcSig);
	}
      }
    }
    else if(vm.count("adctestrate" )){
      if(testRte>3) {printf("Error: Test rate parameter between 0 and 3\n");return -16;}
      if(vm.count("channel")){
	settestRate(boardID,serv,chNum,testRte);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settestRate(boardID,serv,chAll,testRte);
	}
      }
    }
    else if(vm.count("bslrecalc"   )){
      if(bslRec>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setbaselineRecalc(boardID,serv,chNum,bslRec);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setbaselineRecalc(boardID,serv,chAll,bslRec);
	}
      }
    }
    else if(vm.count("sigpol"      )){
      if(adcPol>1){printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setsigPolarity(boardID,serv,chNum,adcPol);
      }else {
	  for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	    setsigPolarity(boardID,serv,chAll,adcPol);
	  }
      }
    }
    else if(vm.count("trgmode"     )){
      if(trgMode>3 && trgMode!=2) {printf("Error: Parameter is between 0 and 3 (but not 2)\n"); return -2;}
      if(vm.count("channel")){
	settrgMode(boardID,serv,chNum,trgMode);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgMode(boardID,serv,chAll,trgMode);
	}
      }
    }
    else if(vm.count("bslsamples"  )){
      if(bslSpl>4) {printf("Error: Parameter is between 0 and 4\n");return -2;}
      if(vm.count("channel")){
	setbaselineCalc(boardID,serv,chNum,bslSpl);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setbaselineCalc(boardID,serv,chAll,bslSpl);
	}
      }
    }
    else if(vm.count("selftrg"     )){
      if(selfTrg>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setselftrgEnable(boardID,serv,chNum,selfTrg);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setselftrgEnable(boardID,serv,chAll,selfTrg);
	}
      }
    }
    else if(vm.count("qrejena"     )){
      if(qrejEna>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setqthrEnable(boardID,serv,chNum,qrejEna);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setqthrEnable(boardID,serv,chAll,qrejEna);
	}
      }
    }
    else if(vm.count("purrejena"   )){
      if(purrejEna>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setpurEnable(boardID,serv,chNum,purrejEna);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpurEnable(boardID,serv,chAll,purrejEna);
	}
      }
    }
    else if(vm.count("psdcutlow"   )){
      if(psdClow>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setpsdcutLow(boardID,serv,chNum,psdClow);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpsdcutLow(boardID,serv,chAll,psdClow);
	}
      }
    }
    else if(vm.count("psdcutabv"   )){
      if(psdCabv>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setpsdcutAbv(boardID,serv,chNum,psdCabv);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setpsdcutAbv(boardID,serv,chAll,psdCabv);
	}
      }
    }
    else if(vm.count("orrejena"    )){
      if(orrejEna>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setoverrangeEnable(boardID,serv,chNum,orrejEna);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setoverrangeEnable(boardID,serv,chAll,orrejEna);
	}
      }
    }
    else if(vm.count("trghys"      )){
      if(trgHys>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	settrgHysteresis(boardID,serv,chNum,trgHys);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgHysteresis(boardID,serv,chAll,trgHys);
	}
      }
    }
    else if(vm.count("trgopppol"   )){
      if(trgOpp>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	settrgOppPolarity(boardID,serv,chNum,trgOpp);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  settrgOppPolarity(boardID,serv,chAll,trgOpp);
	}
      }
    }
    else if(vm.count("localtrgmode"   )){
      if(ltrgMode>3) {printf("Error: Parameter is 0 to 3\n"); return -2;}
      if(vm.count("channel")){
	setLocalShMode(boardID,serv,chNum,ltrgMode);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setLocalShMode(boardID,serv,chAll,ltrgMode);
	}
      }
    }
    else if(vm.count("localtrgena"   )){
      if(enaltrg>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setLocalShEnable(boardID,serv,chNum,enaltrg);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setLocalShEnable(boardID,serv,chAll,enaltrg);
	}
      }
    }
    else if(vm.count("localtrgopt"   )){
      if(ltrgOpt>2) {printf("Error: Parameter is 0 to 2\n"); return -2;}
      if(vm.count("channel")){
	setLocalShOptions(boardID,serv,chNum,ltrgOpt);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setLocalShOptions(boardID,serv,chAll,ltrgOpt);
	}
      }
    }
    else if(vm.count("localvalmode"   )){
      if(lvalMode>3 && lvalMode != 0) {printf("Error: Parameter is 0 to 3\n"); return -2;}
      if(vm.count("channel")){
	setLocalValMode(boardID,serv,chNum,lvalMode);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setLocalValMode(boardID,serv,chAll,lvalMode);
	}
      }
    }
    else if(vm.count("localvalena"   )){
      if(enalval>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setLocalValEnable(boardID,serv,chNum,enalval);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setLocalValEnable(boardID,serv,chAll,enalval);
	}
      }
    }
    else if(vm.count("extraconf"   )){
      if(cfextra>7 && cfextra!=3 && cfextra!=6) {printf("Error: Parameter is 0 to 7 but not 3 nor 6\n"); return -2;}
      if(vm.count("channel")){
	setExtraConfiguration(boardID,serv,chNum,cfextra);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setExtraConfiguration(boardID,serv,chAll,cfextra);
	}
      }
    }
    else if(vm.count("smoothena"   )){
      if(enaSmoo>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setEnableSmoothing(boardID,serv,chNum,enaSmoo);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setEnableSmoothing(boardID,serv,chAll,enaSmoo);
	}
      }
    }
    else if(vm.count("smoothconf"   )){
      if(cfSmoo>4) {printf("Error: Parameter is 0 to 4\n"); return -2;}
      if(vm.count("channel")){
	setConfSmoothing(boardID,serv,chNum,cfSmoo);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setConfSmoothing(boardID,serv,chAll,cfSmoo);
	}
      }
    }
    else if(vm.count("trgcrconf"   )){
      if(trgcrConf>2) {printf("Error: Parameter is 0 to 2\n"); return -2;}
      if(vm.count("channel")){
	setTriggerCounterMode(boardID,serv,chNum,trgcrConf);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setTriggerCounterMode(boardID,serv,chAll,trgcrConf);
	}
      }
    }
    else if(vm.count("vetoconf"   )){
      if(vetoConf>3) {printf("Error: Parameter is 0 to 3\n"); return -2;}
      if(vm.count("channel")){
	setConfVeto(boardID,serv,chNum,vetoConf);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setConfVeto(boardID,serv,chAll,vetoConf);
	}
      }
    }
    else if(vm.count("satmark"   )){
      if(satMark>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setMarkSaturation(boardID,serv,chNum,satMark);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setMarkSaturation(boardID,serv,chAll,satMark);
	}
      }
    }
    else if(vm.count("vetomode"   )){
      if(vetoMode>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setVetoMode(boardID,serv,chNum,vetoMode);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setVetoMode(boardID,serv,chAll,vetoMode);
	}
      }
    }
    else if(vm.count("tstamprst"   )){
      if(tstamprst>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(vm.count("channel")){
	setEnableTSReset(boardID,serv,chNum,tstamprst);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setEnableTSReset(boardID,serv,chAll,tstamprst);
	}
      }
    }
    else if(vm.count("adcoffset"   )){
      if(adcOffs>65535) {printf("Error: Parameter is 0 to 65535\n"); return -2;}
      if(vm.count("channel")){
	setADCOffset(boardID,serv,chNum,adcOffs);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setADCOffset(boardID,serv,chAll,adcOffs);
	}
      }
    }
    else if(vm.count("indsfttrg"   )){
      if(vm.count("channel")){
	setIdvSoftTrg(boardID,serv,chNum);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setIdvSoftTrg(boardID,serv,chAll);
	}
      }
    }
    else if(vm.count("vetowidth"   )){
      if(vetoWdth>65535) {printf("Error: Parameter is 0 to 65535\n"); return -2;}
      if(vm.count("channel")){
	setVetoWidth(boardID,serv,chNum,vetoWdth);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setVetoWidth(boardID,serv,chAll,vetoWdth);
	}
      }
    }
    else if(vm.count("vetosteps"   )){
      if(vetoStps>3) {printf("Error: Parameter is 0 to 3\n"); return -2;}
      if(vm.count("channel")){
	setVetoSteps(boardID,serv,chNum,vetoStps);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setVetoSteps(boardID,serv,chAll,vetoStps);
	}
      }
    }
    else if(vm.count("bslfreeze"   )){
      if(bslFreeze>511) {printf("Error: Parameter is 0 or 511\n"); return -2;}
      if(vm.count("channel")){
	setBaselineFreeze(boardID,serv,chNum,bslFreeze);
      }else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setBaselineFreeze(boardID,serv,chAll,bslFreeze);
	}
      }
    }
    else if(vm.count("autoflush"   )){
      if(autoFlsh>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->autoflush(boardID,autoFlsh,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board automatic flushing of the data is %s\n",(autoFlsh == 0) ? "DISABLED" : "ENABLED");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("trgprop"   )){
      if(trgProp>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->trgprop(boardID,trgProp,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board Trigger propagation of the individual trigger is %s\n",(trgProp == 0) ? "DISABLED" : "ENABLED");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("dtmode"   )){
      if(dtMode>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->dualtracemode(boardID,dtMode,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board Dual Trace mode is set to %u\n",dtMode);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("algmode"   )){
      if(algMode>2) {printf("Error: Parameter is 0 to 2\n"); return -2;}
      if(serv->analogtracemode(boardID,algMode,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board Analog Probe selection set to %u\n",algMode);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("stcena"   )){
      if(stcEna>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->stcena(boardID,stcEna,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board waveform recording is %s\n",(stcEna == 0) ? "DISABLED" : "ENABLED");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("extraena"   )){
      if(enaextra>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->extraena(boardID,enaextra,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board EXTRA word in data flow is %s\n",(enaextra == 0) ? "DISABLED" : "ENABLED");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("digpbe1"   )){
      if(digPbe1>7) {printf("Error: Parameter is 0 to 7\n"); return -2;}
      if(serv->digprobe1PSD(boardID,digPbe1,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board digital virtual probe 1 set to %u\n",digPbe1);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("digpbe2"   )){
      if(digPbe2>1) {printf("Error: Parameter is 0 to 7\n"); return -2;}
      if(serv->digprobe2(boardID,digPbe2,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board digital virtual probe 2 set to %u\n",digPbe2);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("digpbeena"   )){
      if(digPbena>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->enadigprobe(boardID,digPbena,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board digital virtual probes are %s into the data stream",
		 (digPbena == 0) ? "passed" : "not passed");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("aggrorg"   )){
      if(aggrOrg>1) {printf("Error: Parameter is 2 to 10\n"); return -2;}
      if(serv->aggrorg(boardID,aggrOrg,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board number of aggregates set to %u\n",(uint32_t)(std::pow(2,aggrOrg)));
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("swchon"   )){
      if(swOnCh>1) {printf("Error: Parameter is 0 or 1\n"); return -2;}
      if(serv->swtchon(boardID,swOnCh,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Board channel switched off by temperature protection are on \n");
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("acqdump"   )){
      ns2__acqInfo rVal;
      if(serv->acqdump(boardID,rVal)==SOAP_OK){
	if(rVal.error==0){
	  printf("  =======  ACQ STATUS ======= \n");
	  printf("\t Acquisition is %s \n",(rVal.acqStatus == 0) ? "STOPPED" : "RUNNING");
	  printf("\t %s event is available for readout\n", (rVal.evtReady == 0) ? "No" : "At least one");
	  printf("\t %s \n",(rVal.evtFull==0) ? "no channel has reached the FULL condition"
		 : "the maximum number of events to be read is reached"     );
	  printf("\t Clock source is set to %s \n", (rVal.clockSource  == 0) ? "internal" : "external" );
	  printf("\t %s \n",(rVal.pllUnlock==0) ? "PLL has had an unlock condition since the last register read access" :
		 "PLL has not had any unlock condition since the last register read access");
	  printf("\t %s \n", (rVal.boardReady==0) ? "board is not ready to start the acquisition" :
		 "board is ready to start the acquisition");
	  printf("\t %s \n", (rVal.chShutDown==0) ? "channels are ON" : "channels are in shutdown"  );
	  printf("  =========================== \n");
	}else{
	  EmitError(rVal.error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("gltrgdump"   )){
      ns2__glbtrgInfo rVal;
      if(serv->glbtrginfo(boardID,rVal)==SOAP_OK){
	if(rVal.error==0){
	  bool trgchEnabled[8] = {rVal.couple0,rVal.couple1,rVal.couple2,rVal.couple3,
				  rVal.couple4,rVal.couple5,rVal.couple6,rVal.couple7};
	  printf("  =======  GLOBAL TRIGGER STATUS ======= \n");
	  for(uint16_t couple = 0 ; couple < 8 ; couple++){
	    printf("\t Trigger of Couple %i (ch: %i-%i) is %s \n",couple,couple*2,couple*2+1,
		   (trgchEnabled[8]==1) ? "ENABLED" : "DISABLED");
	  }
	  printf("\t Coincidence time window set to %u ns\n",rVal.coincWidth*8);
	  printf("\t Coincidence min fold set to %u \n" ,rVal.coincLvl);
	  printf("\t LVDS trigger is %s \n",(rVal.lvdsTrg==0) ? "DISABLED" : "ENABLED");
	  printf("\t External trigger is %s \n",(rVal.extTrg==0) ? "DISABLED" : "ENABLED");
	  printf("\t Software trigger is %s \n",(rVal.sftTrg==0) ? "DISABLED" : "ENABLED");
	  printf("  ====================================== \n");
	}else{
	  EmitError(rVal.error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }
    }
    else if(vm.count("coincwidth")){
      if(coincWdt>7){printf("Error: parameter has to be <= 7\n");return -2;}
      if(serv->coinctw(boardID,coincWdt,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Coincidence time window set to %u ns \n",8*coincWdt);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }      
    }
    
    else if(vm.count("coinclvl")){
      if(coincLvl>15){printf("Error: parameter has to be <= 15\n");return -2;}
      if(serv->coinclvl(boardID,coincLvl,error)==SOAP_OK){
	if(error==0){
	  printf(" -- Coincidence min fold set to %u\n",coincLvl);
	}else{
	  EmitError(error);
	}
      }else{
	printf("SOAP: Communication Error!\n");
      }

    }
    else if(vm.count("trgena")){
      if(trgEna > 1) {printf("Error: parameter has to be 0 or 1\n"); return -10;}
      if(vm.count("channel")){
	setCoupleTrgEna(boardID,serv,chNum,trgEna);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll +=2){
	  setCoupleTrgEna(boardID,serv,chAll,trgEna);
	}
      }
    }
//    else if(vm.count("idleperiod")){
//      if(serv->setidleper(idlePer,error)){
//	if(error==0){
//	  printf(" -- Idle cycle period set to %u ms",idleperiod);
//	}else
//	  EmitError(error);
//      }else{
//	printf("SOAP: Communication Error!\n");
//      }
//    }
//    else if(vm.count("idleena")){
//      if(serv->setidleper(idlePer,error)){
//	if(error==0){
//	  printf(" -- Idle cycle %s",idleperiod);
//	}else
//	  EmitError(error);
//      }else{
//	printf("SOAP: Communication Error!\n");
//      }
//    }
    else if(vm.count("trgshtime")){
      if(vm.count("channel")){
	setTriggerShapingTime(boardID,serv,chNum,trgShtime);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTriggerShapingTime(boardID,serv,chAll,trgShtime);
	}
      }
    }
    else if(vm.count("trgrc")){
      if(vm.count("channel")){
	setRCCR2Smoothing(boardID,serv,chNum,trgRcCr);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setRCCR2Smoothing(boardID,serv,chAll,trgRcCr);
	}
      }
    }
    else if(vm.count("trzfinegain")){
      if(vm.count("channel")){
	setTrapFineGain(boardID,serv,chNum,trzFineGain);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll ++){
	  setTrapFineGain(boardID,serv,chAll,trzFineGain);
	}
      }
    }
    else if(vm.count("trzrisetime")){
      if(vm.count("channel")){
	setTrapRiseTime(boardID,serv,chNum,trzRiseTime);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapRiseTime(boardID,serv,chAll,trzRiseTime);
	}
      }
    }
    else if(vm.count("trzflattop")){
      if(vm.count("channel")){
	setTrapFlatTop(boardID,serv,chNum,trzFlatTop);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapFlatTop(boardID,serv,chAll,trzFlatTop);
	}
      }
    }
    else if(vm.count("trzpeaktime")){
      if(vm.count("channel")){
	setTrapPeakTime(boardID,serv,chNum,trzPeakTime);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapPeakTime(boardID,serv,chAll,trzPeakTime);
	}
      }
    }
    else if(vm.count("trzdecaytime")){
      if(vm.count("channel")){
	setTrapDecayTime(boardID,serv,chNum,trzDecayTime);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapDecayTime(boardID,serv,chAll,trzDecayTime);
	}
      }
    }
    else if(vm.count("rtimeval")){
      if(vm.count("channel")){
	setRiseTimeValidationWind(boardID,serv,chNum,rTimeValWind);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setRiseTimeValidationWind(boardID,serv,chAll,rTimeValWind);
	}
      }
    }
    else if(vm.count("peakhoff")){
      if(vm.count("channel")){
	setPeakHoldOff(boardID,serv,chNum,peakHoldOff);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setPeakHoldOff(boardID,serv,chAll,peakHoldOff);
	}
      }
    }
    else if(vm.count("trzrescaling")){
      if(vm.count("channel")){
	setTrapRescaling(boardID,serv,chNum,trzRescaling);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapRescaling(boardID,serv,chNum,trzRescaling);
	}
      }
    }
    else if(vm.count("decimfactor")){
      if(vm.count("channel")){
	setDecimationFactor(boardID,serv,chNum,decFactor);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setDecimationFactor(boardID,serv,chAll,decFactor);
	}
      }
    }
    else if(vm.count("decimgain")){
      if(vm.count("channel")){
	setDecimationGain(boardID,serv,chNum,decGain);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setDecimationGain(boardID,serv,chAll,decGain);
	}
      }
    }
    else if(vm.count("trzeneavg")){
      if(vm.count("channel")){
	setTrapAveragingWindow(boardID,serv,chNum,trzEneAver);}
      else {
	for(uint16_t chAll = 0 ; chAll < nbChannel ; chAll++){
	  setTrapAveragingWindow(boardID,serv,chAll,trzEneAver);
	}
      }
    }
    



    else{
      printf("Error: Unknown option!\n");
      std::cout << desc << std::endl;
      return 0;
    }
    
  }
  catch(std::exception &e){
    std::cerr << e.what() << std::endl;
  }
  

  serv->destroy();
  return 1;
}
