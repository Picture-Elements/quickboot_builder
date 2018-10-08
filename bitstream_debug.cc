/*
 * Copyright (c) 2018 Picture Elements, Inc.
 *    Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

# include  "read_bit_file.h"
# include  <vector>
# include  <cstdint>
# include  <cstdio>
# include  <cstdlib>
# include  <cstring>
# include  <cassert>

using namespace std;

static const char*register_name(uint8_t address)
{
      static const char*name_table[32] = {
	    "CRC",    "FAR",    "FDRI",    "FDRO", // 00000 - 00011
	    "CMD",    "CTL0",   "MASK",    "STAT", // 00100 - 00111
	    "LOUT",   "COR0",   "MFWR",    "CBC",  // 01000 - 01011
	    "IDCODE", "AXSS",   "COR1",    "0x0f", // 01100 - 01111
	    "WBSTAR", "TIMER",  "0x12",    "0x13", // 10000 - 10011
	    "0x14",   "0x15",   "BOOTSTS", "0x17", // 10100 - 10111
	    "CTL1",   "0x19",   "0x1a",    "0x1b", // 11000 - 11011
	    "0x1c",   "0x1d",   "0x1e",    "BSPI", // 11100 - 11111
      };
      address &= 0x1f;
      return name_table[address];
}

static const char*command_name(uint32_t command)
{
      static const char*name_table[32] = {
	    "NULL",    "WCFG",    "MFW",      "LFRM",     // 00000 - 00011
	    "RCFG",    "START",   "RCAP",     "RCRC",     // 00100 - 00111
	    "AGHIGH",  "SWITCH",  "GRESTORE", "SHUTDOWN", // 01000 - 01011
	    "GCAPTURE","DESYNC",  "0x0e",     "IPROG",    // 01100 - 01111
	    "CRCC",    "LTIMER",  "BSPI_READ","FALL_EDGE",// 10000 - 10011
	    "0x14",    "0x15",    "0x16",     "0x17",     // 10100 - 10111
	    "0x18",    "0x19",    "0x1a",     "0x1b",     // 11000 - 11011
	    "0x1c",    "0x1d",    "0x1e",     "0x1f",     // 11100 - 11111
      };
      command &= 0x1f;
      return name_table[command];
}

static void process_type1(const vector<uint8_t>&vec, size_t&ptr)
{
      assert(ptr+4 <= vec.size());
      uint32_t command = vec[ptr+0];
      command <<= 8;
      command += vec[ptr+1];
      command <<= 8;
      command += vec[ptr+2];
      command <<= 8;
      command += vec[ptr+3];

      size_t word_count = (command >>  0) & 0x000007ff;
      uint8_t address   = (command >> 13) & 0x1f;
      uint8_t opcode    = (command >> 27) & 0x3;

      assert(ptr+4+4*word_count <= vec.size());

      switch (opcode) {
	  case 0: /* NO-OP */
	    fprintf(stdout, "NOP            (word_count=%zu):", word_count);
	    for (size_t idx = 0 ; idx < word_count ; idx += 1) {
		  uint32_t val = vec[ptr+4+4*idx+0];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+1];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+2];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+3];
		  fprintf(stdout, " %08x", val);
	    }
	    fprintf(stdout, "\n");
	    break;
	  case 1: /* Read */
	    fprintf(stdout, "Read  %-8s (word_count=%zu)\n", register_name(address), word_count);
	    break;
	  case 2: /* Write */
	    fprintf(stdout, "Write %-8s (word_count=%zu):", register_name(address), word_count);
	    for (size_t idx = 0 ; idx < word_count ; idx += 1) {
		  uint32_t val = vec[ptr+4+4*idx+0];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+1];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+2];
		  val <<= 8;
		  val += vec[ptr+4+4*idx+3];
		  fprintf(stdout, " %08x", val);
		  if (address == 0x04 && idx == 0) {
			fprintf(stdout, " (%s)", command_name(val));
		  }
	    }
	    fprintf(stdout, "\n");
	    break;
	  case 3: /* Reserved Opcode */
	    fprintf(stdout, "RESERVED       (address=0x%x, word_count=%zu)\n", address, word_count);
	    break;
	  default:
	    assert(0);
      }

	// Advance the pointer past the packet.
      ptr += 4 + 4*word_count;
}

