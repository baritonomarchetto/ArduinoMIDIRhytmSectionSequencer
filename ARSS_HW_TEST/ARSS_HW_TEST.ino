/* 16 Steps MIDI Drum Sequencer
 * HARDWARE TEST SKETCH
 * - press a step button: the corresponding LED should turn on and a MIDI note on MIDI channel 10 will be sent to the output.
 * - turn a potentiometer: LEDS will light accordingly (first pot - first LED row, second pot - second LED row, and so on).
 * - if a MIDI note on is received, first LED will blink.
 * 
 * by Barito, 2019
 * (last update - 02/12/19)
*/

#include <MIDI.h>

#define LEDS_NUM 16
#define MIDI_CHANNEL 10     //MIDI OUT CHANNEL. Set at your will (1-16)
#define DISABLE_THRU

MIDI_CREATE_DEFAULT_INSTANCE();

//-------------------PINOUT-----------------------
const byte button1Pin = 2;
const byte button2Pin = 3;
const byte button3Pin = 4;
const byte button4Pin = 5;
const byte button5Pin = 6;

//STEPS BUTTONS, PIN 22-37
//STEPS LEDS, PIN 38-53

const byte pot1Pin = A0;
const byte pot2Pin = A1;
const byte pot3Pin = A2;
//------------------------------------------------

boolean button1State;
boolean button2State;
boolean button3State;
boolean button4State;
boolean button5State;

const int debounceTime = 50;
unsigned long dbTime;

byte drumNote[16] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86};

boolean buttonState[LEDS_NUM] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};                                           

void setup() {
pinMode(button1Pin, INPUT_PULLUP);
pinMode(button2Pin, INPUT_PULLUP);
pinMode(button3Pin, INPUT_PULLUP);
pinMode(button4Pin, INPUT_PULLUP);
pinMode(button5Pin, INPUT_PULLUP);

for (int i = 0; i < LEDS_NUM; i++){ //STEPS BUTTONS, PIN 22-37
  pinMode(i+22, INPUT_PULLUP);
  buttonState[i] = digitalRead[i+22];//INITIALIZE BUTTONS STATE
}

for (int j = 0; j < LEDS_NUM; j++){ //STEPS LEDS, PIN 38-53
  pinMode(j+38, OUTPUT);}

MIDI.setHandleNoteOn(Handle_Note_On);
MIDI.begin(MIDI_CHANNEL_OMNI);// start MIDI and LISTEN to ALL MIDI channels
#ifdef DISABLE_THRU
MIDI.turnThruOff();
#endif
}//setup close

void loop(){
MIDI.read();        //calls MIDI.handles
LED_Button_Test();
Button_Test();
POT_Test();
}

//if we receive a note on signal, blink LED #1
void Handle_Note_On(byte channel, byte pitch, byte velocity){
  blink();
}

