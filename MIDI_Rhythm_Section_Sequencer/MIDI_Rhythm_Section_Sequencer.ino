/* Arduino 64 Steps MIDI Rhythm Section Sequencer.
 * Step-by-step and real time recording/playing.
 * 
 * - Select a drum by pressing steps from 1 to 12 while keeping pressed "shift" button.
 * - Lock to a bar by pressing steps from 13 to 16 while keeping pressed "shift" button.
 * - Let the sequence play over the entire 64 steps lenght by pressing the locked (lighted) bar step while keeping pressed "shift" button.
 * - copy and paste the locked bar by pressing "REC" while keeping pressed "shift" button.
 * - By pressing "roll" button, the currently active drum (see "step-by-step sequencing") will be played at each step (in a roll).
 * - By pressing any step button while keeping pressed "mute" button the drum associated to that step will be muted (or unmuted).
 * - By pressing 1st bar button (step 13) or 2nd bar button (step 14)while keeping "mute" pressed will mute or unmute all drums.
 * - clear the whole sequence by keeping presed the "start" button for more than 3 seconds.
 * - clear the CC/synth/drum sequence (in this order) by pressing the drum/channel button while keeping presed "REC"
 * - disble/enable MIDI echo by pressing "mute" button while keeping pressed "shift" button.
 * - "swing" your sequence by turning the "swing" potentiometer.
 * - drums 11 and 12 are used for arpeggio trig signals too. For V-Trig simply add a 1KOhm resistor in series with Arduino out. For S-Trig a  
 *   simple pnp transitor switch (i.e BC547 and a 1KOhm base resistor) is required.
 * - LIVE record incoming MIDI messages or your playing on the steps matrix by pressing "REC" button (only if bars are unlocked and sequence running).
 * - LIVE record incoming MIDI messages to the specific step by keeping pressed the destination step while receiving the message (only if the sequence is not playing - STOP).
 * Both MIDI clock input and output are implemented. In case no clock input is received, tempo is set with the dedicated potentiometer.
 * In case a MIDI clock input is received, tempo is computed from that and the pot will be unresponsive. MIDI clock is always sent to the MIDI out.
 * 
 * by barito
 * (last update - 31/07/2020)
*/

#include <MIDI.h>

#define STEPS_NUM 16         //number of STEP BUTTONS
#define DRUM_NUM 12          //number of drums/instruments
#define BAR_NUM 4            //number of bars/measures
#define POLYPHONY 5          //the first slot is reserved to drums. Then, the number of contemporary "bass" notes playable is this number "-1". 
#define SEND_INT_CLOCK       //"comment" or delete this line if you dont want internal clock to be sent out
#define EXT_CLOCK            //"comment" or delete this line if you dont want clock to be received
#define MIDI_CHANNEL 10      //MIDI out channel for drums. Set at your will (1-16)
#define DISABLE_THRU
//#define MIN_PITCH 21

MIDI_CREATE_DEFAULT_INSTANCE();
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI); //this should work in case of the use of arduno MEGA 2560 to free the default serial port for programming only.

//-------------------PINOUT-----------------------

const byte button1Pin = 2; //record or paste (2nd function)
const byte button2Pin = 3; //shift
const byte button3Pin = 4; //mute
const byte button4Pin = 5; //start
const byte button5Pin = 6; //roll

//STEPS BUTTONS, PIN 22-37
//STEPS LEDS, PIN 38-53

const byte recLEDPin = 7; // live recording LeD

const byte pot1Pin = A0; //accent/step volume
const byte pot2Pin = A1; //TEMPO
const byte pot3Pin = A2; //swing

const byte arp1OutPin = A3; //Arpeggio Clock out
const byte arp2OutPin = A4; //Arpeggio Clock out
//------------------------------------------------

boolean button1State;
boolean button2State;
boolean button3State;
boolean button4State;
boolean button5State;

byte drum;
byte bar;
byte Step;
bool START;
bool barHold;
bool liveRecording;
bool noNotesYet;
bool incomingClock;
bool trigReverse;
int pot1Val;
int pot2Val;
int pot3Val;
byte pot1ValVol;
byte swingShOp = 5;
byte volShOp = 3;
bool evenStep;
byte swingFactor;
byte dS;
byte delParam;
bool midiEcho = 1;//<<-- set to 0 to disable MIDI echo on startup. You can change this at any time pressing "mute" while keeping "shift".
unsigned long Time;
unsigned long stepLenght; //delay between steps
unsigned long clockLenght; // =1/6 stepLenght
unsigned long clockTime;
unsigned long swingLenght;
const int debounceTime = 50;
unsigned long dbTime;
unsigned long firstClockTick;
byte clockTick;
const unsigned long STEPLENGHTMIN = 60000;                    //determines MAX BPM (250)
const unsigned long STEPLENGHTMAX = STEPLENGHTMIN + 255000;   //determines MIN BPM (47 @ 60 MIN)

