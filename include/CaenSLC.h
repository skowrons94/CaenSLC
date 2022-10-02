class ns2__readReg
{ public:
  uint32_t value;
  int      error;
};

class ns2__returnBoard
{
  std::string connType   ;
  std::string ModelName  ;
  std::string FWName     ;
  uint16_t fwType        ;
  uint16_t linkNumber    ;
  uint16_t boardNumber   ;
  uint16_t samplingClock ;
  int handle;
};

class ns2__returnInfo
{ public:
  std::string    ModelName;
  uint32_t       Model;
  uint32_t       Channels;
  uint32_t       FormFactor;
  uint32_t       FamilyCode;
  std::string    ROC_FirmwareRel;
  std::string    AMC_FirmwareRel;
  uint32_t       SerialNumber;
  uint32_t       PCB_Revision;
  uint32_t       ADC_NBits;
};

class ns2__channelInfo
{public:
  bool           chEnable        ;
  bool           trEnable        ;
  bool           inputRange      ;
  std::string    trgType         ;
  uint32_t       triggerThreshold;
  uint32_t       nSamples        ;
  uint32_t       temperature     ;
  uint32_t       adcOffset       ;
  int            error           ;
  bool           isPositive      ;
  bool           isSPIbusy       ;
  bool           isADCcal        ;
  bool           isADCdown       ;
};

class ns2__psdInfo
{ public:
  uint16_t      cfdDelay          ;
  uint16_t      cfdFraction       ;
  uint16_t      cfdPoints         ;
  uint16_t      chargeThreshold   ;
  uint16_t      shortGate         ;
  uint16_t      longGate          ;
  uint16_t      gateOffset        ;
  uint16_t      shapedTrgWidth    ;
  uint16_t      trgHoldOff        ;
  uint16_t      psdCut            ;
  uint16_t      purThreshold      ;
  uint16_t      chargeSensitivity ;
  bool          isChargePedestal  ;
  bool          isTrgCounting     ;
  bool          isDiscrMode       ;
  bool          isPileUp          ;
  uint16_t      trgMode           ;
  uint16_t      baselineMean      ;
  bool          isChargeCut       ;
  bool          isPUR             ;
  bool          isOverRangeReject ;
  bool          isTrgHysteresis   ;
  bool          localShapedTrg    ;
  uint16_t      localShapedTrgMode;
  bool          localTrgVal       ;
  uint16_t      localTrgValMode   ;
  uint16_t      extras            ;
  int           error             ;
};

class ns2__phaInfo
{ public:
  uint16_t      inputRiseTime     ;
  uint16_t      riseTime          ;
  uint16_t      flatTop           ;
  uint16_t      peakingTime       ;
  uint16_t      decayTime         ;
  uint16_t      riseTimeValWindow ;
  uint16_t      trgHoldOff        ;
  uint16_t      peakHoldOff       ;
  uint16_t      trpzRescaling     ;
  uint16_t      decimationGain    ;
  uint16_t      decimation        ;
  uint16_t      fineGain          ;
  uint16_t      peakMean          ;
  uint16_t      rccr2             ;
  uint16_t      extras2           ;
  uint16_t      trgMode           ;
  bool          localShapedTrg    ;
  uint16_t      localShapedTrgMode;
  bool          localTrgVal       ;
  uint16_t      localTrgValMode   ;
  uint16_t      baselineAvgWindow ;
  bool          isNegative        ;
  bool          roFlag            ;
  bool          puFlag            ;
  int           error             ;
  
};

class ns2__generalInfo
{ public:
  uint16_t       analogProbePSD     ;
  uint16_t       analogProbe1PHA    ;
  uint16_t       analogProbe2PHA    ;
  uint16_t       digitalProbe1PHA   ;
  uint16_t       digitalProbe2PHA   ;
  uint16_t       digitalProbe1PSD   ;
  uint16_t       digitalProbe2PSD   ;
  bool           inputRange         ;
  bool           isAutoDataFlush    ;
  bool           isTrgPropagation   ;
  bool           isExtras           ;
  bool           isDualTrace        ;
  bool           isTrace            ;
  bool           isPSD              ;
  bool           isPHA              ;
  int            error              ;
};

class ns2__cfdInfo
{public:
  uint16_t cfdDelay;
  uint16_t cfdFraction;
  uint16_t cfdInterpolation;
  int      error;
};

