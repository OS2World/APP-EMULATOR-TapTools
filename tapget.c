/************************************************************************

    TAPTOOLS v1.0.3 - Tapefile manipulation utilities

    Copyright (C) 1996, 2005  John Elliott <jce@seasip.demon.co.uk>

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

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	"taputil.h"

byte header1[128], header2[128];
int is_zxt = 0;
long taplen;

char **args;
int argn;

char *match(unsigned char *name)
{
	int n, m, mt;
	static char name2[12];

	for (n = 2; n < argn; n++)
	{
		sprintf(name2, "%-10.10s", args[n]);
		mt = 1;
	
		for (m = 0; m < 10; m++)
		{
			if (name2[m] == '*')	/* "*" is a wildcard */
			{
				name2[m+1] = '*';
				name2[m]   = name[m];
			}
			if (name2[m] == '?')	/* "?" is a wildcard */
			{
				name2[m]   = name[m];
			}
			if (name2[m] != name[m]) 
			{
				mt = 0; break;
			}
		}
		if (!mt) continue;
		name2[10] = 0;
		for (m = 9; m >= 0; m--)
		{
			if (name2[m] == ' ') name2[m] = 0; 
			else if (name2[m] != 0) break;
		}
		return name2;
	}	
	return NULL;
}






void writeblk(FILE *f, char *name, byte *fhdr, int filelen)
{
	unsigned long zxlen, p3len;
	byte sum = 0;
	int n;
	FILE *fpout;

	memset(header2, 0, 128);
	memcpy(header2, "PLUS3DOS\032\01\00", 11);

	zxlen = fhdr[11] + 256 * fhdr[12];		
	p3len = filelen + 128;

	header2[11] = (p3len)       & 0xFF;
	header2[12] = (p3len >> 8)  & 0xFF;
	header2[13] = (p3len >> 16) & 0xFF;
	header2[14] = (p3len >> 24) & 0xFF;

	header2[15] = fhdr[0];
	memcpy(header2 + 16, fhdr + 11, 7);

	for (n = 0; n < 127; n++) sum += header2[n];

	header2[127] = sum;

	fpout = fopen(name, "wb");
	if (!fpout)
	{
                fprintf(stderr,"File creation failed for %s.\n",name);
                return;
	}	
	if (fwrite(header2,1,128,fpout) < 128)
	{
		fprintf(stderr,"Header write failed for %s.\n",name);
		fclose(fpout);
		return;
	}

/* Header written, do the data */

	for (n=0; n < filelen; n++)
	{
		byte c;

		c=fgetc(f);

		if (feof(f) || ferror (f))
		{
			fprintf(stderr,"Unexpected End-Of-File or error on TAP.");
			fclose(fpout);
			return;
		}

		if (fputc(c,fpout) == EOF)
		{
			fprintf(stderr,"Write fail while copying %s.\n",name);
			fclose(fpout);
			return;
		}

	}
	fclose(fpout);

	fgetc(f);	/* Skip checkbittoggle */
	}





int main(int argc, char **argv)
{
	FILE *tapfile;
	char *tname, *xname;
	int n;

	argn = argc;
	args = argv;
	
	if (argc < 2)
	{
		fprintf(stderr,"Syntax: TAPGET tapfile member member ... \n");
		exit(1);
	}
	tname=argv[1];
	n=2;
	
	/* Check for a .ZXT file */

	tapfile = opentap(tname, "rb", NULL, &taplen);
	/* Start extracting the blocks */
	while (!feof(tapfile))
	{
		byte blkhdr[3];
		byte taphdr[18];

		if (fread(blkhdr, 1, 3, tapfile) < 3) break;

		if ((blkhdr[0] != 19) || (blkhdr[1] != 0) || (blkhdr[2] != 0))
		{
			int len = blkhdr[1] * 256 + blkhdr[0];

			len--;

			if (len) fseek(tapfile, len, SEEK_CUR);

			continue;	
		}
		if (fread(taphdr, 1, 18, tapfile) < 18) break;

		switch(taphdr[0])
		{
			case 0: printf("Program: "); 	   break;
			case 1: printf("Number array: ");  break;
			case 2: printf("Character array:");break;
			case 3: printf("Bytes: ");         break;
			default:printf("Data: ");	   break;
		}
		printf("%-10.10s", taphdr + 1);	

		if (fread(blkhdr, 1, 3, tapfile) < 3) break; 
		
		if ((xname = match(taphdr + 1)))
		{
			int len = blkhdr[1] * 256 + blkhdr[0];
			len -= 2;	
			writeblk(tapfile, xname, taphdr, len);
			printf("  - extracted\n");
		}
		else 
                {
                        int len = blkhdr[1] * 256 + blkhdr[0];

                        len--;

                        if (len) fseek(tapfile, len, SEEK_CUR);
			printf("\n");
                }
 	

	}
	fclose(tapfile);
	return 0;
}

