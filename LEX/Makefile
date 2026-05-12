# Makefile for LEX folder
# part of the BASIC256 project.

# revision History
# date.... programmer... description...
# ????????               original coding
# 20170612 j.m.reneau    added noline options to bison and flex for Rasberry Pi Build
# 20200405 j.m.reneau    added output of debugging file 

all: basicParse.l basicParse.y
	bison -d -v -l basicParse.y
	flex -L basicParse.l

