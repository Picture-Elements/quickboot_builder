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
 *  Take in a silver file and convert it to gold.
 */

# include  "read_bit_file.h"
# include  "replace_register_write.h"
# include  "test_image_compat.h"
# include  "disable_stream_crc.h"
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
      const char*path_silver = 0;

      bool bpi16_gen = false;
      bool spi_gen = false;

      for (int optarg = 1 ; optarg < argc ; optarg += 1) {
	    if (strncmp(argv[optarg], "--output=",9) == 0) {
		  path_out = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg], "--silver=",9) == 0) {
		  path_silver = argv[optarg] + 9;

	    } else if (strcmp(argv[optarg],"--bpi16") == 0) {
		  bpi16_gen = true;

	    } else if (strcmp(argv[optarg],"--spi") == 0) {
		  spi_gen = true;

	    } else {
		  fprintf(stderr, "Unknown flag: %s\n", argv[optarg]);
		  return -1;
	    }
      }

      if (spi_gen==false && bpi16_gen==false) {
	    fprintf(stderr, "BPI16 or SPI? Please specify --bpi16 or --spi\n");
	    return -1;
      }

      if (spi_gen && bpi16_gen) {
	    fprintf(stderr, "Please specify only one of --bpi16 or --spi\n");
	    return -1;
      }

      if (path_out == 0) {
	    fprintf(stderr, "No output file? Please specify --output=<path>\n");
	    return -1;
      }

      if (path_silver == 0) {
	    fprintf(stderr, "No silver file file? Please specify --silver=<path>\n");
	    return -1;
      }

	// Read the silver file, strip any header, and be ready.
      FILE*fd_silver = fopen(path_silver, "rb");
      if (fd_silver == 0) {
	    fprintf(stderr, "Unable to open silver file: %s\n", path_silver);
	    return -1;
      }

      fprintf(stdout, "Reading silver file: %s\n", path_silver);
      fflush(stdout);
      vector<uint8_t> vec_silver;
      read_bit_file(vec_silver, fd_silver);
      if (vec_silver.size() == 0)
	    return -1;

      fclose(fd_silver);
      fd_silver = 0;

      if (! test_silver_image_compatible(vec_silver)) {
	    fprintf(stderr, "Silver file %s not compatible with Quickboot assembly.\n", path_silver);
	    return -1;
      }

	// MAke sure the gold stream has a gold magic number.
      uint32_t AXSS = replace_register_write(vec_silver, 0xd, 0x474f4c44);
      fprintf(stdout, "AXSS (gold): 0x474f4c44 (was: 0x%08x)\n", AXSS);

      if (bpi16_gen) {
	    uint32_t COR0 = replace_register_write(vec_silver, 0x09, 0x062055dc);
	    fprintf(stdout, "COR0 (gold): 0x062055dc (was: 0x%08x)\n", COR0);

	    uint32_t COR1 = replace_register_write(vec_silver, 0x0e, 0x0000000a);
	    fprintf(stdout, "COR1 (gold): 0x0000000a (was: 0x%08x)\n", COR1);
      }

      while (disable_stream_crc(vec_silver)) {
	/* repeat */
      }

      FILE*fd_out = fopen(path_out, "wb");
      if (fd_out == 0) {
	    fprintf(stderr, "Unable to open output file: %s\n", path_out);
	    return -1;
      }

      size_t rc = fwrite(&vec_silver[0], 1, vec_silver.size(), fd_out);
      assert(rc == vec_silver.size());

      fclose(fd_out);

      return 0;
}
