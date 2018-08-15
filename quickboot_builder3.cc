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

/*
 * COMMAND LINE FLAGS:
 *   --output=<path>  Specify the output file. The output file will
 *                    contain the .mcs file stream.
 *
 *   --disable-silver
 *   --no-disable-silver (default)
 *                    Debug aid. Intentionally corrupt the silver
 *                    image, so that the fall back to gold can be
 *                    tested.
 *
 *   --clif32-4=<path>
 *   --clif32-6=<path>
 *   --clif30=<path>
 *   --clif31=<path>
 *                    Specify the various input designs that go into
 *                    making the flash image. These input .bit files
 *                    are taken to be silver files. Gold files are
 *                    generated from the silver files.
 */

# include  "disable_stream_crc.h"
# include  "read_bit_file.h"
# include  "replace_register_write.h"
# include  "write_to_mcs_file.h"
# include  <vector>
# include  <cstdint>
# include  <cstdio>
# include  <cstdlib>
# include  <cstring>
# include  <cassert>

using namespace std;

/*
 * S25FL128/256 flash chips in Hybrid sector size option
 * have 64Kbyte sectors.
 */
static const size_t flash_sector = 64*1024;

/*
 * Silver image is 4MBytes past Gold image
 */
static const size_t multiboot_offset = 4*1024*1024;

/*
 * Flash contains designs that are gold/silver pairs.
 */
static const size_t design_offset = 2 * multiboot_offset;

static bool debug_trash_silver = false;

static void make_design(vector<uint8_t>&vec_out, int design_pos, const vector<uint8_t>&vec_silver);

int main(int argc, char*argv[])
{
      const char*path_out = 0;
      const char*path_clif32_6 = 0;
      const char*path_clif32_4 = 0;
      const char*path_clif31 = 0;
      const char*path_clif30 = 0;
      FILE*fd = 0;

      for (int optarg = 1 ; optarg < argc ; optarg += 1) {
	    if (strncmp(argv[optarg],"--output=",9) == 0) {
		  path_out = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg],"--clif30=",9) == 0) {
		  path_clif30 = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg],"--clif31=",9) == 0) {
		  path_clif31 = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg],"--clif32-4=",11) == 0) {
		  path_clif32_4 = argv[optarg] + 11;

	    } else if (strncmp(argv[optarg],"--clif32-6=",11) == 0) {
		  path_clif32_6 = argv[optarg] + 11;

	    } else if (strcmp(argv[optarg],"--disable-silver") == 0) {
		  debug_trash_silver = true;

	    } else if (strcmp(argv[optarg],"--no-disable-silver") == 0) {
		  debug_trash_silver = false;

	    } else {
	    }
      }

      if (path_out == 0) {
	    fprintf(stderr, "No output file? Please specify --output=<path>\n");
	    return -1;
      }
#if 0
      if (path_clif30 == 0) {
	    fprintf(stderr, "No CLIF30 file? Please specify --clif30=<path>\n");
	    return -1;
      }
#endif
#if 0
      if (path_clif31 == 0) {
	    fprintf(stderr, "No CLIF31 file? Please specify --clif31=<path>\n");
	    return -1;
      }
#endif
      if (path_clif32_4 == 0) {
	    fprintf(stderr, "No CLIF32-4 file? Please specify --clif32-4=<path>\n");
	    return -1;
      }

	/* Number of designs to load. */
      size_t design_count = 0;

	/* Read in the CLIF32-4 design */
      fd = fopen(path_clif32_4, "rb");
      if (fd == 0) {
	    fprintf(stderr, "Unable to open CLIF32-4 file: %s\n", path_clif32_4);
	    return -1;
      }

      fprintf(stdout, "Reading CLIF32-4 silver file: %s\n", path_clif32_4);
      fflush(stdout);
      vector<uint8_t> vec_clif32_4;
      read_bit_file(vec_clif32_4, fd, 256+32 /* Need large 0xff pad */);
      if (vec_clif32_4.size() == 0)
	    return -1;

      design_count += 1;
      fclose(fd);

	/* If the user also specifies the CLIF32-6 design, then load
	   it as well. */
      vector<uint8_t> vec_clif32_6;
      if (path_clif32_6) {
	      /* Read in the CLIF32-6 design */
	    fd = fopen(path_clif32_6, "rb");
	    if (fd == 0) {
		  fprintf(stderr, "Unable to open CLIF32-6 file: %s\n", path_clif32_6);
		  return -1;
	    }

	    fprintf(stdout, "Reading CLIF32-6 silver file: %s\n", path_clif32_6);
	    fflush(stdout);
	    read_bit_file(vec_clif32_6, fd, 256+32 /* Need large 0xff pad */);
	    if (vec_clif32_6.size() == 0)
		  return -1;

	    design_count += 1;
	    fclose(fd);
      }

      vector<uint8_t> vec_out;
	/* Make an image that holds the designs. */
      vec_out.resize(design_count * design_offset);
      memset(&vec_out[0], 0xff, vec_out.size());


      fprintf(stdout, "Processing CLIF32-4 design...\n");
      fflush(stdout);

      make_design(vec_out, 0, vec_clif32_4);


      if (vec_clif32_6.size() > 1) {
	    fprintf(stdout, "Processing CLIF32-6 design...\n");
	    fflush(stdout);

	    make_design(vec_out, 1, vec_clif32_6);
      }


      fprintf(stdout, "Done processing designs, writing mcs file.\n");
      fd = fopen(path_out, "wb");
      if (fd == 0) {
	    fprintf(stderr, "Unable to open output file: %s\n", path_out);
	    return -1;
      }
      fflush(stdout);

      write_to_mcs_file(fd, vec_out);

      fclose(fd);
      fd = 0;

      return 0;
}

