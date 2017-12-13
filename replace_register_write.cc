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

# include  "replace_register_write.h"
# include  <cassert>

using namespace std;

uint32_t replace_register_write(std::vector<uint8_t>&vec, uint32_t addr, uint32_t val)
{
      const uint8_t magic[4] = {0xaa, 0x99, 0x55, 0x66};
      size_t match = 0;
      size_t ptr = 0;

	// Scan into the bit stream for the sync word.
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

	// Now the ptr points into the bit file at the first
	// command. Step forward through the commands until we find
	// the write to the address we are after, or we are past the
	// interesting commands.
      ptr += match;

      while (ptr < 0x100) {
	    uint32_t word = vec[ptr+0];
	    word <<= 8;
	    word |= vec[ptr+1];
	    word <<= 8;
	    word |= vec[ptr+2];
	    word <<= 8;
	    word |= vec[ptr+3];

	      // Is this a NOP?
	    if ((word&0xf8000000) == 0x20000000) { // NOP
		  ptr += 4;
		  continue;
	    }

	    if ((word&0xf8000000) == 0x28000000) { // READ
		  ptr += 4;
		  assert(0); // What are reads doing here?
	    }

	    if ((word&0xf8000000) == 0x30000000) { // WRITE
		  uint32_t wc = word&0x3ff;
		  uint32_t ad = (word>>13) & 0x3fff;
		  if (ad != addr) {
			ptr += 4 + wc*4;
			continue;
		  }

		  assert(wc == 1);
		  ptr += 4;
		    // Get the existing word being written.
		  word = vec[ptr+0];
		  word <<= 8;
		  word |= vec[ptr+1];
		  word <<= 8;
		  word |= vec[ptr+2];
		  word <<= 8;
		  word |= vec[ptr+3];
		    // Put the new value into the word write.
		  vec[ptr+3] = val & 0xff;
		  val >>= 8;
		  vec[ptr+2] = val & 0xff;
		  val >>= 8;
		  vec[ptr+1] = val & 0xff;
		  val >>= 8;
		  vec[ptr+0] = val & 0xff;
		    // Return the old value.
		  return word;
	    }

	      // The first type-2 header should mark the start of the
	      // bit stream and the end of other commands.
	    if ((word&0xe0000000) == 0x40000000) { // Type-2 header
		  break;
	    }

	      // Everything should have been accounted for.
	    assert(0);
      }

      return 0;
}
