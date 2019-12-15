#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/triangle2048_int8.h> // sine table for oscillator
#include <tables/saw2048_int8.h> // sine table for oscillator
#include <tables/square_no_alias_2048_int8.h> // sine table for oscillator
#include <mozzi_midi.h>
#include <ADSR.h>
#include <EventDelay.h>
#include <Ead.h> // exponential attack decay
#include <LowPassFilter.h>

#define MODE_PIN 4
#define PARAM_PIN 5
#define POT1_PIN 1
#define POT2_PIN 2
#define POT3_PIN 3

#define MODE_PHRASE 0
#define MODE_SONG 2
#define MODE_SYNTH 1

const PROGMEM float midi2frequ[] = {0.00, 8.66, 9.18, 9.72, 10.30, 10.91, 11.56, 12.25, 12.98, 13.75, 14.57, 15.43, 16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 29.14, 30.87, 32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74, 65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01, 4434.92, 4698.63, 4978.03, 5274.04, 5587.65, 5919.91, 6271.92, 6644.87, 7040.00, 7458.61, 7902.13, 8372.01, 8869.84, 9397.27, 9956.06, 10548.08, 11175.31, 11839.82};
// notes to frequency lookup table, notes follow MIDI definition
const byte phrase[8][8] = {
  {60, 62, 64, 65, 67, 69, 71, 72}, // Dur
  {64, 66, 67, 71, 72, 76, 78, 79}, // Hapi drum: Origin E Akebono Pentatonic
  {60, 67, 60, 65, 60, 65, 60, 67}, // phrase A
  {60, 64, 60, 64, 60, 67, 60, 67}, // phrase B
  {60, 64, 67, 72, 65, 65, 69, 72}, // phrase C
  {60, 62, 60, 65, 60, 69, 60, 72}, // phrase D
  {60, 64, 67, 67, 62, 65, 69, 69}, // phrase E
  {65, 60, 71, 70, 69, 65, 62, 60}, // phrase F
};
const byte song[4][16] = {
  {50, 66, 69, 74, 57, 64, 69, 73, 59, 62, 66, 71, 53, 61, 66, 69}, // Kanon, Pachelbel
  {67, 67, 74, 74, 76, 76, 74, 74, 72, 72, 71, 71, 69, 69, 67, 67}, // Morgen kommt der Weihnchtsmann
  {60, 60, 62, 60, 65, 64, 64, 64, 60, 60, 62, 60, 67, 65, 65, 65}, // Happy Birthday
  {60, 62, 64, 65, 67, 69, 71, 72, 60, 62, 64, 65, 67, 69, 71, 72}, // test
};
// MIDI note 48 = c (kl. Oktave); 60 = Schlüssel-C (12/Oktave) - CAVE: +/-32 wegen TUNE zu beachten!
int prevMode, prevParam, prevPot1, prevPot2, prevPot3; //to store potentiometer setting & detect changes
bool chgMode, chgParam, chgPot1, chgPot2, chgPot3; //flag if potentiometer setting was changed

int phraseSpeed = 600; //note lenght in ms
int phraseTune = 0; //offset in MIDI notes, for phrase&song
byte phraseType = 2; //# of phrase or song
bool phraseDir = true; //true=forward, false=backwards
bool phraseMode = true; //false=song
byte phrasePos = 0; //which note to play in phrase, for phrase&song

byte oscWave = 1; // 0=sine, 1=triangle, 2=saw, 3=square
byte currPot = 0; // which potentiometer to read, as reading all at one takes up too much time
byte oscLfo = 0; // strength of LFO modulation
byte oscEnv = 0; // strength of env modulation
byte lfoWave = 0; // 0=sine, 1=triangle, 2=saw, 3=square
float lfoFreq = 10.0;
int envGain, envAttack = 64, envDelay = 768; // envelope parameters
int lpfCutoff = 255, lpfResonance = 0;

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> lfo(SIN2048_DATA);
//ADSR <CONTROL_RATE, AUDIO_RATE> envelope;
Ead envelope(CONTROL_RATE); // resolution will be CONTROL_RATE
LowPassFilter lpf;

EventDelay timer;

#define DEBUG //this will send paramater changes to serial monitor

void setup() {
#ifdef DEBUG
  Serial.begin (115200); // serial for debugging
#endif
  /* for (int i = 0; i < 127; i++) {
     midi2frequ [i] = mtof(float(i)); //pre-calcuate frequency table
    }*/
  startMozzi(); // start engine
  adcDisconnectAllDigitalIns();
  // envelope.setLevels(255, 200, 100, 0);
  // envelope.setTimes(400, 100, 500, 100);
  lfo.setFreq (lfoFreq);
  lpf.setCutoffFreq(lpfCutoff);
  lpf.setResonance(lpfResonance);
  timer.start(100);
}

