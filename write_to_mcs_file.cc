/*
 * Copyright (c) 2017,2018 Picture Elements, Inc.
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

# include  "write_to_mcs_file.h"

/*
 * Write the entire assembled vector into the output file as an .mcs
 * stream.
 */
void write_to_mcs_file(FILE*fd, const std::vector<uint8_t>&vec)
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

      fprintf(stdout, "MCS target device size >= 0x%08zx\n", address);

	/* EOF Marker */
      fprintf(fd, ":00000001FF\n");
}
