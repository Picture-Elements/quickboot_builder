
# This is a makefile for building the quickboot_builder under Linux.
# requires a c++ compiler that supports c++-2011 standard.

CXX = g++ -std=c++11
CXXFLAGS = -O -g -Wall

all: quickboot_builder quickboot_gold quickboot_builder3 quickboot_silver3 quickboot_gold3

clean:
	rm -f *.o *~

O = quickboot_builder.o extract_register_write.o read_bit_file.o replace_register_write.o test_image_compat.o disable_stream_crc.o write_to_mcs_file.o

quickboot_builder: $O
	$(CXX) $(CXXFLAGS) -o quickboot_builder $O

O3 = quickboot_builder3.o read_bit_file.o write_to_mcs_file.o replace_register_write.o disable_stream_crc.o

quickboot_builder3: $(O3)
	$(CXX) $(CXXFLAGS) -o quickboot_builder3 $(O3)

G = quickboot_gold.o read_bit_file.o test_image_compat.o extract_register_write.o replace_register_write.o disable_stream_crc.o

quickboot_gold: $G
	$(CXX) $(CXXFLAGS) -o quickboot_gold $G

S3 = quickboot_silver3.o read_bit_file.o replace_register_write.o

quickboot_silver3: $(S3)
	$(CXX) $(CXXFLAGS) -o quickboot_silver3 $(S3)

G3 = quickboot_gold3.o read_bit_file.o replace_register_write.o disable_stream_crc.o

quickboot_gold3: $(G3)
	$(CXX) $(CXXFLAGS) -o quickboot_gold3 $(G3)

quickboot_builder.o: quickboot_builder.cc read_bit_file.h extract_register_write.h replace_register_write.h test_image_compat.h disable_stream_crc.h write_to_mcs_file.h

quickboot_builder3.o: quickboot_builder3.cc disable_stream_crc.h read_bit_file.h replace_register_write.h write_to_mcs_file.h

quickboot_gold.o: quickboot_gold.cc read_bit_file.h replace_register_write.h test_image_compat.h disable_stream_crc.h

quickboot_silver3.o: quickboot_silver3.cc read_bit_file.h replace_register_write.h

quickboot_gold3.o: quickboot_gold3.cc read_bit_file.h replace_register_write.h disable_stream_crc.h

read_bit_file.o:     read_bit_file.cc read_bit_file.h
extract_register_write.o: extract_register_write.cc extract_register_write.h
replace_register_write.o: replace_register_write.cc replace_register_write.h
test_image_compat.o: test_image_compat.cc test_image_compat.h extract_register_write.h
disable_stream_crc.o: disable_stream_crc.cc disable_stream_crc.h
write_to_mcs_file.o: write_to_mcs_file.cc write_to_mcs_file.h
