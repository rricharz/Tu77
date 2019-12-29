/*
 * demo.c
 *
 * Runs a demo on the tu56 visual display of the DECtape TU56 front panel
 * 
 * for the Raspberry Pi and other Linux systems
 * 
 * Copyright 2019  rricharz
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TSTATE_ONLINE		 1		// Turns the online light on
#define TSTATE_DRIVE1		 2		// Selects drive 0 or 1
#define TSTATE_BACKWARDS	 4		// Sets the direction
#define TSTATE_SEEK		 	 8		// Spins the reels
#define TSTATE_READ			16		// Spins the reels
#define TSTATE_WRITE		32		// Turns the write light on and spins the reels

static FILE *statusFile = 0;

void setStatus(int status, long position)
// set the status bits in the status file
// if status file is accessible
{
	
	char *fname = "/tmp/tu56status";
	
	if (statusFile == 0) {
		statusFile = fopen(fname, "w");
	}

	if (statusFile != 0) {
		fseek(statusFile,0L,SEEK_SET);
		// putc(32 + status,statusFile);
		fprintf(statusFile, "%c%ld\n", 32 + status, position);
		fflush(statusFile);
	}
}

int main(int argc, char **argv)
{	
	long pos;
	pos = 0;
	setStatus(0, 0);
	for (int i = 1; i < 100; i++) {
		setStatus(TSTATE_ONLINE, pos);
		usleep(1000000);
		setStatus(TSTATE_ONLINE | TSTATE_READ, pos);
		usleep(400000);
		pos +=  10000;
	}
	setStatus(0, 0);
	fclose(statusFile);	
	return 0;
}

