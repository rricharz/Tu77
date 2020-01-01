# TU77 magnetic tape front panel emulator

Simulation of a TU77 and a Te16 magnetic tape front panel for SimH (PDP-11).
The TU77 and the Te16 did of course only work properly if the vacuum columns
were closed, but it is much more interesting to see how the tape moves in the
open vacuum columns.

![tu77 front panel](Tu77-front.jpg?raw=true "tu77 front panel")
![te16 front panel](Te16-front.jpg?raw=true "te16 front panel")

Vacuum column tape drives are described on
[Wikipedia - nine track tapes](https://en.wikipedia.org/wiki/9_track_tape).

For the Raspberry Pi and other Linux systems.

[Video of a TU77 demo](https://youtu.be/ilRWveCcB44) showing tu77 in action on a PiDP-11.
[Video of a TE16 demo](https://youtu.be/8GL19xPuenI) showing te16 in action on a PiDP-11.

The TU77 magnetic tape unit had a very fast tape transportation speed for reads and writes of
3.2 m/s, and a rotation speed of the reels of up to 500 rpm.

A driver to use tu77 and te16 with the PiDP-11 is included.

**Hardware requirements**

This TU77 and TE16 simulator has been tested on the PiDP-11 using a Raspberry Pi 4B.
A considerable effort has been made to reduce the CPU usage to acceptable levels. It
should therefore also work on Raspberry PI 3B  and Raspberry Pi 3B+ models. Please send
your positive or negative feedback on using tu77 on Raspberry Pi 3 models to
rricharz77@gmail.com, so that I can update these requirements.

Slower Raspberry Pi models are not recommended.


**Download the program**

I recommend using "git clone" to download the program. This allows for very easy upgrades.

```
  sudo apt-get update
  sudo apt-get upgrade
  sudo apt-get install git
  cd
  git clone --depth 1 git://github.com/rricharz/Tu77
  cd Tu77
```

To upgrade the program later, type

```
  cd
  cd Tu77
  git pull
```

If you want to recompile the program, proceed as follows

```
  cd
  cd Tu77
  sudo apt-get install libgtk-3-dev
  touch tu77.c
  make
```

Start the program with

```
  cd
  cd Tu77
  
  ./tu77
```

The following keys can be used to exit from tu77 and close the corresponding window. These keys are
especially useful in full screen mode where no close window button is available.

 - ESC
 - ctrl-C
 - ctrl-Q

**Buttons**

The only active button is the "ONLINE" button. If you toggle this button in the offline
position, the online light will turn off and the drive will not listen to SimH anymore.

**Demo of tape write**

There is a small demo program which demonstates reading from tape. It is also
a test for the communication between a simulated "host" (the demo program in this case)
and tu77.

Start tu77 in background (with the &) using

```
  ./tu77 &
```
 
Start the demo program with

```
  sudo ./demo
```
 
Sudo might be required if SimH has written to the status byte file before.

If you abort the demo program while it's running you might end up with reels spinning forever. Just use

```
 sudo rm /tmp/tu56status
```

Note: The status file is called tu56status, because it works both for the tu77 magtape
front panel emulator and the tu56 DECtape front panel emulator.

**Simulating a TE16**

There is also an emulation of a TE16 tape drive including. Start the TE16 emulation with

```
 ./te16
```


**Installing the proper driver in SimH**

A slightly modified tape driver needs to be installed in SimH. This driver writes the necessary
status bits into a little status file, where it can be read by tu77.

There is currently only a driver for the PDP-11 available:

 - magtape driver for the PiDP-11

**Instructions for different operating systems**

 - [Using tu77 with 2.11 BSD on the PiDP-11](bsd.txt)

Improved instructions are very much appreciated. Please send improved versions of these
instructions, and instructions for other operating systems to rricharz77@gmail.com

**Options of the command tu77**

tu77 has the following options:

	-full		start tu77 in a full screen window
                        
	-fullv          start tu77 in a decorated window using the maximal vertical space
			available.

	-unit1		attach to tape unit 1 instead of unit 0

	-label "text"	show a blue label on the removeable reel with the specified text


**Versions**

See [versions](versions.txt)


**Contributors**

Johnny Billquist, Neal G., Terry Kennedy and Jonathan Morton have helped to understand
vacuum magnet tape motions. The pictures of the open TU77 have been obtained from
Henk Gooijen. The picture of the open TE16 has been obtained from
https://commons.wikimedia.org/wiki/File:9-track-drive-open.jpg#mw-jump-to-license.
Thanks to all others who have helped to develop this software.


**The usual disclaimer**

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
