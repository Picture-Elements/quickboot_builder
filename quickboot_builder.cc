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
 * This program reads in two .bit files, which are gold and silver
 * FPGA images, and writes them into a single .mcs file with a
 * Quickboot header as described in XAPP1081. This .mcs file is used
 * to do factory programming of the device prom. The target device can
 * then do safe updates of the silver FPGA image with some confidence
 * that the device will not be bricked if the silver image is broken.
 *
 * COMMAND LINE FLAGS:
 *   --output=<path>  Specify the output file. The resuling file will
 *                 contain the .mcs file stream.
 *
 *   --gold=<path> Specify the gold design. This should be a .bit file
 *                 as generated by Xilinx tools. Note that this .bit
 *                 file should NOT include the IPROG command as the
 *                 quickboot header that this program writes will
 *                 include a multiboot and IPROG command sequence.
 *
 *   --silver=<path>
 *                 Specify the silver design. This is the most current
 *                 design, and the design that can be updated in the
 *                 field. This file will be written to the "multiboot"
 *                 address, and the quickboot header will point to it.
 *
 *   --spi
 *   --bpi16
 *                 Specify the quickboot format, whether to write an
 *                 image for SPI flash devices or BPI devices. Exactly
 *                 one of these flags must be given.
 *
 *   --multiboot=<number>
 *                 Specify the multiboot offset. If this flag is not
 *                 present, the program will try to extract it from
 *                 the gold bit stream. If it cannot find it there,
 *                 then the program will quit with an error.
 *
 *   --disable-silver
 *   --no-disable-silver  (default)
 *                 Write the silver stream into the mcs file, but
 *                 disable it by leaving the Critical Switch word
 *                 off. Use this either to test the fallback, or to
 *                 program the device in "gold" mode and let a future
 *                 field update load and enable the silver.
 *
 *    --debug-trash-silver
 *                 Intentionally corrupt the silver image by blanking
 *                 a random sector. This is a debug aid to make sure
 *                 the fallback works. It's not intended to be used in
 *                 the real world.
 *
 * FIELD PROGRAMMING:
 * The quickboot image includes both the gold and the silver FPGA
 * images in a single MCS stream that can be written to the PROM. In
 * the field, the sinver image is meant to be updatable. The silver
 * image is in the prom at the MULTIBOOT address. To update the silver
 * image in the field, follow this process:
 *
 *   1: Erase (to 0xff) the first page/sector of the flash.
 *
 *        This disables the quickboot of the silver image, so that
 *        reboot of the flash will load the gold image. Leave the
 *        quickboot disabled until the silver image is updated.
 *
 *   2: Erase and reprogram the silver image.
 *
 *        The silver image is written to the MULTIBOOT address. Erase
 *        from there to the end of the prom, and write the new silver
 *        image in place. It is recommended that the programmer read
 *        back the entire silver image to assure that the program
 *        process worked properly.
 *
 *   3: Restore quickboot
 *
 *        Write the Critical Switch word to the last address of the
 *        first page. This re-enables the quickboot boot of the silver
 *        image.
 */

# include  <vector>
# include  <cstdint>
# include  <cstdio>
# include  <cstdlib>
# include  <cstring>
# include  <cassert>

using namespace std;

static size_t flash_sector  = 0;
static const size_t QUICKBOOT_SPACE = 64;

static bool disable_silver = false;


static bool test_gold_image_compatible(const std::vector<uint8_t>&vec);
static uint32_t extract_multiboot_address(std::vector<uint8_t>&vec, bool bpi16_gen, uint32_t&WBSTAR);
static void spi_quickboot_header(std::vector<uint8_t>&dst, size_t mb_offset, size_t sector);
static void bpi16_quickboot_header(std::vector<uint8_t>&dst, size_t mb_offset, uint32_t WBSTAR, size_t sector);
static void bpi16_fixup_endian(std::vector<uint8_t>&dst);
static void write_to_mcs_file(FILE*fd, const std::vector<uint8_t>&vec);

/*
 * Read a bit file into the dst vector, and strip the header details
 * off. The result is a vector that starts at the mark (0xffffffff)
 * bytes.
 */
