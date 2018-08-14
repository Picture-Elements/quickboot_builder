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
 *
 * Optionally, make sure the mark is padded to be at least pad_ff
 * bytes of pad.
 */
void read_bit_file(vector<uint8_t>&dst, FILE*fd, size_t pad_ff)
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

	/* rc is now the offset into the image where the first 0xff
	   (after the header) starts. Now count the number of 0xff
	   bytes are present. */
      size_t ff_count = 0;
      while ((dst[rc+ff_count] == 0xff) && ((rc+ff_count) < dst.size()))
	    ff_count += 1;

      if (pad_ff < ff_count)
	    pad_ff = ff_count;

	/* Now ff_count is the number of leading ff bytes that we
	   have, and pad_ff is the number of ff bytes that we
	   nead. Also, pad_ff >= ff_count. */

      if ((pad_ff-ff_count) > rc) {
	      /* If the amount of extra pad we need is more then the
		 amount of header that we can trim, then expand the
		 target array to make room for the pad. Note that we
		 can overwrite header with pad. */
	    size_t shift = pad_ff - ff_count - rc;
	    dst.resize(dst.size() + shift);
	    memmove(&dst[shift], &dst[0], dst.size());

      } else if ((pad_ff-ff_count) < rc) {
	      /* If the amount of extra pad we need is less then the
		 amount of header that we can trim, then trim off the
		 excess header and shrink the target array. Leave
		 enough space for the target ff pad. */
	    size_t shift = rc - (pad_ff-ff_count);
	    memmove(&dst[0], &dst[shift], dst.size() - shift);
	    dst.resize(dst.size() - shift);
      }

	/* In any case, make sure the prefix starts with pad. This may
	   erase unused header, or fill in new array space. */
      memset(&dst[0], 0xff, pad_ff);
}
