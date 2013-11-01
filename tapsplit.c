/************************************************************************

    TAPTOOLS v1.0.4 - Tapefile manipulation utilities

    Copyright (C) 1996, 2005, 2007, 2009  John Elliott <jce@seasip.demon.co.uk>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taputil.h"

#ifdef __PACIFIC__
#define AV0 "TAPSPLIT"
#else
#define AV0 argv[0]
#endif

byte header[18];
FILE *fpi;
unsigned int blklen;
byte blktype;
byte p3hdr[128];
byte p3sig[] = "PLUS3DOS\032\001";
int uctr = 1;
int nctr = 1;
char *outdir = NULL;

#define NAMELENGTH 100	/* Maximum filename length. 100 is massive overkill
			   when normal Spectrum names don't exceed 10 chars */

char lfname[NAMELENGTH];
char *lfpath;

void mkname(char *name)
{
	if (outdir) sprintf(lfpath, "%s/%s", outdir, name);
	else	strncpy(lfpath, name, NAMELENGTH - 1);
	lfpath[NAMELENGTH - 1] = 0;
}

FILE *lfopen(char *name, char *mode)
{
	mkname(name);
	return fopen(lfpath, mode);
}

void progress(char *what, char *name)
{
	mkname(name);
	printf("%s %s\n", what, lfpath);
}

void x_header(void)
{
	char *p;
	if (fread(header, 1, 18, fpi) < 18)	/* 17 data & 1 checksum */
	{
		fclose(fpi);
		fprintf(stderr, "Unexpected EOF\n");
		exit(1);
	}
	sprintf(lfname, "%-10.10s", header + 1); 
	lfname[10] = 0;
	/* Chop off any trailing spaces */
	for (p = lfname + 9; p >= lfname; p--)
	{
		if (p[0] != ' ') break;
		*p = 0;
	}

	memset(p3hdr, 0, sizeof(p3hdr));
	memcpy(p3hdr, p3sig, sizeof(p3sig));

	p3hdr[15] = header[0];
	memcpy(p3hdr + 16, header + 11, 6);
}

void x_data(void)
{
	unsigned long blk2 = blklen + 0x7E;	/* +80h for +3dos header */
	FILE *fpo = NULL;			/* -2 for type & checksum*/
	int n;
	byte csum;
	
	p3hdr[11] =  blk2        & 0xFF;
	p3hdr[12] = (blk2 >>  8) & 0xFF;	
	p3hdr[13] = (blk2 >> 16) & 0xFF;
	p3hdr[14] = 0;

	for (n = csum = 0; n < 127; n++) csum += p3hdr[n];
	p3hdr[n] = csum;

	if (lfname[0])
	{
		if (lfname[0] < 32) sprintf(lfname, "unnamed.%d", nctr++);

		do
		{
			fpo = lfopen(lfname, "r");
			if (fpo == NULL) break;
			strcat(lfname, ".1");
			fclose(fpo);
		} while(1);

		progress("Writing", lfname);

		fpo = lfopen(lfname, "wb");
		if (fpo == NULL)
		{
			perror(lfpath);
			exit(1);
		}
		fwrite(p3hdr, 1, 128, fpo);
	}
	else
	{
		if (blktype == 'S' && blklen == 49181)
		{
			sprintf(lfname, "Block%d.sna", uctr++);
			progress("Writing snapshot", lfname);
		}
		else
		{
			sprintf(lfname, "Block%d", uctr++);
			progress("Writing headerless", lfname);
		}
		fpo = lfopen(lfname, "wb");
		if (fpo == NULL)
		{
			perror(lfpath);
			exit(1);
		}
	}
	while (blklen > 2)	/* 2 extra bytes counted in blklen - */
	{			/* block type & checksum */
		int c = fgetc(fpi);
		if (c == EOF)
		{
			fprintf(stderr, "Error reading TAP file\n");
			fclose(fpo);
			exit(1);	
		}
		fputc(c, fpo);
		--blklen;
	}
	fclose(fpo);
	fgetc(fpi);	/* Checksum */
	lfname[0] = 0;
}


int main(int argc, char **argv)
{
	long taplen;
	int is_zxt;

	if (argc < 2)
	{
		fprintf(stderr, "Syntax: %s tapfile { <output-dir> }\n", AV0);
		return 1;
	}
	if (argc > 2)
	{
		outdir = argv[2];
		lfpath = malloc(NAMELENGTH + 1 + strlen(outdir));
		printf("Output directory is %s\n", outdir);
	}
	else
	{
		lfpath = malloc(NAMELENGTH);
	}

	fpi = opentap(argv[1], "rb", &is_zxt, &taplen);
	if (!fpi) 
	{
		fprintf(stderr, "Can't open %s\n", argv[1]);
		exit(1);
	}
	lfname[0] = 0;

	while (taplen > 0)
	{
		blklen = fgetc(fpi);          if (feof(fpi)) break;
		blklen += 256 * fgetc(fpi);   if (feof(fpi)) break;
		blktype = fgetc(fpi);         if (feof(fpi)) break;

		if (blklen == 0x13 && blktype == 0) x_header();
		else                                x_data();
		taplen -= (blklen + 2);
	}
	fclose(fpi);
	free(lfpath);
	return 0;
}