class ns2__acqInfo{
 public:
  bool     acqStatus   ;
  bool     evtReady    ;
  bool     evtFull     ;
  bool     clockSource ;
  bool     pllUnlock   ;
  bool     boardReady  ;
  bool     chShutDown  ;
  int      error;
};

class ns2__glbtrgInfo{
 public:
  bool     couple0   ;
  bool     couple1   ;
  bool     couple2   ;
  bool     couple3   ;
  bool     couple4   ;
  bool     couple5   ;
  bool     couple6   ;
  bool     couple7   ;
  uint16_t coincWidth;
  uint16_t coincLvl  ;
  bool     lvdsTrg   ;
  bool     extTrg    ;
  bool     sftTrg    ;
  int      error     ;
};

//gsoap ns service name:            CaenSLC Simple CAEN Slow Control service 
//gsoap ns service protocol:        SOAP
//gsoap ns service style:           rpc
//gsoap ns service encoding:        encoded
//gsoap ns service namespace:       http://smluna:8080/CaenSLC.wsdl
//gsoap ns service location:        http://smluna:8080/CaenSLC.cgi

//gsoap ns schema namespace:        urn:CaenSLC
//gsoap ns service method:          read Read the value of a specific register
int ns__read(uint16_t board, uint32_t address,  ns2__readReg &rVal);

//gsoap ns service method:          write Set the value of a specific register
int ns__write(uint16_t board, uint32_t address, uint32_t value, int &error);

//gsoap ns service method:          reset Resets the digitizer
int ns__reset(uint16_t board, int &error);

//gsoap ns service method: get the number of boards connected to the slow control server;
int ns__getNbBoards(uint32_t &nbBoards);

//gsoap ns service method: get the information for the board i
int ns__getBoardDescription(uint16_t board, ns2__returnBoard &rVal);

//gsoap ns service method: return firmware type
int ns__getFwVersion(uint16_t board, int16_t &fwRel);

//gsoap ns service method: start Starts the digitizer acquisition
int ns__start(uint16_t board, int &error);

//gsoap ns service method: stop Stops the digitizer acquisition
int ns__stop(uint16_t board, int &error);

//gsoap ns service method: clear Clears the data stored in the buffers of the Digitizer
int ns__clear(uint16_t board, int &error);

//gsoap ns service method: swtrg Sends a software trigger to the digitizer
int ns__swtrg(uint16_t board, int &error);

//gsoap ns service method: calibrate Calibrate the channels
int ns__calibrate(uint16_t board, int &error);

//gsoap ns service method: boardinfo  Returns the board info
int ns__boardinfo(uint16_t board, ns2__returnInfo &rVal);

//gsoap ns service method: channelinfo  Returns the channels info
int ns__channelinfo(uint16_t board, uint16_t channel, ns2__channelInfo &rVal);

//gsoap ns service method: channelinfo  Returns the channels info
int ns__channelpsdsettings(uint16_t board, uint16_t channel, ns2__psdInfo &rVal);

//gsoap ns service method: channelinfo  Returns the channels info
int ns__channelphasettings(uint16_t board, uint16_t channel, ns2__phaInfo &rVal);

//gsoap ns service method: generalinfo  Returns the general info
int ns__generalsettings(uint16_t board, ns2__generalInfo &rVal);

//gsoap ns service method: acqmode  Set acquisition mode
int ns__acqmode(uint16_t board, uint32_t mode, uint32_t save, int &error);

//gsoap ns service method: setmaxaggr Set maximum number of aggregate per block transfer
int ns__setmaxaggr(uint16_t board, uint32_t numAggr, int &error);