static void process_type2(const vector<uint8_t>&vec, size_t&ptr)
{
      assert(ptr+4 <= vec.size());
      size_t word_count = vec[ptr+0] & 0x07;
      word_count <<= 8;
      word_count += vec[ptr+1];
      word_count <<= 8;
      word_count += vec[ptr+2];
      word_count <<= 8;
      word_count += vec[ptr+3];

      fprintf(stdout, "Type 2 Packet: word_count=%zu (0x%zx)\n", word_count, word_count);
      fprintf(stdout, " ... skip %zu bytes of data ...\n", 4 * word_count);
      
      assert(ptr + 4 + 4*word_count <= vec.size());
      ptr += 4 + 4*word_count;
}

int main(int argc, char*argv[])
{
      const char*path_in = 0;

      for (int optarg = 1 ; optarg < argc ; optarg += 1) {
	    if (strncmp(argv[optarg],"--input=",8) == 0) {
		  path_in = argv[optarg] + 8;

	    } else {
		  fprintf(stderr, "Unknown flag: %s\n", argv[optarg]);
		  return -1;
	    }
      }

      if (path_in == 0) {
	    fprintf(stderr, "Please specify an input file with --input=<path>.\n");
	    return -1;
      }

      FILE*fd_in = fopen(path_in, "rb");
      if (fd_in == 0) {
	    fprintf(stderr, "Unable to open input .bit file: %s\n", path_in);
	    return -1;
      }

      fprintf(stdout, "Reading input .bit file: %s\n", path_in);
      fflush(stdout);
      
      vector<uint8_t> vec_in;
      read_bit_file(vec_in, fd_in);
      if (vec_in.size() == 0)
	    return -1;

      fclose(fd_in);
      fd_in = 0;

      size_t ptr = 0;
	// Look for bus width detect words.
	// Scan past the ff prefix stream.
      while (vec_in[ptr] == 0xff) {
	    ptr += 1;
      }

      if (vec_in[ptr+0] != 0x00)
	    return -1;
      if (vec_in[ptr+1] != 0x00)
	    return -1;
      if (vec_in[ptr+2] != 0x00)
	    return -1;
      if (vec_in[ptr+3] != 0xbb)
	    return -1;
      if (vec_in[ptr+4] != 0x11)
	    return -1;
      if (vec_in[ptr+5] != 0x22)
	    return -1;
      if (vec_in[ptr+6] != 0x00)
	    return -1;
      if (vec_in[ptr+7] != 0x44)
	    return -1;

      fprintf(stdout, "Bus width detect code (8 bytes) at offset 0x%04zx\n", ptr);
      ptr += 8;

	// Look for sync word
      while (vec_in[ptr] == 0xff) {
	    ptr += 1;
	    if (ptr >= vec_in.size()) break;
      }

      if (ptr+4 >= vec_in.size()) {
	    fprintf(stderr, "NO SYNC WORD\n");
	    return -1;
      }
      
      if (vec_in[ptr+0] != 0xaa)
	    return -1;
      if (vec_in[ptr+1] != 0x99)
	    return -1;
      if (vec_in[ptr+2] != 0x55)
	    return -1;
      if (vec_in[ptr+3] != 0x66)
	    return -1;

      fprintf(stdout, "Sync word (4 bytes) at offset 0x%04zx\n", ptr);
      ptr += 4;
      
	// Now we found the sync word. The stream is happening and
	// should be just type 1 and type 2 headers from now on.
      while (ptr < vec_in.size()) {
	    switch ((vec_in[ptr] >> 5) & 0x07) {
		case 1: /* Type 1 */
		  process_type1(vec_in, ptr);
		  break;
		case 2: /* Type 2 */
		  process_type2(vec_in, ptr);
		  break;
		default: /* error */
		  fprintf(stderr, "mal-formed packet at ptr=0x%04zx\n", ptr);
		  return 01;
	    }
      }


      return 0;
}