// MODE: phrase / song / synth
// PARAM/phrase: phrase
// PARAM/song: song
// PARAM/synth: type, osc, filter, lfo, ADSR time, ADSR level
// POT/phrase: 1-speed, 2-transpose
// POT/song: 1-speed, 2-transpose
// POT/synth/type: additive (more to be implemented)
// POT/synth/osc: 1-waveform, 2-tune, 3-mod
// POT/synth/adsr: 1-attack, 2-decay, 3-sustain (no release)
// POT/synth/filter: 1-cutoff, 2-resonance, 3-mod
// POT/synth/lfo: 1-waveform, 2-tune


void updateControl() {
  readPotis();
  if (chgMode)  // mode changed, only changes internal state, no action
  {
    if (prevMode == MODE_PHRASE) {
      phraseMode = true;
#ifdef DEBUG
      Serial.println("mode:PHRASE");
#endif
    }
    else if (prevMode == MODE_SONG) {
      phraseMode = false;
#ifdef DEBUG
      Serial.println("mode:SONG");
#endif
    }
    else { /* if (prevMode == MODE_SYNTH)*/  // 3+4=synth
#ifdef DEBUG
      Serial.println("mode:SYNTH");
#endif
    }
    chgMode = false;
  }
  if (chgParam) //param changed, only changes internal state, no action
  {
#ifdef DEBUG
    Serial.print("param"); Serial.println(prevParam);
#endif
  }
  if (chgPot1 || chgPot2 || chgPot3) {
    if (prevMode == MODE_PHRASE) {
      doPhrase();
    }
    else if (prevMode == MODE_SONG) {
      doSong();
    }
    else if (prevMode == MODE_SYNTH) {
      doSynth();
    }
    chgPot1 = chgPot2 = chgPot3 = false;
  }


  if (timer.ready()) { //timer for next note has triggered

    if (phraseMode) { //play phrase
      if (phraseDir) { //forward
        phrasePos++;
        if (phrasePos > 7) phrasePos = 0;
      }
      else //backward
      {
        phrasePos--;
        if (phrasePos < 0) phrasePos = 7;
      }
      osc.setFreq(pgm_read_float_near( midi2frequ + phrase[phraseType][phrasePos] + phraseTune ));
      //      osc.setFreq(midi2frequ[phrase[phraseType][phrasePos] + phraseTune]);

    }
    else { // play song
      phrasePos++;
      if (phrasePos > 15) phrasePos = 0;
      osc.setFreq(pgm_read_float_near( midi2frequ + song[phraseType][phrasePos] + phraseTune ));
      //      osc.setFreq(midi2frequ[song[phraseType][phrasePos] + phraseTune]);
    }

    envelope.start(envAttack, envDelay);
    // envelope.noteOn();
    timer.start(phraseSpeed); //start next note timer
  }
  envGain = (int) envelope.next();
}

int updateAudio() {
  Q15n16 vibrato = (Q15n16) oscLfo * lfo.next();
  //return (envGain * osc.phMod(vibrato)) >> 8; // phase modulation to modulate frequency & gain for envelope
  return lpf.next((((envGain | oscEnv)) * osc.phMod(vibrato)) >> 8); // phase modulation to modulate frequency & gain+depth for envelope
  //    return osc.phMod(vibrato); //only lfo
  //return osc.next();//only osc

}


void loop() {
  audioHook(); // required here
}

void doPhrase() {
  if (chgPot1) { //speed&direction
    if (prevPot1 < 32) //backwards
    {
      phraseDir = false;
      phraseSpeed = (prevPot1 << 5) + 31; // scale to 31ms - 1050ms
    }
    else //forward
    {
      phraseDir = true;
      phraseSpeed = 2048 - (prevPot1 << 5) + 63;
    }
#ifdef DEBUG
    Serial.print ("phrase-speed:"); Serial.println (phraseSpeed);
#endif
    chgPot1 = false;
  }
  if (chgPot2) { // tune
    phraseTune = prevPot2 - 31;
#ifdef DEBUG
    Serial.print ("phrase-tune:"); Serial.println (phraseTune);
#endif
    chgPot2 = false;
  }
  if (chgParam) { // select phrase
    phraseType = prevParam;
    chgParam = false;
#ifdef DEBUG
    Serial.print("phrase:"); Serial.println (phraseType);
#endif
  }
}

void doSong() {
  if (chgPot1) { //speed
    phraseDir = true; //songs only forward
    phraseSpeed = 1024 - (prevPot1 << 4) + 31;
#ifdef DEBUG
    Serial.print ("song-speed:"); Serial.println (phraseSpeed);
#endif
    chgPot1 = false;
  }
  if (chgPot2) { // tune with limited range
    phraseTune = (prevPot2 >> 1) - 15;
#ifdef DEBUG
    Serial.print ("song-tune:"); Serial.println (phraseTune);
#endif
    chgPot2 = false;
  }
  if (chgParam) { // select song
    phraseType = prevParam >> 1;
    chgParam = false;
#ifdef DEBUG
    Serial.print ("song:"); Serial.println (phraseType);
#endif
  }
}