//gsoap ns service method: cfddelay  Set the CFD Delay
int ns__cfddelay(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: cfdfraction  Set the CFD Fraction
int ns__cfdfraction(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: cfdinterpolation  Set the CFD Interpolation
int ns__cfdinterpolation(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: cfddump  Dump the CFD Settings
int ns__cfddump(uint16_t board, uint16_t channel, ns2__cfdInfo &rval);

//gsoap ns service method: getNbChan  Get the maximum number of channels on the digitizer
int ns__getNbChan(uint16_t board, uint16_t &channel);

//gsoap ns service method: chenable Enable/disable a channel
int ns__chenable(uint16_t board, uint16_t channel, bool value, int &error);

//gsoap ns service method: isena Check status of a channel
int ns__isena(uint16_t board, uint16_t channel, bool &value);

//gsoap ns service method: enaettt Enable the extended time tag
int ns__enaettt(uint16_t board, int &error);

// DPP-PSD specific registers

//gsoap ns service method: tracelength Set the length of the traces
int ns__tracelength(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: inputrange Set the input range between 0.5 and 2 Vpp
int ns__inputrange(uint16_t board, uint16_t channel, bool value, int &error);

//gsoap ns service method: evtaggregate Set the number of evt per aggregate
int ns__evtaggregate(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: pretrigger Set the number of samples before the trigger in the saved waveform
int ns__pretrigger(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: forceflush Force the flush of the data in memory
int ns__forceflush(uint16_t board, uint16_t channel, int &error);

//gsoap ns service method: zerosupp Suppression of low energy signal using the long integration
int ns__zerosupp(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: shortgate Set the length of the short integration gate
int ns__shortgate(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: longgate Set the legnth of the long integration gate
int ns__longgate(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: gateoffset Number of samples before the trigger to start the integration
int ns__gateoffset(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: trgthres Trigger threshold
int ns__trgthres(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: fixbaseline Set fixed value of the basline
int ns__fixbaseline(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: shtrigwidth Set width of the logical local trigger
int ns__shtrgwidth(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: trgholdoff Inhibite for the overall Trigger Hold-Off duration
int ns__trgholdoff(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: cutpsdthres Sets the PSD threshold to online select events according to their PSD value.
int ns__cutpsdthres(uint16_t board, uint16_t channel, float value, int &error);

//gsoap ns service method: purthres Pile-up discrimination threshold
int ns__purthres(uint16_t board, uint16_t channel, uint16_t value, int &error);

//gsoap ns service method: chargesense Set charge sensitivity 
int ns__chargesense(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: chargepedestal Set charge pedestal
int ns__chargepedestal(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: trgcounting Propagation of rejected signal to the TRG-OUT
int ns__trgcounting(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: discritype  Set the discrimination to LE (0) or CFD (1)
int ns__discritype(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: purenable  Enable the pile-up within the gate
int ns__purenable(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: adcsigsource Enable the internal signal generation
int ns__adcsigsource(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: adctestrate Set the rate of the built-in pulse emulator
int ns__adctestrate(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: bslrecalc   Enable the baseline recalculation at the end of the long gate
int ns__bslrecalc(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: adcpolarity  Pulse polarity 0 - positive 1 - negative
int ns__adcpolarity(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: trgmode Set the trigger mode
int ns__trgmode(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: bslcalc Set the number of samples to be used for the bsl calc before the trigger
int ns__bslcalc(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: selftrigena  Enable disable the self trigger for the channel
int ns__selftrgena(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: chargereject Enable the zero suppression based on the charge integration
int ns__chargereject(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: purreject    Enable disable the PUR rejection
int ns__purreject(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: psdcutlow    PSD cut below threshold
int ns__psdcutlow(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: psdcutabv    PSD cut above threshold
int ns__psdcutabv(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: overrange    Reject events that are over the adc range 
int ns__overrange(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: trghysteresis Trigger inhibited furing the trailing edge of a pulse
int ns__trghysteresis(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: oppolarity   Detection of signals of opposite polarity to inhibit the zero crossing
int ns__opppolarity(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: localtrgmode Set the Local Shaped Trigger mode inside a couple
int ns__localtrgmode(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: enablelocaltrg Enable the Local Shaped Trigger
int ns__enablelocaltrg(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: localtrgval Set the Local trigger validation mode
int ns__localtrgval(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: enablelocalval Enable the Local trigger Validation
int ns__enablelocalval(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: confextra Configure the EXTRA word
int ns__confextra(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: usesmooth Enable the smoothing of the signal for charge integration
int ns__usesmooth(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: confsmooth Set the number of bits to be used for the smoothing
int ns__confsmooth(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: conftrgcr step for trigger counter rate
int ns__conftrgcr(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method:  confveto Select source of the veto 
int ns__confveto(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: continuusbsl Disable the baseline calculation before the start (PHA only)
int ns__continuusbsl(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: coincflag tag correlated/uncorrelated events (PHA only)
int ns__coincflag(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: enablesat Mark saturated signals
int ns__enablesat(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: localtrgopt Additional Local Trigger validation
int ns__localtrgopt(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: vetomode Veto signal operating mode
int ns__vetomode(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: Reset Time on External veto from TRG-IN
int ns__rststampveto(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: bsropt Baseline restorer optimization to avoid tail
int ns__bsropt(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: adcoffset set DC offset
int ns__adcoffset(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: indsofttrg Send software trigger to specific channel
int ns__indsofttrg(uint16_t board, uint16_t channel, int &error);

//gsoap ns service method: vetowidth Set value of the veto width
int ns__vetowidth(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: vetosteps Set steps of the veto width
int ns__vetosteps(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: bslfreeze  Set duration of the freeze time of the baseline before after the gate
int ns__bslfreeze(uint16_t board, uint16_t channel, uint32_t value, int &error);

//gsoap ns service method: autoflush Enable automatic data flush
int ns__autoflush(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: trigprop Enable the propagation of the individual trigger from MB individual trigger logic to the mezzanines
int ns__trgprop(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: dualtracemode Set the Dual trace mode
int ns__dualtracemode(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: analogtracemode Select the Analog trace to be sent
int ns__analogtracemode(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: analogtrace2mode Select the Analog trace to be sent
int ns__analogtrace2mode(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: stcena Enable the traces recording
int ns__stcena(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: extraena Enable the extra word
int ns__extraena(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: digprobe Digital probe 1 (PHA)
int ns__digprobe1PHA(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: digprobe Digital probe 1 (PSD)
int ns__digprobe1PSD(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: digprobe Digital probe 2
int ns__digprobe2(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: enadigprobe Enable the Digital probes
int ns__enadigprobe(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: aggrorg Aggregate organization
int ns__aggrorg(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: swtchon switch on all the channels after they have been switched off by the auto shutdown procedure
int ns__swtchon(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: acqstat Return the DAQ status
int ns__acqdump(uint16_t board, ns2__acqInfo &rVal);

//gsoap ns service method: glbtrginfo Return the Global Trigger info
int ns__glbtrginfo(uint16_t board, ns2__glbtrgInfo &rVal);

//gsoap ns service method: enablecouple Enable the triggering of specified couple
int ns__enablecouple(uint16_t board, uint32_t couple, uint32_t value, int &error);

//gsoap ns service method: coinctw set the coincidence time window
int ns__coinctw(uint16_t board, uint32_t value, int &error);

//gsoap ns service method: coinctw set the coincidence time window
int ns__coinclvl(uint16_t board, uint32_t value, int &error);


// DPP-PHA
//gsoap ns service method: finegain set the trapezoidal fine gain for the energy calculation
int ns__finegain(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: acqstart control the acquisition for individual channels
int ns__acqstart(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: rccr2smooth Defines the number of samples of a moving average filter used for the RC-CR2 signal formation
int ns__rccr2smooth(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: inprisetime time constant of the derivative component of the PHA fast discriminator filter 
int ns__inprisetime(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: trarisetime shaping time of the energy filter
int ns__trarisetime(uint16_t board, uint32_t channel, uint32_t value, int &error);

//gsoap ns service method: traflattop sets the trapezoidal flat top width
int ns__traflattop(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: trapeakingtime sets the region of the flat top to be used for the calculation of the peak height
int ns__trapeakingtime(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: tradecaytime set the decay time of the pre-amp
int ns__tradecaytime(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: risetimevalidation time window between the crossing of the RC-CR2 and the ne<xt pulse
int ns__risetimevalidation(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: peakholdoff minimal distance between two trapezoidal 
int ns__peakholdoff(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: trarescaling rescaling of the trapezoidal energy
int ns__trarescaling(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: decimation decimation factor
int ns__decimation(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: decimationgain decimation gain factor
int ns__decimationgain(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: peakmean number of sample for the averaging window of the trapezoid height calculation
int ns__peakmean(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: roolover enable rool over flag
int ns__rollover(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: puflag Enable Pile-up flag
int ns__puflag(uint16_t board, uint32_t value, uint32_t channel, int &error);

//gsoap ns service method: exttrgmode Set external trigger input mode
int ns__setExtTrgLevel(uint16_t board, int level, int &error);

//gsoap ns service method: getexttrgmode Get external trigger input mode
int ns__getExtTrgLevel(uint16_t board, int &level);
























































