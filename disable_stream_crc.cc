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

# include  "disable_stream_crc.h"
# include  <cassert>

using namespace std;

bool disable_stream_crc(std::vector<uint8_t>&vec)
{
      size_t ptr = vec.size();
      const size_t base = ptr - 3192;

      const uint8_t magic[4] = {0x30, 0x00, 0x00, 0x01};
      size_t match = 0;

      ptr -= 4;
      while (ptr>base && match<4) {
	    if (vec[ptr+match] == magic[match]) {
		  match += 1;
		  continue;
	    }

	    match = 0;
	    ptr -= 4;
      }

      if (match < 4)
	    return false;

      assert(ptr+8 <= vec.size());

	// Replace the CRC code with the Reset CRC command.
      vec[ptr+0] = 0x30;
      vec[ptr+1] = 0x00;
      vec[ptr+2] = 0x80;
      vec[ptr+3] = 0x01;
      vec[ptr+4] = 0x00;
      vec[ptr+5] = 0x00;
      vec[ptr+6] = 0x00;
      vec[ptr+7] = 0x07;
      return true;
}