void doSynth() {
  if (prevParam == 0) { // PARAM-osc
    if (chgPot1) { //waveform
      oscWave = prevPot1 >> 4;
      switch (oscWave) {
        case 0:
          osc.setTable(SIN2048_DATA);
#ifdef DEBUG
          Serial.println("osc-sin");
#endif
          break;
        case 1:
          osc.setTable(TRIANGLE2048_DATA);
#ifdef DEBUG
          Serial.println("osc-tri");
#endif
          break;
        case 2:
          osc.setTable(SAW2048_DATA);
#ifdef DEBUG
          Serial.println("osc-saw");
#endif
          break;
        case 3:
          osc.setTable(SQUARE_NO_ALIAS_2048_DATA);
#ifdef DEBUG
          Serial.println("osc-sqr");
#endif
          break;
      }
      chgPot1 = false;
    }
    if (chgPot2) { //mod-lfo
      oscLfo  = prevPot2 << 1; //0-127
#ifdef DEBUG
      Serial.print("osc-lfo:");          Serial.println(oscLfo);
#endif
      chgPot2 = false;
    }
    if (chgPot3) { //mod-env
      oscEnv = 255 - (prevPot3 << 2); //255-0
#ifdef DEBUG
      Serial.print("osc-env:");          Serial.println(oscEnv);
#endif
      chgPot3 = false;
    }
  }
  else if (prevParam == 1) { //PARAM-env
    if (chgPot1) { //attack
      envAttack = prevPot1 << 1;
#ifdef DEBUG
      Serial.print("attack:");          Serial.println(envAttack);
#endif
      chgPot1 = false;
    }
    if (chgPot2) { //decay
      envDelay = prevPot2 << 5; // idea: set realtive to speed of phrase
#ifdef DEBUG
      Serial.print("delay:");          Serial.println(envDelay);
#endif
      chgPot2 = false;
    }
  }
  else if (prevParam == 2) { //PARAM-lfo
    if (chgPot1) { //lfo waveform
      lfoWave = prevPot1 >> 4;
      switch (lfoWave) {
        case 0:
          lfo.setTable(SIN2048_DATA);
#ifdef DEBUG
          Serial.println("lfo-sin");
#endif
          break;
        case 1:
          lfo.setTable(TRIANGLE2048_DATA);
#ifdef DEBUG
          Serial.println("lfo-tri");
#endif
          break;
        case 2:
          lfo.setTable(SAW2048_DATA);
#ifdef DEBUG
          Serial.println("lfo-saw");
#endif
          break;
        case 3:
          lfo.setTable(SQUARE_NO_ALIAS_2048_DATA);
#ifdef DEBUG
          Serial.println("lfo-sqr");
#endif
          break;
      }
      chgPot1 = false;
    }
    if (chgPot2) { //lfo frequ
      lfoFreq  = (float)(prevPot2 >> 1); //0-31
#ifdef DEBUG
      Serial.print("lfo-freq:");          Serial.println(lfoFreq);
#endif
      lfo.setFreq(lfoFreq);
      chgPot2 = false;
    }
  }
  else if (prevParam == 3) { //PARAM-filter
    if (chgPot1) { //cutoff
      lpfCutoff = prevPot1 << 2;  // 0 - 255
      lpf.setCutoffFreq(lpfCutoff);
#ifdef DEBUG
      Serial.print("cutoff:");          Serial.println(lpfCutoff);
#endif
      chgPot1 = false;
    }
    if (chgPot2) { //resonance
      lpfResonance = prevPot2 << 2; //0-255
      lpf.setResonance(lpfResonance);
#ifdef DEBUG
      Serial.print("resonance:");          Serial.println(lpfResonance);
#endif
      chgPot2 = false;
    }
  }
  chgParam = false;
}


void readPotis () { // all potentiometers wired in reverse, sorry :-)
  int value;
  if (currPot == 0) {
    value = 3 - (mozziAnalogRead (MODE_PIN) >> 8); // read mode poti and reduce range to 0-3
    if (value != prevMode) // change of mode
    {
      prevMode = value;
      chgMode = true;
      if (phraseType > 3 && prevMode == MODE_SONG) phraseType = 3; // only 4 songs but 8 phrases, for phrase>3 reset to 3
    }
  } else if (currPot == 1) {
    value = 7 - (mozziAnalogRead (PARAM_PIN) >> 7); // read mode poti and reduce range to 0-7
    if (value != prevParam) // change of mode
    {
      prevParam = value;
      chgParam = true;
    }
  } else if (currPot == 2) {
    value = 63 - (mozziAnalogRead (POT1_PIN) >> 4); // read mode poti and reduce range to 0-63

    if (value != prevPot1) // change of pot1
    {
      prevPot1 = value;
      chgPot1 = true;
    }
  } else if (currPot == 3) {
    value = 63 - (mozziAnalogRead (POT2_PIN) >> 4); // read mode poti and reduce range to 0-63
    if (value != prevPot2) // change of pot2
    {
      prevPot2 = value;
      chgPot2 = true;
    }
  } else if (currPot == 4) {
    value = 63 - (mozziAnalogRead (POT3_PIN) >> 4); // read mode poti and reduce range to 0-63
    if (value != prevPot3) // change of pot3
    {
      prevPot3 = value;
      chgPot3 = true;
    }
    currPot = -1;
  }
  currPot++;
}