const byte drumNote[DRUM_NUM] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79}; //C4 - G5 (Nord Drum 3P default + next)

//byte stepDrumVolume[DRUM_NUM][STEPS_NUM][BAR_NUM];

boolean stepButtonState[STEPS_NUM] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

boolean muteState[DRUM_NUM];
                             
boolean activeStep[DRUM_NUM][STEPS_NUM][BAR_NUM];

byte bassPitch[DRUM_NUM][STEPS_NUM][BAR_NUM][POLYPHONY];

byte bassVel[DRUM_NUM][STEPS_NUM][BAR_NUM][POLYPHONY];

byte CCNum[DRUM_NUM][STEPS_NUM][BAR_NUM];

byte CCVal[DRUM_NUM][STEPS_NUM][BAR_NUM];

void setup() {
//INPUTS
pinMode(button1Pin, INPUT_PULLUP);
pinMode(button2Pin, INPUT_PULLUP);
pinMode(button3Pin, INPUT_PULLUP);
pinMode(button4Pin, INPUT_PULLUP);
pinMode(button5Pin, INPUT_PULLUP);
for (int i = 0; i < STEPS_NUM; i++){ //STEPS BUTTONS, PIN 22-37
  pinMode(i+22, INPUT_PULLUP);
  stepButtonState[i] = digitalRead[i+22];//INITIALIZE BUTTONS STATE
}
//OUTPUTS
pinMode(recLEDPin, OUTPUT);
for (int j = 0; j < STEPS_NUM; j++){ //STEPS LEDS, PIN 38-53
  pinMode(j+38, OUTPUT);
}
pinMode(arp1OutPin, OUTPUT);
pinMode(arp2OutPin, OUTPUT);

//MIDI HANDLING
//MIDI.setHandleStart(Handle_Start);
//MIDI.setHandleStop(Handle_Stop);
#ifdef EXT_CLOCK
MIDI.setHandleClock(Handle_Clock);
#endif
MIDI.setHandleNoteOn(Handle_Note_On);
MIDI.setHandleNoteOff(Handle_Note_Off);
MIDI.setHandleControlChange(Handle_CC);
MIDI.setHandlePitchBend(Handle_PB);
MIDI.setHandleAfterTouchChannel(Handle_AT);
MIDI.begin(MIDI_CHANNEL_OMNI);// start MIDI and listen to ALL MIDI channes
#ifdef DISABLE_THRU
MIDI.turnThruOff();
#endif
//INITIALIZATION
Initialize();
}//setup close

void loop(){
MIDI.read();        //calls MIDI.handles
#ifdef SEND_INT_CLOCK 
SendClock();
#endif
PotsHandling();
RESET();
SetStepLenght();
Buttons_Handling();
SetStep();
Sequencer();
ArpTrig();
}

////////////////////////////////////////
//INITIALIZE
void Initialize(){
button1State = digitalRead(button1Pin);
button2State = digitalRead(button2Pin);
button3State = digitalRead(button3Pin);
button4State = digitalRead(button4Pin);
button5State = digitalRead(button5Pin);
Full_Panic();
drum = 0;
bar = 0;
barHold = true;
liveRecording = false;
incomingClock = false;
digitalWrite(recLEDPin, LOW);
digitalWrite(arp1OutPin, LOW);
digitalWrite(arp2OutPin, LOW);
noNotesYet = true;
Step = STEPS_NUM;
START = false;
Time = 0;
for(int i = 0; i<DRUM_NUM; i++){
  muteState[i] = false;
  for(int j = 0; j<STEPS_NUM; j++){
    for(int k = 0; k<BAR_NUM; k++){
      activeStep[i][j][k] = false;
      //stepDrumVolume[i][j][k] = 0;
      CCNum[i][j][k] = 0;
      CCVal[i][j][k] = 0;
      for(int l = 0; l<POLYPHONY; l++){
        bassPitch[i][j][k][l] = 0;//synth/bass pitches initialization
        bassVel[i][j][k][l] = 0;//velocities initialization
      }
    }
  }
}
LED_Page_Update();
}