/*
 * Make a design into the output vector, based on the design position
 * (0-3) and the input silver file. Generate a gold file, and write
 * both into the output image and the correct position for the design.
 */
static void make_design(vector<uint8_t>&vec_out, int design_pos, const vector<uint8_t>&raw_silver)
{
	/* Location in the image of this design (gold and silver) */
      const size_t design_base = design_pos * design_offset;
	/* This is the BSPI value to use. */
      const uint8_t BSPI = 0x0c;

	/* Local copy of the silver image, that we can edit. */
      vector<uint8_t> vec_silver = raw_silver;

	/* Make a gold image from the silver input. */
      vector<uint8_t> vec_gold = vec_silver;

      uint32_t AXSS = replace_register_write(vec_gold, 0x0d, 0x474f4c44);
      fprintf(stdout, "... AXSS (gold): 0x474f4c44 (was: 0x%08x)\n", AXSS);

      uint32_t old_BSPI = replace_register_write(vec_gold, 0x1f, BSPI);
      fprintf(stdout, "... BSPI (gold): 0x%08x (was: 0x%08x)\n", BSPI, old_BSPI);

	/* Gold images have the CRC disabled. */
      while (disable_stream_crc(vec_gold)) {
	      /* repeat */
      }

      old_BSPI = replace_register_write(vec_silver, 0x1f, BSPI);
      fprintf(stdout, "... BSPI (silver): 0x%08x (was: 0x%08x)\n", BSPI, old_BSPI);

	/* Put the gold image here (after the design_base) to allow
	   space for the quickboot header. */
      const size_t gold_start = flash_sector*2;

      if (gold_start+vec_gold.size() > multiboot_offset) {
	    fprintf(stderr, "ERROR: Gold image (%zu bytes) does not fit "
		    "in multiboot region (%zu bytes)\n",
		    vec_gold.size(), multiboot_offset - gold_start);
      }

	/* Write the CLIF32-4 images into the total image. */
      fprintf(stdout, "... Write GOLD image at byte address 0x%08zx\n",
	      design_base + gold_start);
      memcpy(&vec_out[design_base + gold_start], &vec_gold[0], vec_gold.size());

      fprintf(stdout, "... Write SILVER image at byte address 0x%08zx\n",
	      multiboot_offset);
      memcpy(&vec_out[design_base + multiboot_offset], &vec_silver[0], vec_silver.size());

      if (debug_trash_silver) {
	    size_t trash_offset = vec_silver.size() / 2;
	    trash_offset &= ~(flash_sector-1);
	    fprintf(stdout, "*** DEBUG Trash sector at 0x%08zx in silver image.\n", trash_offset);
	    for (size_t idx = 0 ; idx < flash_sector ; idx += 1)
		  vec_out[design_base+multiboot_offset+trash_offset+idx] = 0xff;
      }

	/* Generate a quickboot header for the image set. */
      fprintf(stdout, "... Critical Switch word is aa:99:55:66 at 0x%08zx\n",
	      design_base + flash_sector - 4);
      fflush(stdout);

      uint32_t offset = design_base + multiboot_offset; /* Branch to silver. */
	// write offset[32:8] to WBSTAR instead of [23:0]. We will be
	// writing a 0x0000000c to BSPI to call out that mode.
      offset >>= 8;
	// Assert that START_ADDR in WBSTAR does not overflow into the
	// RS_TS_B and RS bits.
      assert((offset & 0xe0000000) == 0);


      vec_out[design_base + flash_sector - 4] = 0xaa; /* Sync word */
      vec_out[design_base + flash_sector - 3] = 0x99; /* ... */
      vec_out[design_base + flash_sector - 2] = 0x55; /* ... */
      vec_out[design_base + flash_sector - 1] = 0x66; /* ... */
      vec_out[design_base + flash_sector + 0] = 0x20; /* NOOP */
      vec_out[design_base + flash_sector + 1] = 0x00; /* ... */
      vec_out[design_base + flash_sector + 2] = 0x00; /* ... */
      vec_out[design_base + flash_sector + 3] = 0x00; /* ... */
      vec_out[design_base + flash_sector + 4] = 0x30; /* Write to BSPI */
      vec_out[design_base + flash_sector + 5] = 0x03; /* ... */
      vec_out[design_base + flash_sector + 6] = 0xe0; /* ... */
      vec_out[design_base + flash_sector + 7] = 0x01; /* ... */
      vec_out[design_base + flash_sector + 8] = 0x00; /* ... */
      vec_out[design_base + flash_sector + 9] = 0x00; /* ... */
      vec_out[design_base + flash_sector +10] = 0x00; /* ... */
      vec_out[design_base + flash_sector +11] = BSPI; /* ... */
      vec_out[design_base + flash_sector +12] = 0x30; /* Write to Command */
      vec_out[design_base + flash_sector +13] = 0x00; /* ... */
      vec_out[design_base + flash_sector +14] = 0x80; /* ... */
      vec_out[design_base + flash_sector +15] = 0x01; /* ... */
      vec_out[design_base + flash_sector +16] = 0x00; /* ... */
      vec_out[design_base + flash_sector +17] = 0x00; /* ... */
      vec_out[design_base + flash_sector +18] = 0x00; /* ... */
      vec_out[design_base + flash_sector +19] = 0x12; /* ... BSPI_Read */
      vec_out[design_base + flash_sector +20] = 0x20; /* NOOP */
      vec_out[design_base + flash_sector +21] = 0x00; /* ... */
      vec_out[design_base + flash_sector +22] = 0x00; /* ... */
      vec_out[design_base + flash_sector +23] = 0x00; /* ... */
      vec_out[design_base + flash_sector +24] = 0x20; /* NOOP */
      vec_out[design_base + flash_sector +25] = 0x00; /* ... */
      vec_out[design_base + flash_sector +26] = 0x00; /* ... */
      vec_out[design_base + flash_sector +27] = 0x00; /* ... */
      vec_out[design_base + flash_sector +28] = 0x20; /* NOOP */
      vec_out[design_base + flash_sector +29] = 0x00; /* ... */
      vec_out[design_base + flash_sector +30] = 0x00; /* ... */
      vec_out[design_base + flash_sector +31] = 0x00; /* ... */
      vec_out[design_base + flash_sector +32] = 0x30; /* Write to WBSTAR */
      vec_out[design_base + flash_sector +33] = 0x02; /* ... */
      vec_out[design_base + flash_sector +34] = 0x00; /* ... */
      vec_out[design_base + flash_sector +35] = 0x01; /* ... */
      vec_out[design_base + flash_sector +36] = (offset>>24) & 0xff; /* ... */
      vec_out[design_base + flash_sector +37] = (offset>>16) & 0xff; /* ... */
      vec_out[design_base + flash_sector +38] = (offset>> 8) & 0xff; /* ... */
      vec_out[design_base + flash_sector +39] = (offset>> 0) & 0xff; /* ... */
      vec_out[design_base + flash_sector +40] = 0x30; /* Write to COMMAND */
      vec_out[design_base + flash_sector +41] = 0x00; /* ... */
      vec_out[design_base + flash_sector +42] = 0x80; /* ... */
      vec_out[design_base + flash_sector +43] = 0x01; /* ... */
      vec_out[design_base + flash_sector +44] = 0x00; /* ... */
      vec_out[design_base + flash_sector +45] = 0x00; /* ... */
      vec_out[design_base + flash_sector +46] = 0x00; /* ... */
      vec_out[design_base + flash_sector +47] = 0x0f; /* ... IPROG */

	/* Pad the rest of the flash_sector with NOOP */
      for (size_t idx = 48 ; idx < flash_sector ; idx += 4) {
	    vec_out[design_base + flash_sector + idx + 0] = 0x20;
	    vec_out[design_base + flash_sector + idx + 1] = 0x00;
	    vec_out[design_base + flash_sector + idx + 2] = 0x00;
	    vec_out[design_base + flash_sector + idx + 3] = 0x00;
      }
}
