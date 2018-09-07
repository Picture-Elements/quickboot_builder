/*
 * Copyright (c) 2017 Picture Elements, Inc.
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

/*
 *  Take in a raw .bit file from Xilinx tools and convert it into a
 *  .bit file that is ready for field installation into the flash.
 *
 *  NOTE: This program is NOT needed to prepare .bit file for input to
 *  the quickboot_builder3 program. Only use this program if you are
 *  NOT using the quickboot_builder3 program to prepare the silver image.
 */

# include  "read_bit_file.h"
# include  "replace_register_write.h"
# include  <vector>
# include  <cstdio>
# include  <cstdint>
# include  <cstdlib>
# include  <cstring>
# include  <cassert>

using namespace std;

int main(int argc, char*argv[])
{
      const char*path_out = 0;
      const char*path_raw = 0;

      for (int optarg = 1 ; optarg < argc ; optarg += 1) {
	    if (strncmp(argv[optarg], "--output=",9) == 0) {
		  path_out = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg], "--raw=",6) == 0) {
		  path_raw = argv[optarg] + 6;

	    } else {
		  fprintf(stderr, "Unknown flag: %s\n", argv[optarg]);
		  return -1;
	    }
      }

      if (path_out == 0) {
	    fprintf(stderr, "No output file? Please specify --output=<path>\n");
	    return -1;
      }

      if (path_raw == 0) {
	    fprintf(stderr, "No raw input file? Please specify --raw=<path>\n");
	    return -1;
      }

	// Read the raw file, strip any header, and be ready.
      FILE*fd_raw = fopen(path_raw, "rb");
      if (fd_raw == 0) {
	    fprintf(stderr, "Unable to open raw file: %s\n", path_raw);
	    return -1;
      }

      fprintf(stdout, "Reading raw file: %s\n", path_raw);
      fflush(stdout);
      vector<uint8_t> vec_raw;
      read_bit_file(vec_raw, fd_raw, 256+32);
      if (vec_raw.size() == 0)
	    return -1;

      fclose(fd_raw);
      fd_raw = 0;

	// Edit the silver stream BSPI register value.
      uint32_t old_BSPI = replace_register_write(vec_raw, 0x14, 0x0c);
      fprintf(stdout, "BSPI (silver): 0x00000c (was: 0x%08x)\n", old_BSPI);

      FILE*fd_out = fopen(path_out, "wb");
      if (fd_out == 0) {
	    fprintf(stderr, "Unable to open output file: %s\n", path_out);
	    return -1;
      }

      size_t rc = fwrite(&vec_raw[0], 1, vec_raw.size(), fd_out);
      assert(rc == vec_raw.size());

      fclose(fd_out);

      return 0;
}
