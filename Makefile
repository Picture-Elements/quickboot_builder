
# This is a makefile for building the quickboot_builder under Linux.
# requires a c++ compiler that supports c++-2011 standard.

CXX = g++ -std=c++11
CXXFLAGS = -O -g -Wall

all: quickboot_builder quickboot_gold

clean:
	rm -f *.o *~

O = quickboot_builder.o extract_register_write.o read_bit_file.o replace_register_write.o test_image_compat.o disable_stream_crc.o

quickboot_builder: $O
	$(CXX) $(CXXFLAGS) -o quickboot_builder $O

G = quickboot_gold.o read_bit_file.o test_image_compat.o extract_register_write.o replace_register_write.o disable_stream_crc.o

quickboot_gold: $G
	$(CXX) $(CXXFLAGS) -o quickboot_gold $G

quickboot_builder.o: quickboot_builder.cc read_bit_file.h extract_register_write.h replace_register_write.h test_image_compat.h disable_stream_crc.h

quickboot_gold.o: quickboot_gold.cc read_bit_file.h replace_register_write.h test_image_compat.h disable_stream_crc.h

read_bit_file.o:     read_bit_file.cc read_bit_file.h
extract_register_write.o: extract_register_write.cc extract_register_write.h
replace_register_write.o: replace_register_write.cc replace_register_write.h
test_image_compat.o: test_image_compat.cc test_image_compat.h extract_register_write.h
disable_stream_crc.o: disable_stream_crc.cc disable_stream_crc.h
