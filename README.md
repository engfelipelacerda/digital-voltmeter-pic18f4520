# Digital Voltmeter - PIC18F4520

A dual-range digital voltmeter developed using the PIC18F4520 microcontroller.

This project was developed as part of the Microprocessors course and simulates a digital voltmeter using Proteus and MPLAB X IDE with the XC8 compiler.

## Features

- Voltage measurement from 0 to 5 V
- Automatic scale switching
  - 0–999 mV
  - 1–5 V
- 10-bit ADC conversion
- Three multiplexed 7-segment displays
- Common Cathode displays
- Display multiplexing using Timer0 interrupt
- Average of multiple ADC readings to reduce noise

## Hardware

- PIC18F4520
- 4 MHz crystal oscillator
- Three Common Cathode 7-segment displays
- 10 kΩ potentiometer (analog input)
- Current limiting resistors
- Proteus simulation
- Resistors

## Software

- MPLAB X IDE
- XC8 Compiler
- Proteus 8 Professional

## Building

1. Open the MPLAB X project.
2. Compile using the XC8 compiler.
3. Load the generated HEX file into the Proteus project.
4. Run the simulation.

## Author

Luis Felipe Pessoa Lacerda

Federal University of Ceará (UFC)

Microprocessors
