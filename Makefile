
# This is a makefile for building the quickboot_builder under Linux.
# requires a c++ compiler that supports c++-2011 standard.

CXX = g++ -std=c++11
CXXFLAGS = -O -g -Wall

O = quickboot_builder.o extract_register_write.o read_bit_file.o replace_register_write.o

quickboot_builder: $O
	$(CXX) $(CXXFLAGS) -o quickboot_builder $O

quickboot_builder.o: quickboot_builder.cc read_bit_file.h extract_register_write.h replace_register_write.h
read_bit_file.o:     read_bit_file.cc read_bit_file.h
extract_register_write.o: extract_register_write.cc extract_register_write.h
replace_register_write.o: replace_register_write.cc replace_register_write.h
