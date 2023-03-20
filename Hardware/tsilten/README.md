# Tsiltsen -- a tool for working with human-readable netlists.

The name is stupid, it's 'netlist' written backwards, and impossible to spell. It sounds like some ethnic ancient British cheese.

## Purpose
I never like drawing schematics with a COD tool if they were never intended for use in producing a PCB. I always found using a bare netlist format much
more natural. A netlist lists the connections between pins on the various components in your design, and can be made very compact if you take advantage 
of the repetition inherent in much design work, eg a counter IC has eight outputs Q0, Q1,... Q7, or more succinctly Q[0-7].

A netlist is also the _natural_ format to use if you are wiring up a prototype by hand. Each time you make a connection, you strike it out on the netlist.

## A Simple example

Here's a very simple example circuit, it doesn't even need a schematic. A battery, a switch, a resistor and a LED. Here's a complete design:

$parts
LED
	{A,K}
RES
	{1,2}
BATT 
	{+VE,-VE}
SW
	{NO,C,NC}
	
$comps
LED1 	LED
R1		RES		1K0
SW1		SW
B1		BATT

$nets
V+			B1/+VE SW1/NO
			SW1/C R1/1
			R1/1 LED/A
0V			B1/-VE LED1/K

This can be processed by tsilten to give a complete netlist. This is very easy to work with for building a prototype:

V+:			B1/+VE SW1/NO
SW1/C-R1/1:	SW1/C R1/1
R1/1-LED/A:	R1/1 LED/A
0V:			B1/-VE LED1/K

## A More Complex example

This uses all the facilities of tsilten.