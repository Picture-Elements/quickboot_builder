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

# include  "test_image_compat.h"
# include  "extract_register_write.h"
# include  <cstdio>

using namespace std;

bool test_basic_image_compatibility(const std::vector<uint8_t>&vec)
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
	    fprintf(stderr, "Unable to find sync word in bit file.\n");
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

      fprintf(stderr, "Found IPROG command in bit stream.\n");

      return false;
}

bool test_silver_image_compatible(const std::vector<uint8_t>&vec)
{
      if (!test_basic_image_compatibility(vec)) {
	    fprintf(stderr, "Silver image fails basic tests.\n");
	    return false;
      }


      uint32_t AXSS = extract_register_write(vec, 0x0d);
      if (AXSS != 0x53494c56) {
	    fprintf(stderr, "Found AXSS=0x%08x\n (s/b 0x53494c56)\n", AXSS);
	    return false;
      }

      return true;
}