static void read_bit_file(vector<uint8_t>&dst, FILE*fd)
{
      fseek(fd, 0, SEEK_END);
      size_t file_size = ftell(fd);
      assert(file_size > 0);

      dst.resize(file_size);

      fseek(fd, 0, SEEK_SET);
      size_t rc = fread(&dst[0], 1, file_size, fd);
      if (rc != file_size) {
	    fprintf(stderr, "Unable to read bit file bytes\n");
	    dst.clear();
	    return;
      }

	/* Look for 0xff bytes that indicate the end of the header. */

      rc = 0;
      while (rc < dst.size() && dst[rc] != 0xff) {
	    rc += 1;
      }

      if (rc == dst.size()) {
	    fprintf(stderr, "Unable to find end of header in bit file.\n");
	    dst.clear();
	    return;
      }

	/* Strip the header off of the bit file. The dst vector now
	   contains the bit stream, ready to insert. */
      if (rc > 0) {
	    memmove(&dst[0], &dst[rc], dst.size()-rc);
	    dst.resize(dst.size()-rc);
      }
}

int main(int argc, char*argv[])
{
      size_t multiboot_offset = 0;
      const char*path_out = 0;
      const char*path_gold = 0;
      const char*path_silver = 0;
      bool bpi16_gen = false;
      bool spi_gen = false;
      bool debug_trash_silver = false;

	/* Test and interpret the command line flags. */
      for (int optarg = 1 ; optarg < argc ; optarg += 1) {
	    if (strncmp(argv[optarg],"--output=",9) == 0) {
		  path_out = argv[optarg] + 9;

	    } else if (strncmp(argv[optarg],"--gold=",7) == 0) {
		  path_gold = argv[optarg] + 7;

	    } else if (strncmp(argv[optarg],"--silver=",9) == 0) {
		  path_silver = argv[optarg] + 9;

	    } else if (strcmp(argv[optarg],"--bpi16") == 0) {
		  bpi16_gen = true;

	    } else if (strcmp(argv[optarg],"--spi") == 0) {
		  spi_gen = true;

	    } else if (strncmp(argv[optarg],"--multiboot=",12) == 0) {
		  multiboot_offset = strtoul(argv[optarg]+12, 0, 0);

	    } else if (strcmp(argv[optarg],"--disable-silver") == 0) {
		  disable_silver = true;

	    } else if (strcmp(argv[optarg],"--no-disable-silver") == 0) {
		  disable_silver = false;

	    } else if (strcmp(argv[optarg],"--debug-trash-silver") == 0) {
		  debug_trash_silver = true;

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

      if (path_gold == 0) {
	    fprintf(stderr, "No gold file? Please specify --gold=<path>\n");
	    return -1;
      }

      if (path_silver == 0) {
	    fprintf(stderr, "No silver file? Please specify --silver=<path>\n");
	    return -1;
      }

	// If the flash sector size is not otherwise specified, then
	// choose a default based on the targeted flash device.
      if (flash_sector == 0) {
	    if (spi_gen) {
		  flash_sector = 4096;
	    } else if (bpi16_gen) {
		  flash_sector = 32768;
	    }
      }

	/* Read the gold file, strip any header, and get it ready to
	   be included in the result file. */
      FILE*fd_gold = fopen(path_gold, "rb");
      if (fd_gold == 0) {
	    fprintf(stderr, "Unable to open gold file: %s\n", path_gold);
	    return -1;
      }

      fprintf(stdout, "Reading gold file: %s\n", path_gold);
      fflush(stdout);
      vector<uint8_t> vec_gold;
      read_bit_file(vec_gold, fd_gold);
      if (vec_gold.size() == 0)
	    return -1;

      fclose(fd_gold);
      fd_gold = 0;

      if (! test_gold_image_compatible(vec_gold)) {
	    fprintf(stderr, "Gold file %s not compatible with Quickboot assembly.\n", path_gold);
	    return -1;
      }

	/* Read the silver file, strip any header, and be ready. */
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

      uint32_t WBSTAR = 0;
      size_t use_multiboot = extract_multiboot_address(vec_gold, bpi16_gen, WBSTAR);
      fprintf(stdout, "Extracted WBSTAR register value: 0x%08x\n", WBSTAR);
      fprintf(stdout, "Extracted multiboot BYTE address: 0x%08zX\n", use_multiboot);
      if (multiboot_offset == 0) {
	    multiboot_offset = use_multiboot;
      } else {
	      // If we get the address from the command line, then we
	      // will be generating a new WBSTAR address later.
	    WBSTAR = 0;
      }

      if (multiboot_offset == 0) {
	    fprintf(stderr, "Unable to guess the MULTIBOOT address. Please use --multiboot=<number>\n");
	    return -1;
      }

      if (multiboot_offset % flash_sector != 0) {
	    fprintf(stderr, "MULTIBOOT Address 0x%08zx is not on a prom sector boundary\n", multiboot_offset);
	    fprintf(stderr, "PROM sector size is %zu bytes\n", flash_sector);
	    return -1;
      }


      if ((vec_gold.size() + flash_sector + QUICKBOOT_SPACE) > multiboot_offset) {
	    fprintf(stderr, "Unable to fit gold bits into region.\n");
	    fprintf(stderr, "Gold file is %zu bytes\n", vec_gold.size());
	    fprintf(stderr, "MULTIBOOT byte address is 0x%08zx\n", multiboot_offset);
	    fprintf(stderr, "Quickboot header is %zu bytes\n", flash_sector + QUICKBOOT_SPACE);
	    return -1;
      }

      fprintf(stdout, "MULTIBOOT Address: 0x%08zx\n", multiboot_offset);
      fprintf(stdout, "PROM Page Size: %zu bytes\n", flash_sector);

	// To simulate failing to program a segment of the prom, erase
	// some random sector in the silver image.
      if (debug_trash_silver) {
	    size_t trash_offset = vec_silver.size() / 2;
	    trash_offset &= ~(flash_sector-1);
	    fprintf(stdout, "**** DEBUG Trash sector at 0x%08zx in silver image.\n", trash_offset);
	    for (size_t idx = 0 ; idx < flash_sector ; idx += 1)
		  vec_silver[trash_offset+idx] = 0xff;
      }


	/* Now the vec_gold and vec_silver vectors contain the bit
	   files that will go into the quickboot assembled mcs
	   stream. */
      vector<uint8_t> vec_out;
      vec_out.resize(multiboot_offset + vec_silver.size());
      memset(&vec_out[0], 0xff, vec_out.size());

	/* Write the gold file into the stream. */
      memcpy(&vec_out[flash_sector+QUICKBOOT_SPACE], &vec_gold[0], vec_gold.size());

	/* Write the silver file into the stream. */
      memcpy(&vec_out[multiboot_offset], &vec_silver[0], vec_silver.size());

	/* Generate a quickboot header for the type of flash that we
	   are targetting. */
      assert(spi_gen || bpi16_gen);
      if (spi_gen) {
	    spi_quickboot_header(vec_out, multiboot_offset, flash_sector);

      } else if (bpi16_gen) {
	    bpi16_quickboot_header(vec_out, multiboot_offset, WBSTAR, flash_sector);
	    bpi16_fixup_endian(vec_out);
      }

	/* Write the generated image to a .mcs file. This file can be
	   written to the prom by prom programmer. */
      FILE*fd_out = fopen(path_out, "wb");
      if (fd_out == 0) {
	    fprintf(stderr, "Unable to open output file: %s\n", path_out);
	    return -1;
      }

      write_to_mcs_file(fd_out, vec_out);

      fclose(fd_out);
      fd_out = 0;

	/* All done. */
      return 0;
}

static bool test_gold_image_compatible(const std::vector<uint8_t>&vec)
{
      const uint8_t magic_sync[4] = {0xaa, 0x99, 0x55, 0x66};
      size_t match = 0;
      size_t ptr = 0;

      while (ptr < 0x100 && match < 4) {
	    if (vec[ptr+match] == magic_sync[match]) {
		  match += 1;
	    } else {
		  ptr += 1;
		  match = 0;
	    }
      }

      if (match < 4) {
	    fprintf(stderr, "Unable to find sync word in gold file.\n");
	    return false;
      }

      ptr += match;

      const uint8_t magic_iprog[8] = {0x30, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x0f};

      match = 0;
      while (ptr < 0x100 && match < 8) {
	    if (vec[ptr+match] == magic_iprog[match]) {
		  match += 1;
	    } else {
		  ptr += 1;
		  match = 0;
	    }
      }

      if (match < 8) {
	    return true;
      }

      fprintf(stderr, "Found IPROG command in gold stream.\n");

      return false;
}

/*
 * Extract the multiboot address from a bit stream by searching for
 * the WRITE to WBSTAR early in the bit stream. If I find it, only the
 * low 24bits are useful so mask the high bits. If I do not find it,
 * then return 0.
 */
static uint32_t extract_multiboot_address(std::vector<uint8_t>&vec, bool bpi16_gen, uint32_t&WBSTAR)
{
      const uint8_t magic[4] = {0x30, 0x02, 0x00, 0x01};
      size_t match = 0;
      size_t ptr = 0;

      while (ptr < 0x100 && match < 4) {
	    if (vec[ptr+match] == magic[match]) {
		  match += 1;
	    } else {
		  ptr += 1;
		  match = 0;
	    }
      }

      if (match < 4)
	    return 0;

      ptr += match;

	// Get the WBSTAR value
      uint32_t wbstar = vec[ptr+0];
      wbstar <<= 8;
      wbstar |= vec[ptr+1];
      wbstar <<= 8;
      wbstar |= vec[ptr+2];
      wbstar <<= 8;
      wbstar |= vec[ptr+3];

      uint32_t res = 0;
      if (bpi16_gen) {
	    if (wbstar & 0x40000000) /* RS[0]? */
		  res |= 0x00800000;
	    res |= 2*(wbstar & 0x003fffff);

      } else {
	      // For SPI devices, only use the low address bits
	    res = wbstar & 0x00ffffff;
      }

      WBSTAR = wbstar;
      return res;
}

static void spi_quickboot_header(std::vector<uint8_t>&dst, size_t mb_offset, size_t sector)
{
      fprintf(stdout, "Quickboot SPI header\n");
      fprintf(stdout, "Critical Switch word is aa:99:55:66 at 0x%08zx (page 0)\n", sector-4);

      memset(&dst[0], 0xff, sector-4);

      if (disable_silver) {
	    dst[sector- 4] = 0xff;
	    dst[sector- 3] = 0xff;
	    dst[sector- 2] = 0xff;
	    dst[sector- 1] = 0xff;
      } else {
	    dst[sector- 4] = 0xaa; /* Sync word */
	    dst[sector- 3] = 0x99;
	    dst[sector- 2] = 0x55;
	    dst[sector- 1] = 0x66;
      }
      dst[sector+ 0] = 0x20; /* NOOP */
      dst[sector+ 1] = 0x00;
      dst[sector+ 2] = 0x00;
      dst[sector+ 3] = 0x00;
      dst[sector+ 4] = 0x30; /* Write to WBSTAR */
      dst[sector+ 5] = 0x02;
      dst[sector+ 6] = 0x00;
      dst[sector+ 7] = 0x01;
      dst[sector+ 8] = (mb_offset>>24) & 0xff;
      dst[sector+ 9] = (mb_offset>>16) & 0xff;
      dst[sector+10] = (mb_offset>> 8) & 0xff;
      dst[sector+11] = (mb_offset>> 0) & 0xff;
      dst[sector+12] = 0x30; /* Write to COMMAND */
      dst[sector+13] = 0x00;
      dst[sector+14] = 0x80;
      dst[sector+15] = 0x01;
      dst[sector+16] = 0x00;
      dst[sector+17] = 0x00;
      dst[sector+18] = 0x00;
      dst[sector+19] = 0x0f; /* ... IPROG command */
	/* Fill the reset of the second sector with NOOP commands */
      for (size_t idx = 20 ; idx < QUICKBOOT_SPACE ; idx += 4) {
	    dst[sector+idx+0] = 0x20;
	    dst[sector+idx+1] = 0x00;
	    dst[sector+idx+2] = 0x00;
	    dst[sector+idx+3] = 0x00;
      }
}

static void bpi16_quickboot_header(std::vector<uint8_t>&dst, size_t mb_offset, uint32_t WBSTAR, size_t sector)
{
      fprintf(stdout, "Quickboot BPI header\n");
      fprintf(stdout, "Critical Switch word is 00:00:00:bb at 0x%08zx (page 0)\n", sector-4);

	// If we got the multiboot address from the command line
	// instead of from the gold file, then generate a new WBSTAR value.
      if (WBSTAR == 0) {
	      // In the quickboot header for a BPI16 device, we use RS[0]
	      // instead of any other multiboot bits.

	    if (mb_offset & 0x01000000)
		  WBSTAR |= 0x60000000; /* RS[0],RS_TS_B */

	    WBSTAR |= (mb_offset & 0x00ffffff) / 2;
      }

      memset(&dst[0], 0xff, sector-4);

      if (disable_silver) {
	      // If the silver is disabled, then leave the critical
	      // switch word out.
	    dst[sector-4] = 0xff;
	    dst[sector-3] = 0xff;
	    dst[sector-2] = 0xff;
	    dst[sector-1] = 0xff;
      } else {
	    dst[sector- 4] = 0x00; /* Critical Switch word word */
	    dst[sector- 3] = 0x00;
	    dst[sector- 2] = 0x00;
	    dst[sector- 1] = 0xbb;
      }
      dst[sector+ 0] = 0x11; /* bus width detect */
      dst[sector+ 1] = 0x22;
      dst[sector+ 2] = 0x00;
      dst[sector+ 3] = 0x44;
      dst[sector+ 4] = 0xff;
      dst[sector+ 5] = 0xff;
      dst[sector+ 6] = 0xff;
      dst[sector+ 7] = 0xff;
      dst[sector+ 8] = 0xff;
      dst[sector+ 9] = 0xff;
      dst[sector+10] = 0xff;
      dst[sector+11] = 0xff;
      dst[sector+12] = 0xaa; /* Sync word */
      dst[sector+13] = 0x99;
      dst[sector+14] = 0x55;
      dst[sector+15] = 0x66;
      dst[sector+16] = 0x20; /* NOOP */
      dst[sector+17] = 0x00;
      dst[sector+18] = 0x00;
      dst[sector+19] = 0x00;
      dst[sector+20] = 0x30; /* WRITE to WBSTAR */
      dst[sector+21] = 0x02;
      dst[sector+22] = 0x00;
      dst[sector+23] = 0x01;
      dst[sector+24] = (WBSTAR >> 24) & 0xff;
      dst[sector+25] = (WBSTAR >> 16) & 0xff;
      dst[sector+26] = (WBSTAR >>  8) & 0xff;
      dst[sector+27] = (WBSTAR >>  0) & 0xff;
      dst[sector+28] = 0x30; /* Write to COMMAND */
      dst[sector+29] = 0x00;
      dst[sector+30] = 0x80;
      dst[sector+31] = 0x01;
      dst[sector+32] = 0x00;
      dst[sector+33] = 0x00;
      dst[sector+34] = 0x00;
      dst[sector+35] = 0x0f; /* ... IPROG command */

	/* Fill the reset of the second sector with NOOP commands */
      for (size_t idx = 36 ; idx < QUICKBOOT_SPACE ; idx += 4) {
	    dst[sector+idx+0] = 0x20;
	    dst[sector+idx+1] = 0x00;
	    dst[sector+idx+2] = 0x00;
	    dst[sector+idx+3] = 0x00;
      }
}

static void bpi16_fixup_endian(std::vector<uint8_t>&dst)
{
      for (size_t idx = 0 ; idx < dst.size() ; idx += 2) {
	    uint8_t tmp = dst[idx+1];
	    uint8_t val0 = tmp & 1;
	    for (int bit = 1 ; bit < 8 ; bit += 1) {
		  val0 <<= 1;
		  tmp >>= 1;
		  val0 |= tmp & 1;
	    }

	    tmp = dst[idx+0];
	    uint8_t val1 = tmp & 1;
	    for (int bit = 1 ; bit < 8 ; bit += 1) {
		  val1 <<= 1;
		  tmp >>= 1;
		  val1 |= tmp & 1;
	    }

	    dst[idx+0] = val0;
	    dst[idx+1] = val1;
      }

}

/*
 * Write the entire assembled vector into the output file as an .mcs
 * stream.
 */
static void write_to_mcs_file(FILE*fd, const std::vector<uint8_t>&vec)
{
      size_t address = 0;

      while (address < vec.size()) {
	    int sum = 2 + 4 + ((address>>16)&0xff) + ((address>>24)&0xff);

	      /* Write an extended address record. */
	    fprintf(fd, ":02000004%04zX%02X\n", address>>16, 0xff & -sum);

	      /* Now write up to 64K worth of bytes, 16 at a time. */
	    size_t addr2 = 0;
	    while ((addr2 < 0x10000) && ((address+addr2) < vec.size())) {
		  size_t trans = 16;
		  int sum = 0;

		  if (address+addr2+trans > vec.size())
			trans = vec.size() - address - addr2;

		  fprintf(fd, ":%02zX%04zX00", trans, addr2);
		  sum += trans;
		  sum += (addr2&0xff) + ((addr2>>8) & 0xff);

		  for (size_t idx = 0 ; idx < trans ; idx += 1) {
			fprintf(fd, "%02X", vec[address+addr2+idx]);
			sum += vec[address+addr2+idx];
		  }

		  fprintf(fd, "%02X\n", 0xff & -sum);
		  addr2 += trans;
	    }

	    address += addr2;
      }

	/* EOF Marker */
      fprintf(fd, ":00000001FF\n");
}
