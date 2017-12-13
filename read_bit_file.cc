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

# include  "read_bit_file.h"
# include  <cstring>
# include  <cassert>

using namespace std;


/*
 * Read a bit file into the dst vector, and strip the header details
 * off. The result is a vector that starts at the mark (0xffffffff)
 * bytes.
 */
void read_bit_file(vector<uint8_t>&dst, FILE*fd)
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