void ArpTrig(){
if(micros()-Time > 10000){ //10 ms. Increase if the synth arpeggio is not triggered (it happened to me that a freshly capped juno 6, working at 3 ms before recap, stopped working after recap and asked for 8 ms min).
  digitalWriteDirect(arp1OutPin, LOW); //V-trig
  digitalWriteDirect(arp2OutPin, LOW); //S-trig
  /*if(trigReverse){digitalWriteDirect(arpOutPin, HIGH);}//arpeggio clock, positive default state
  else{digitalWriteDirect(arpOutPin, LOW);}//arpeggio clock, negative default state*/
}
}

///////////////////////////////
//POTENTIOMETERS HANDLING
void PotsHandling(){
pot1Val = analogRead(pot1Pin);
//pot2Val = analogRead(pot2Pin);
pot3Val = analogRead(pot3Pin);
pot1ValVol = pot1Val>>volShOp;
swingFactor = pot3Val>>swingShOp;
}

///////////////////////////////
//FAST DIGITALWRITE/READ
inline void digitalWriteDirect(int pin, boolean val){
  if(val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
  else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
}
inline int digitalReadDirect(int pin){
  return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}

////////////////////////////////
//START - MIDI IN message
void Handle_Start(){
/*if(midiEcho){
   MIDI.sendRealTime(MIDI_NAMESPACE::Start);
}*/
START = true;
liveRecording = false;
digitalWriteDirect(recLEDPin, LOW);
bar = 0;
Step = 0;
}

////////////////////////////////
//STOP - MIDI IN message
void Handle_Stop(){
/*if(midiEcho){
   MIDI.sendRealTime(MIDI_NAMESPACE::Stop);
}*/
START = false;
bar = 0;
Step = 0;
Full_Panic();
}

/////////////////////////////////
//Pitch Bend
void Handle_PB(byte channel, int bend){
if(midiEcho){
  MIDI.sendPitchBend(bend, channel); //echo the message
}
}

/////////////////////////////////
//Control Change
void Handle_CC(byte channel, byte number, byte value){
if(midiEcho){
  MIDI.sendControlChange(number, value, channel); //echo the message
}
if(liveRecording){
  if(number >0 && channel <=DRUM_NUM){
    CCNum[channel-1][Step][bar] = number;
    CCVal[channel-1][Step][bar] = value;
  }
  }
}

/////////////////////////////////
//After Touch
void Handle_AT(byte channel, byte pressure){
if(midiEcho){
  MIDI.sendAfterTouch(pressure, channel); //echo the message
}
}

/////////////////////////////////
//CLOCK
void Handle_Clock(){
//using namespace midi;
if(incomingClock){
  MIDI.sendRealTime(MIDI_NAMESPACE::Clock);
}
//if(START == false){//COMPUTE TEMPO ONLY AT REST (TO REDUCE CPU LOAD)
clockTick++;
if (clockTick == 1){
  firstClockTick = micros();
}
else if (clockTick == 25){//25-1 = 24 = 1 beat
  clockTick = 0;
  //if(micros()-firstClockTick > STEPLENGHTMIN){
    //SET the steplenght from clock
  incomingClock = true;
  stepLenght = (micros()-firstClockTick)/4;
  //}
  //else {incomingClock = false;}
}
//}
}

///////////////////////////////
//NOTE ON
//receive every channel and transmit over MIDI_CHANNEL
void Handle_Note_On(byte channel, byte pitch, byte velocity){
if(midiEcho){
  MIDI.sendNoteOn(pitch, velocity, channel);//echo the message
}
/*else { //this helps to limit MIDI loop issues. Anohter is defined in MIDI.sendNoteOff() function
  if(channel <= DRUM_NUM && channel != MIDI_CHANNEL) {
    muteState[channel-1] = true;
  }
}*/
if(channel <= DRUM_NUM /*&& pitch >= MIN_PITCH*/){
  noNotesYet = false;
  if(START){
    if(liveRecording){
      if(micros()-Time > stepLenght/2){dS = 1;}
      else{dS = 0;}
      if(channel == MIDI_CHANNEL){ //RECORD INCOMING MIDI DRUMS
        for(int i = 0; i< DRUM_NUM; i++) {    
          if(pitch ==  drumNote[i]){
            if(Step < STEPS_NUM-1){
              activeStep[i][Step+dS][bar] = true;
              bassVel[i][Step+dS][bar][0] = velocity;
              //bassPitch[i][Step+dS][bar][0] = 0;//if zero, it's a drum
            }
            else{//if(Step = latest step)
              if(dS == 0){
                activeStep[i][STEPS_NUM-1][bar] = true;
                bassVel[i][STEPS_NUM-1][bar][0] = velocity;
                //bassPitch[i][STEPS_NUM-1][bar][0] = 0;//if zero, it's a drum
              }
              else /*if(dS == 1)*/{
                if(bar < BAR_NUM-1){
                  activeStep[i][0][bar+1] = true;
                  bassVel[i][0][bar+1][0] = velocity;
                  //bassPitch[i][0][bar+1][0] = 0;//if zero, it's a drum
                }
                else{
                  activeStep[i][0][0] = true;
                  bassVel[i][0][0][0] = velocity;
                  //bassPitch[i][0][0][0] = 0;//if zero, it's a drum
                }
              }
            }
          }
        }
      }
      else{//channel != MIDI_CHANNEL -> RECORD INCOMING MIDI PITCHES
        drum = channel-1;//this speeds up undoing the recording because places you on the correct drum -> channel
        muteState[drum] = 0;//unmute
        if(Step < STEPS_NUM-1){
          activeStep[channel-1][Step+dS][bar] = true;
          for(int m = 1; m< POLYPHONY; m++) {//m = 1 because the first slot is reserved for drums
            if(bassPitch[channel-1][Step+dS][bar][m] == 0){
              bassPitch[channel-1][Step+dS][bar][m] = pitch;
              bassVel[channel-1][Step+dS][bar][m] = velocity;
              return;
            }
          }
        }
        else{//if(Step = latest step)
          if(dS == 0){
            activeStep[channel-1][STEPS_NUM-1][bar] = true;
            for(int m = 1; m< POLYPHONY; m++) {//m = 1 because the first slot is reserved for drums
              if(bassPitch[channel-1][STEPS_NUM-1][bar][m] == 0){
                bassPitch[channel-1][STEPS_NUM-1][bar][m] = pitch;
                bassVel[channel-1][STEPS_NUM-1][bar][m] = velocity;
                return;
              }
            }
          }
          else /*if(dS == 1)*/{
            if(bar < BAR_NUM-1){
              activeStep[channel-1][0][bar+1] = true;
              for(int m = 1; m< POLYPHONY; m++) {//m = 1 because the first slot is reserved for drums
                if(bassPitch[channel-1][0][bar+1][m] == 0){
                  bassPitch[channel-1][0][bar+1][m] = pitch;
                  bassVel[channel-1][0][bar+1][m] = velocity;
                  return;
                }
              }
             }
            else{ //bar == BAR_NUM
              activeStep[channel-1][0][0] = true;
                for(int m = 1; m< POLYPHONY; m++) {//m = 1 because the first slot is reserved for drums
                  if(bassPitch[channel-1][0][0][m] == 0){
                    bassPitch[channel-1][0][0][m] = pitch;
                    bassVel[channel-1][0][0][m] = velocity;
                    return;
                  }
                }
             }
           }
         }
       }
     }
  }
  else{//START == false, record incoming MIDI pitch to the specific step (kept pressed)
    //if(barHold == true && drum == channel-1){
    for(int j = 0; j< STEPS_NUM; j++) {
      if(stepButtonState[j] == LOW){ //WHILE KEEPING THE DESTINATION STEP BUTTON PRESSED
        digitalWriteDirect(j+38, HIGH);//lit the hold LED
        //deactivate the "drum", activated or deactivated by SetStep() as soon as step button is pressed (this makes intentionally recorded drums gone lost. It would require a rewrite of the drum assign routine to work, with button release activation... but I prefer this hack)      
        activeStep[drum][j][bar] = false; 
        bassVel[drum][j][bar][0] = 0;
        //DRUMS
        if(channel == MIDI_CHANNEL){ //INCOMING MIDI DRUMS
          for(int i = 0; i< DRUM_NUM; i++) {    
            if(pitch == drumNote[i]){
              activeStep[i][j][bar] = true;
              bassVel[i][j][bar][0] = velocity;
              //bassPitch[i][j][bar][0] = 0;
            }
          }
        }
        //SYNTHS
        else{//for all channels execpt MIDI_CHANNEL (and channel <= DRUM_NUM)    
          //...set the incoming message to the right MIDI channel. This avoid the need for a preventive selection of the right drum (channel).
          activeStep[channel-1][j][bar] = true; 
          muteState[channel-1] = 0;//unmute
          if(bassPitch[channel-1][j][bar][POLYPHONY-1] > 0){//all slot full...
          //...delete all recorded messages on this step...
            for(int m = 1; m< POLYPHONY; m++) {
              bassPitch[channel-1][j][bar][m] = 0;
              bassVel[channel-1][j][bar][m] = 0;
            }
          }
          for(int r = 1; r< POLYPHONY; r++) {
            if(bassPitch[channel-1][j][bar][r] == 0){
              bassPitch[channel-1][j][bar][r] = pitch;
              bassVel[channel-1][j][bar][r] = velocity;
              return;
            }
          }
        }
      }
    }
    //}
  }
}
}

///////////////////////////////
//NOTE OFF
//NECESSARY FOR PITCHES
//There's no need to replicate note-on code and keep track of pitched sounds because pitched notes are turned off every next step.
void Handle_Note_Off(byte channel, byte pitch, byte velocity){
if(midiEcho){
  MIDI.sendNoteOn(pitch, 0, channel);//echo the message 
  //MIDI.sendNoteOff(pitch, 0, channel);//echo the message
}
/*else {//this helps to limit MIDI loop issues (see Handle_Note_On()).
  if(channel <= DRUM_NUM && channel != MIDI_CHANNEL) {
    muteState[channel-1] = false;
  }
}*/
}

/////////////////////////////////
//Send internal clock if no external clock incoming
void SendClock(){
if(incomingClock == false){
  if(micros()-clockTime >= clockLenght){
    clockTime = micros();
    MIDI.sendRealTime(MIDI_NAMESPACE::Clock);
  }
}
}

///////////////////////////////////
//define which step is active (whithin the instrument/drum), and lit the LED accordingly
//define which instrument is active
//define which instrument is muted
//set the drum volume on that step
//plays the drum if bar is running
//record the drum if live recording is enabled
void SetStep(){
for(int i = 0; i < STEPS_NUM; i++){
  if(digitalRead(i+22) != stepButtonState[i] && millis() - dbTime > debounceTime){
    stepButtonState[i] = !stepButtonState[i];
    dbTime = millis();
    if(stepButtonState[i] == LOW){
      if(button1State == HIGH){//no REC
        if(button2State == HIGH){//no SHIFT
            if(button3State == HIGH){// no MUTE            
              if(barHold == true){
                if(noNotesYet == true){noNotesYet = false;}
                activeStep[drum][i][bar] = !activeStep[drum][i][bar]; //activate/deactivate the step for the current drum
                if(activeStep[drum][i][bar] == true){
                  bassVel[drum][i][bar][0] = pot1ValVol;
                  //bassPitch[drum][i][bar][0] = 0;//if zero, it's a drum
                }
                else {//step not active
                  bassVel[drum][i][bar][0] = 0;
                  //bassPitch[drum][i][bar][0] = 0;
                }
                digitalWriteDirect(i+38, activeStep[drum][i][bar]);//lit the (stationary) LED if the step is active
              }
              else {//if barHold false
                //LIVE PLAY & RECORDING
                if(i < DRUM_NUM){ //to avoid bar steps...
                  //LIVE PLAY
                  MIDI.sendNoteOn(drumNote[i], pot1ValVol, MIDI_CHANNEL); //play the drum                  
                  //LIVE RECORDING
                  if(liveRecording){ //record the drum
                    if(noNotesYet){
                      noNotesYet = false;
                      Step = 0;
                      bar = 0;
                    }
                    if(micros()-Time > stepLenght/2){dS = 1;}
                    else{dS = 0;}
                    if(Step < STEPS_NUM-1){
                      activeStep[i][Step+dS][bar] = true;
                      bassVel[i][Step+dS][bar][0] = pot1ValVol;
                      //bassPitch[i][Step+dS][bar][0] = 0;//if zero, it's a drum
                    }
                    else{
                      if(dS == 0){
                        activeStep[i][STEPS_NUM-1][bar] = true;
                        bassVel[i][STEPS_NUM-1][bar][0] = pot1ValVol;
                        //bassPitch[i][STEPS_NUM-1][bar][0] = 0;//if zero, it's a drum
                      }
                      else /*if(dS == 1)*/{
                        if(bar < BAR_NUM-1){
                          activeStep[i][0][bar+1] = true;
                          bassVel[i][0][bar+1][0] = pot1ValVol;
                          //bassPitch[i][0][bar+1][0] = 0;//if zero, it's a drum
                        }
                        else{
                          activeStep[i][0][0] = true;
                          bassVel[i][0][0][0] = pot1ValVol;
                          //bassPitch[i][0][0][0] = 0;//if zero, it's a drum
                        }
                      }
                    }
                  }                  
                }
              }
            }
            else{ //if(button3State == LOW){//mute
              if(i < DRUM_NUM){ //MUTE THE SPECIFIC DRUM ONLY
                  muteState[i] = !muteState[i];
              }
              else if(i == 12){ //MUTE ALL DRUMS
                for(int x = 0; x < DRUM_NUM; x++){
                  muteState[x] = true;
                }
              }
              else if(i == 13){ //UNMUTE ALL DRUMS
                for(int x = 0; x < DRUM_NUM; x++){
                  muteState[x] = false;
                }
              }
            }
        }
        else{ //if(button2State == LOW) {//shift
          if(i < DRUM_NUM){
            digitalWriteDirect(drum+38, LOW); //turn off previous drum LED
            drum = i;
            digitalWriteDirect(drum+38, HIGH); //turn on actual drum LED
          }
          else{ //if(i >= DRUM_NUM){
            if(barHold == false){
              digitalWriteDirect(bar+50, LOW); //turn off previous bar LED
              bar = i-DRUM_NUM;
              barHold = true;
              digitalWriteDirect(bar+50, HIGH); //turn on actual bar LED
            }
            else{//if barHold == true
                if(i-DRUM_NUM == bar){
                  barHold = false;
                  digitalWriteDirect(bar+50, LOW); //turn off actual bar LED;
                }
                else {
                   digitalWriteDirect(bar+50, LOW); //turn off prev bar LED;
                   bar = i-DRUM_NUM;
                   digitalWriteDirect(bar+50, HIGH); //turn on actual bar LED;
                }
             }
          }
        }
      }
      else{//if REC is being held pressed, delete the step drum/channel sequence
        liveRecording = false;
        digitalWrite(recLEDPin, LOW);
        drum = i;
        delParam = 0;
        //if a CC value is defined for this step, delete all CC's (not pitches, nor drums)
        for(int j = 0; j<STEPS_NUM; j++){
          for(int k = 0; k<BAR_NUM; k++){
            if(CCVal[drum][j][k] > 0){
              delParam = 1;     
            }
          }
        }
        if(delParam == 0){
        //else, if a pitch is recorded, delete all pitches (not drums)
        for(int j = 0; j<STEPS_NUM; j++){
          for(int k = 0; k<BAR_NUM; k++){
            if(bassPitch[drum][j][k][1] > 0){ //[1] is a [one], not a [lower case L]
              delParam = 2;     
            }
          }
        }
        }
        switch (delParam){
          case 1: //CCs
          for(int j = 0; j<STEPS_NUM; j++){
            for(int k = 0; k<BAR_NUM; k++){
              CCNum[drum][j][k] = 0;//CCs initialization
              CCVal[drum][j][k] = 0;//CCs initialization
            }
          }
          break;
          case 2: //pitch
          for(int j = 0; j<STEPS_NUM; j++){
              for(int k = 0; k<BAR_NUM; k++){
                for(int m = 1; m<POLYPHONY; m++){
                  bassPitch[drum][j][k][m] = 0;//pitches initialization
                  bassVel[drum][j][k][m] = 0;//bass/synth velocity initialization
                  if(bassVel[drum][j][k][0] == 0){//no drums underneath...
                    activeStep[drum][j][k] = false;
                  }
                }
              }
           }
           break;
           default: //drum
           for(int j = 0; j<STEPS_NUM; j++){
              for(int k = 0; k<BAR_NUM; k++){
                bassVel[drum][j][k][0] = 0;//drums velocity initialization
                activeStep[drum][j][k] = false;
              }
           }
        }
        Drum_Panic();
        muteState[drum] = 0;
      }
    }
  }
}
}

////////////////////////////////////
//sequencing
void Sequencer(){
if(START == true){
  if(Step%2 == 0){evenStep = true; swingLenght = (stepLenght/(1023>>swingShOp))*swingFactor;}
  else{evenStep = false; swingLenght = 0;}
  if(micros()-Time >= stepLenght+swingLenght){
    if(evenStep == false) {Time = micros();}
    else {Time = micros() - swingLenght;}
    if(button2State == HIGH) {//SHIFT not pressed
      digitalWriteDirect(Step+38, activeStep[drum][Step][bar]);//UNlit (or lit if it was active) the (running) LED, old step
    }
    //KILL all the (previous are going to be soon) synth/bass pitches.
    for (int i = 0; i < DRUM_NUM; i++) {//CHANNELS
      for (int j = 1; j < POLYPHONY; j++) {//POLYPHONY
        //if(bassPitch[i][Step][bar][j] > 0) {//synth, not drum
          MIDI.sendNoteOn(bassPitch[i][Step][bar][j], 0,i+1); //velocity = 0 => note off
        //}
      }
    }
    Step++;
    if(Step >= STEPS_NUM){
      Step = 0;
      //BAR LEDS HANDLING
      if(barHold == false){
          digitalWriteDirect(bar+50, LOW);//turn off the old bar
          bar++;
          if(bar>=BAR_NUM){
            bar = 0;
            //MIDI.sendRealTime(MIDI_NAMESPACE::Start); //this could be used to synch with my MIDI looper
          }
          LED_Page_Update();
          digitalWriteDirect(bar+50, HIGH);//turn on the new bar
      }
    }
    //PLAY ACTIVE STEPS && CCs
    for (int j = 0; j < DRUM_NUM; j++) {
       if(activeStep[j][Step][bar] == true && muteState[j] == false) {
          //DRUMS
          MIDI.sendNoteOn(drumNote[j], bassVel[j][Step][bar][0], MIDI_CHANNEL);
          //BASS/SYNTH
          for(int n = 1; n<POLYPHONY; n++){
             if(bassPitch[j][Step][bar][n] > 0){
                MIDI.sendNoteOn(bassPitch[j][Step][bar][n], bassVel[j][Step][bar][n], j+1);//play all pitches on the step, channels
             }
           }
           if(CCNum[j][Step][bar]>0) {MIDI.sendControlChange(CCNum[j][Step][bar], CCVal[j][Step][bar], j+1);}//play CCs
       }
    }
    //ARPEGGIATE - TRIG
    if(activeStep[DRUM_NUM-2][Step][bar] == true && muteState[DRUM_NUM-2] == false) {//ARPEGGIATOR 1, drum 11
      digitalWriteDirect(arp1OutPin, HIGH);
    }
    if(activeStep[DRUM_NUM-1][Step][bar] == true && muteState[DRUM_NUM-1] == false) {//ARPEGGIATOR 2,drum 12
      digitalWriteDirect(arp2OutPin, HIGH);
      /*if(trigReverse){digitalWriteDirect(arpOutPin, LOW);}//arpeggio clock, negative edge (reverse)
      else{digitalWriteDirect(arpOutPin, HIGH);}//arpeggio clock, positive edge*/
    }
    //ROLL (only drums on MIDI_CHANNEL)
    if(button5State == LOW){
      //if(bassVel[drum][Step][bar][0]>= pot1ValVol){
      //  MIDI.sendNoteOn(drumNote[drum], bassVel[drum][Step][bar][0], MIDI_CHANNEL);
      //}
      //else {
        MIDI.sendNoteOn(drumNote[drum], pot1ValVol, MIDI_CHANNEL);
      //}
    }
    if(button2State == HIGH) {
      digitalWriteDirect(Step+38, !activeStep[drum][Step][bar]);//lit (or unlit if it was active!!) the (running) LED
    }
  }
}
}

/////////////////////////////////////
//SHIFT - MUTE - RECORD - ROLL - START
void Buttons_Handling(){
////////RECORD - PASTE///////////////
if(digitalReadDirect(button1Pin) != button1State && millis() - dbTime > debounceTime){
  button1State = !button1State; //REC
  dbTime = millis();
  if(button1State == LOW){//REC
    if(button2State == LOW){ //SHIFT
      CopyPasteBar();
      barHold = false;
    }
    else{//if SHIFT is high
      if(barHold == false && START == true){ //LIVE RECORDING MODE
        liveRecording = !liveRecording;
        digitalWriteDirect(recLEDPin, liveRecording);
      }
    }
  }
}
///////////////SHIFT///////////////////////
if(digitalReadDirect(button2Pin) != button2State && millis() - dbTime > debounceTime){
  button2State = !button2State; //SHIFT
  dbTime = millis();
    if(button2State == LOW){  //SHIFT
      if(liveRecording) {liveRecording = false; digitalWriteDirect(recLEDPin, LOW);}
      for(int i = 0; i< STEPS_NUM; i++) {
        digitalWriteDirect(i+38, LOW); //turn OFF all LEDs, 
        digitalWriteDirect(drum+38, HIGH); //turn ON actual drum LED
        if(barHold == true) {digitalWriteDirect(bar+50, HIGH);} //turn ON actual bar LED if bar is HOLDED
      }
    }
    //else /*if(shiftButtonState == HIGH)*/{
      LED_Page_Update(); //UPDATE LEDS STATUS to reflect step state
  //}
}
/////////////////MUTE//////////////////////
if(digitalReadDirect(button3Pin) != button3State && millis() - dbTime > debounceTime){
  button3State = !button3State; //MUTE
  dbTime = millis();
  if(button3State == LOW){ //MUTE
    if(button2State == LOW){ //SHIFT
      midiEcho = !midiEcho;
    }
  } 
}
//////////////START-STOP//////////////////
if(digitalReadDirect(button4Pin) != button4State && millis() - dbTime > debounceTime){//START - STOP
   button4State = !button4State;
   dbTime = millis();
   if(button4State == LOW){
      START = !START; 
      if(START == false){//STOP
        //digitalWrite(arp2OutPin, HIGH); //UNEFFECTIVE
        Full_Panic();
        digitalWriteDirect(Step+38, activeStep[drum][Step][bar]);//update the running LED
        digitalWriteDirect(bar+50, LOW);//TURN OFF bar LED
        Step = 0;
        liveRecording = false;
        digitalWriteDirect(recLEDPin, LOW);
        if(barHold == false) {bar = 0;}
        LED_Page_Update();
      }
      else{ //if START
        Step = STEPS_NUM;
        if(barHold == false) {bar = BAR_NUM;}
      }
   }
}
///////////////ROLL///////////////////
if(digitalReadDirect(button5Pin) != button5State && millis() - dbTime > debounceTime){
  button5State = !button5State; //ROLL
  dbTime = millis();
}
}

/////////////////////////////////////
//UPDATE LEDS
void LED_Page_Update(){
if(button2State == HIGH) {//shift
  for(int i = 0; i< STEPS_NUM; i++) {
     digitalWriteDirect(i+38, activeStep[drum][i][bar]);} //UPDATE LEDS STATUS to reflect step state
  }
}

/////////////////////////////////////
//FULL BLINK STEP LEDS
void LED_FULL_BLINK(){
for(int j = 0; j< 5; j++) {
  for(int i = 0; i< STEPS_NUM; i++) {
     digitalWriteDirect(i+38, HIGH);}
  delay(100);
  for(int i = 0; i< STEPS_NUM; i++) {
     digitalWriteDirect(i+38, LOW);}
  delay(200);
}
}

/////////////////////////////////
//FULL RESET if button is kept pressed for more than 3 seconds
void RESET(){
if(button4State == LOW && millis()-dbTime > 3000){ //START
  LED_FULL_BLINK();
  Initialize();}
}

/////////////////////////////////////
// Read the BPM knob, compute the step lenght
void SetStepLenght(){
if(incomingClock == false){
  stepLenght = STEPLENGHTMIN + ((1023 - analogRead(pot2Pin))>>2)*1000; //STEPLENGHTMIN to 255 + STEPLENGHTMIN ms (48 BPM - 250 BPM)
  clockLenght = stepLenght/6;
}
}

///////////////////////////////////
//Copy and Paste current bar MIDI events over all bars
void CopyPasteBar(){
for(int i = 0; i< DRUM_NUM; i++) {
  for(int j = 0; j< STEPS_NUM; j++){
    for(int k = 0; k< BAR_NUM; k++) {  
      activeStep[i][j][k] = activeStep[i][j][bar];
      for(int l = 0; l< POLYPHONY; l++) { 
        bassPitch[i][j][k][l] = bassPitch[i][j][bar][l];
        bassVel[i][j][k][l] = bassVel[i][j][bar][l];
      }
    }
  }
}
}

void Full_Panic(){
for (int j = 0; j < DRUM_NUM; j++) {//the MIDI channel in this case...
  for (int i = 0; i < 127; i++) {//all possible MIDI notes
    MIDI.sendNoteOn(i, 0, j+1); //turn off note (method 1)
    //MIDI.sendNoteOff(i, 0, j+1); //turn off note (method 2)
  }
}
}

void Drum_Panic(){
for (int i = 0; i < 127; i++) {
  MIDI.sendNoteOn(i, 0, drum); //turn off note (method 1)
  //MIDI.sendNoteOff(i, 0, drum); //turn off note (method 2)
}
}
