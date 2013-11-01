/************************************************************************

    TAPTOOLS v1.0.3 - Tapefile manipulation utilities

    Copyright (C) 1996, 2005, 2007  John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/
#ifdef HAVE_LIBDSK_H
#include "libdsk.h"
#else

/* In case the real libdsk is not present, this implements a 
 * positively skeletal substitute. */
typedef FILE *DSK_PDRIVER;
typedef struct 
{
	int dg_secsize;	
	int dg_sectors;
} DSK_GEOMETRY;
typedef int dsk_err_t;
typedef int dsk_ltrack_t;
typedef long dsk_lsect_t;
#define DSK_ERR_OK     (0)
#define DSK_ERR_BADPTR (-1)
#define DSK_ERR_NOMEM  (-2)
#define DSK_ERR_SYSERR (-6)

dsk_err_t dsk_creat(DSK_PDRIVER *self, const char *name, const char *type,
		const char *compress);
dsk_err_t dsk_close(DSK_PDRIVER *fp);
dsk_err_t dg_pcwgeom(DSK_GEOMETRY *dg, const unsigned char *bootsec);
dsk_err_t dsk_alform(DSK_PDRIVER fp, DSK_GEOMETRY *dg, dsk_ltrack_t track, 
		unsigned char filler);
dsk_err_t dsk_lwrite(DSK_PDRIVER fp, DSK_GEOMETRY *dg, void *buf, int sector);
const char *dsk_strerror(int err);
void dsk_reportfunc_set(void *f1, void *f2);
void dsk_report(const char *s);
void dsk_report_end();
#endif
