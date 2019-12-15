# CLUSynthO
Arduino-based digital synthesizer to explore digital sound generation

phrase sequenzer

based on the Mozzi library https://sensorium.github.io/Mozzi/

dedicated to a friend - happy knob-fiddling, Cluso!

wiring: potentiometer connected to A1-A5, audio connected through low pass filter (220 Ohm, 100nF) and amplifier, can be powered via USB or 3x AA battery (in 3D-printed battery case inside) - never switch on battery when on USB!

MODE I: *phrase mode*, PARAM: phrase, A: speed/direction, B: pitch

MODE II: *synth mode* virtual analog subtractive synth

    PARAM 1: *oscillator* A: waveform sine/triangle/sawtooth/square, B: LFO depth, C: envelope depth
  
    PARAM 2: *envelope* A: attack, B: decay
  
    PARAM 3: *lfo* A: waveform sine/triangle/sawtooth/square, B: frequency
  
    PARAM 4: *filter* A: cutoff, B: resonance
  
MODE III: *song mode*, PARAM: song, A: speed, B: pitch

MODE IV: to do (maybe a differend synth...)