/////////////////////////////////////
//if I turn a potentiometer, LEDS light up accordingly
void POT_Test(){
int val1 = analogRead(pot1Pin);
if(val1>1000) {
  digitalWrite(38, HIGH);
  digitalWrite(39, HIGH);
  digitalWrite(40, HIGH);
  digitalWrite(41, HIGH);}
else if(val1>800) {
  digitalWrite(38, HIGH);
  digitalWrite(39, HIGH);
  digitalWrite(40, HIGH);
  digitalWrite(41, LOW);}
else if(val1>450) {
  digitalWrite(38, HIGH);
  digitalWrite(39, HIGH);
  digitalWrite(40, LOW);
  digitalWrite(41, LOW);}
else if(val1>250) {
  digitalWrite(38, HIGH);
  digitalWrite(39, LOW);
  digitalWrite(40, LOW);
  digitalWrite(41, LOW);}
else if(val1>50) {
  digitalWrite(38, LOW);
  digitalWrite(39, LOW);
  digitalWrite(40, LOW);
  digitalWrite(41, LOW);}
int val2 = analogRead(pot2Pin);
if(val2>1000) {
  digitalWrite(42, HIGH);
  digitalWrite(43, HIGH);
  digitalWrite(44, HIGH);
  digitalWrite(45, HIGH);}
else if(val2>800) {
  digitalWrite(42, HIGH);
  digitalWrite(43, HIGH);
  digitalWrite(44, HIGH);
  digitalWrite(45, LOW);}
else if(val2>450) {
  digitalWrite(42, HIGH);
  digitalWrite(43, HIGH);
  digitalWrite(44, LOW);
  digitalWrite(45, LOW);}
else if(val2>250) {
  digitalWrite(42, HIGH);
  digitalWrite(43, LOW);
  digitalWrite(44, LOW);
  digitalWrite(45, LOW);}
else if(val2>50) {
  digitalWrite(42, LOW);
  digitalWrite(43, LOW);
  digitalWrite(44, LOW);
  digitalWrite(45, LOW);}
int val3 = analogRead(pot3Pin);
if(val3>1000) {
  digitalWrite(46, HIGH);
  digitalWrite(47, HIGH);
  digitalWrite(48, HIGH);
  digitalWrite(49, HIGH);}
else if(val3>800) {
  digitalWrite(46, HIGH);
  digitalWrite(47, HIGH);
  digitalWrite(48, HIGH);
  digitalWrite(49, LOW);}
else if(val3>450) {
  digitalWrite(46, HIGH);
  digitalWrite(47, HIGH);
  digitalWrite(48, LOW);
  digitalWrite(49, LOW);}
else if(val3>250) {
  digitalWrite(46, HIGH);
  digitalWrite(47, LOW);
  digitalWrite(48, LOW);
  digitalWrite(49, LOW);}
else if(val3>50) {
  digitalWrite(46, LOW);
  digitalWrite(47, LOW);
  digitalWrite(48, LOW);
  digitalWrite(49, LOW);}
}

/////////////////////////////////////
//if I press a step button, light the corrisponding LED and trig a MIDI note
void LED_Button_Test(){
  for(int i = 0; i < LEDS_NUM; i++){
    if(digitalRead(i+22) != buttonState[i] && millis() - dbTime > debounceTime){
      buttonState[i] = !buttonState[i];
      dbTime = millis();
          if(buttonState[i] == LOW){
              digitalWrite(i+38, HIGH); //TURN ON the corrispondng LED
              MIDI.sendNoteOn(drumNote[i], 100, MIDI_CHANNEL); //trigger the MIDI note
          }
          else {
              digitalWrite(i+38, LOW); //TURN OFF the corrispondng LED
              MIDI.sendNoteOn(drumNote[i], 0, MIDI_CHANNEL); //turn off the MIDI note
          }
    }
   }
}

//if I press a button, light the corresponding LEDS on the matrix (first 5 LEDS in series)
void Button_Test(){
if(digitalRead(button1Pin) != button1State && millis() - dbTime > debounceTime){
button1State = !button1State;
dbTime = millis();
    if(button1State == LOW){
        digitalWrite(38, HIGH); //TURN ON the first LED
    }
    else {
        digitalWrite(38, LOW); //TURN OFF the LED
    }
}
if(digitalRead(button2Pin) != button2State && millis() - dbTime > debounceTime){
button2State = !button2State;
dbTime = millis();
    if(button2State == LOW){
        digitalWrite(39, HIGH); //TURN ON the second LED
    }
    else {
        digitalWrite(39, LOW); //TURN OFF the LED
    }
}
if(digitalRead(button3Pin) != button3State && millis() - dbTime > debounceTime){
button3State = !button3State;
dbTime = millis();
    if(button3State == LOW){
        digitalWrite(40, HIGH); //TURN ON the third LED
    }
    else {
        digitalWrite(40, LOW); //TURN OFF the LED
    }
}
if(digitalRead(button4Pin) != button4State && millis() - dbTime > debounceTime){
button4State = !button4State;
dbTime = millis();
    if(button4State == LOW){
        digitalWrite(41, HIGH); //TURN ON the 4th LED
    }
    else {
        digitalWrite(41, LOW); //TURN OFF the LED
    }
}
if(digitalRead(button5Pin) != button5State && millis() - dbTime > debounceTime){
button5State = !button5State;
dbTime = millis();
    if(button5State == LOW){
        digitalWrite(42, HIGH); //TURN ON the 5th LED
    }
    else {
        digitalWrite(42, LOW); //TURN OFF the LED
    }
}
}

void blink(){
for(int i = 0; i < 3; i++){
  digitalWrite(38, HIGH);
  delay(5);
  digitalWrite(38, LOW);
  delay(5);
}
}
