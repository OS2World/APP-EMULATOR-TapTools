/************************************************************************

    TAPTOOLS v1.0.3 - Tapefile manipulation utilities

    Copyright (C) 1996, 2005, 2008  John Elliott <jce@seasip.demon.co.uk>

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

#include "config.h"
#include "cnvshell.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

typedef enum { PLUS3DOS, TAP, ZXT } FORMAT;

static FORMAT st_format;
static char st_filename[11] = "Default   ";

unsigned char basic1[] =
{
	0x00, 0x0A, 0xFF, 0xFF, 0xEA,	/* 10 <len xx> REM */
};

unsigned char basic2[] = 
{
	0x0D,				/* End of line 10 */
	0x00, 0x14, 			/* Line 20 */
	0xFF, 0xFF,			/* <len xx> */
	0xF1, 'x', '=', 0xC0, '(', '5',	/* LET x=USR (5 */
/* For testing: Print execution address rather than jumping to it */
/*	0xF5, '(', '5',		*/	/* PRINT (5 */
	0x0E, 0, 0, 5, 0, 0, '+', 	/* [number 5] + */
	0xBE, '2', '3', '6', '3', '5',	/* PEEK 23635 */
	0x0E, 0, 0, 0x53, 0x5C, 0, '+',	/* [number 23635] + */
	'2', '5', '6',			/* + 256 */
	0x0E, 0, 0, 0, 1, 0, '*',	/* [number 256] * */
	0xBE, '2', '3', '6', '3', '6',	/* PEEK 23636 */
	0x0E, 0, 0, 0x54, 0x5C, 0,	/* [number 23636] */
	')', 0x0D	
};

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (variable[0] == 't' || variable[0] == 'T')
	{
		st_format = TAP;
		if (value && value[0]) sprintf(st_filename, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 'z' || variable[0] == 'Z')
	{
		st_format = ZXT;
		if (value && value[0]) sprintf(st_filename, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 'p' || variable[0] == 'P')
	{
		st_format = PLUS3DOS;
		return NULL;
	}
	
	return "Unrecognised option";
}

char *cnv_help(void)
{
	return "Syntax: bin2bas { -t | -z | -p } infile outfile\n\n"
		"-t=filename: Output in .TAP format\n"
		"-z=filename: Output in .ZXT format\n"
		"-p: Output in +3DOS format (default)\n";
}

static int writeout(FILE *fp, unsigned char *data, unsigned len, unsigned char *sum)
{
	unsigned char c;

	while (len)
	{
		c = *data;
		++data;
		*sum ^= c;
		if (fputc(c, fp) == EOF) return -1;
		--len;	
	}
	return 0;
}


static int copyover(FILE *fp, FILE *inp, unsigned len, unsigned char *sum)
{
	int c;

	while (len)
	{
		c = fgetc(inp);
		if (c == EOF) return -1;
		*sum ^= c;
		if (fputc(c, fp) == EOF) return -1;
		--len;	
	}
	return 0;
}


char *cnv_execute(FILE *infile, FILE *outfile)
{
	unsigned char p3header[128];
	unsigned char tapheader[24];
	unsigned char sum;
	int n;
	long size, base, bassize, outsize = 0L;

	if (fseek(infile, 0, SEEK_END)) return "Cannot fseek in input file";
	size = ftell(infile);
	if (fseek(infile, 0, SEEK_SET)) return "Cannot fseek in input file";
	base = 0;
	if (fread(p3header, 1, 128, infile) == 128)
	{
		for (sum = n = 0; n < 127; n++) sum += p3header[n];
		if (sum == p3header[n] && !memcmp(p3header, "PLUS3DOS\032", 9)) 
		{
			base = 128;		 
		}
	}
	if (fseek(infile, base, SEEK_SET)) return "Cannot fseek in input file";
	memset(p3header, 0, sizeof(p3header));
	memcpy(p3header, "PLUS3DOS\032\001\000", 11);

	bassize = (size - base) + sizeof(basic1) + sizeof(basic2);

	/* BASIC line 10 has REM + code + newline */
	basic1[2] = (size + 2 - base) & 0xFF;
	basic1[3] = (size + 2 - base) >> 8;

	/* BASIC line 20: Length is sizeof array less 5 */
	basic2[3] = (sizeof(basic2) - 5) & 0xFF;
	basic2[4] = (sizeof(basic2) - 5) >> 8;


	p3header[15] = 0;			/* BASIC */
	p3header[16] = (bassize) & 0xFF;	/* Size of program+vars */
	p3header[17] = (bassize >> 8) & 0xFF;
	p3header[18] = 0;			/* Autostart line */
	p3header[19] = 0;
	p3header[20] = (bassize) & 0xFF;	/* Size of program */
	p3header[21] = (bassize >> 8) & 0xFF;

	/* Copy TAP header from +3DOS header */
	tapheader[0] = 19;	/* 19 bytes in block */
	tapheader[1] = 0;
	tapheader[2] = 0;	/* Header */
	tapheader[3] = 0;	/* BASIC */
	sprintf((char *)tapheader + 4, "%-10.10s", st_filename);
	memcpy(tapheader + 14, p3header + 16, 6);

	for (sum = 0, n = 2; n < 20; n++) sum ^= tapheader[n];
	tapheader[20] = sum;
	/* Length of BASIC block plus type + checksum */
	tapheader[21] = (bassize + 2) & 0xFF;
	tapheader[22] = (bassize + 2) >> 8;
	tapheader[23] = 0xFF;		/* Block type */

	switch(st_format)
	{
		case TAP:
			outsize = bassize + 25; /* 25 bytes, comprising: */
			break;			/* 17 bytes cassette header */
						/* 4 bytes header overhead */
						/* 4 bytes data overhead */
		case ZXT:
			memcpy(p3header + 15, "TAPEFILE", 8);
			outsize = bassize +128 + 25;
						/* 128 for +3DOS header, */
			break;			/* 25 for TAP overhead */
		case PLUS3DOS:
			outsize = bassize + 128;
			break;
	}	
	p3header[11] = (outsize) & 0xFF;
	p3header[12] = (outsize >>  8) & 0xFF;
	p3header[13] = (outsize >> 16) & 0xFF;
	p3header[14] = (outsize >> 24) & 0xFF;

	for (sum = n = 0; n < 127; n++) sum += p3header[n];
	p3header[127] = sum;

	if (st_format == ZXT || st_format == PLUS3DOS)
	{
		if (fwrite(p3header, 1, sizeof(p3header), outfile) < (int)sizeof(p3header))
			return "Failed to write +3DOS header";
	}
	if (st_format == ZXT || st_format == TAP)
	{
		if (fwrite(tapheader, 1, sizeof(tapheader), outfile) < (int)sizeof(tapheader))
			return "Failed to write tape header";
	}
	sum = 0xFF;
	
	if (writeout(outfile, basic1, sizeof(basic1), &sum))
		return "Failed to write BASIC prefix";

	size -= base;
	if (copyover(outfile, infile, size, &sum))
		return "Failed to copy payload";
	if (writeout(outfile, basic2, sizeof(basic2), &sum))
		return "Failed to write BASIC suffix";
	if (st_format == ZXT || st_format == TAP)
	{
		fputc(sum, outfile);
	}
	return NULL;
}

char *cnv_progname = "bin2bas";

