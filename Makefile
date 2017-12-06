
# This is a makefile for building the quickboot_builder under Linux.
# requires a c++ compiler that supports c++-2011 standard.

quickboot_builder: quickboot_builder.cc
	g++ -O -g -std=c++11 -Wall -o quickboot_builder quickboot_builder.cc
