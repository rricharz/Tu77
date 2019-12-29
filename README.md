# tu77: TU 77 vacuum column magnetic tape drive front panel emulator

Simulation of a TU 77 magnetic tape front panel for SimH (PDP-11). The simulation shows an
open TE-16 at the moment. It will be replaced with a TU-77 once I have a good image of a TU-77.
The TU-77 did of course only work properly if the vacuum columns were closed, but it is much
more intereesting to see how the tape moves in the open columns.

Vacuum column tape drives are described on
[Wikipedia - nine track tapes](https://en.wikipedia.org/wiki/9_track_tape)

For the Raspberry Pi and other Linux systems.

This [video of tu77 demo](https://youtu.be/Ye_s0w6C970) shows a slightly outdated version
of tu77 in action on a PiDP-11.

The TU 57 magnetic tape unit had a very fast tape transportation speed for reads and writes of
3.2 m/s, and a rotation speed of the reels of up to 500 rpm.

A driver to use tu77 with the PiDP-11 is included.

**Hardware requirements**

This TU 77 simulator has been tested on the PiDP-11 using a Raspberry Pi 4B.
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

**Demo of tape write**

There is a small demo program which demonstates reading from unit 0 0. It is also
a test for the communication between a simulated "host" (the demo program in this case)
and tu57.

Start tu77 in background (with the &) using

```
  ./tu77 &
```
 
Make sure that both right switches are in the "remote" position (this is the default).
Then start the demo program with

```
  sudo ./demo
```
 
Sudo might be required if SimH has written to the status byte file before. There is currently no 2 way communication back from the panel to the "host". Therefore, at the moment the demo
just flips the left button to the write enable position if required. But the the right buttons need
to be in the remote position. Otherwise the panel does not listen to a "host".

If you abort the demo program while it's running you might end up with reels spinning forever. Just use

```
 sudo rm /tmp/tu77status
```
 
in this case, or start the demo program again and let it go through the complete demo.


**Installing the proper driver in SimH**

A slightly modified tape driver needs to be installed in SimH. This driver writes the necessary
status bits into a little status file, where it can be read by tu77.

There is currently only one driver for the PDP-11 available:

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

	-unit1		attach to tape unit 1 instead of 0

	-label "text"	show a label on the removeable reel with the specified text


**Versions**

See [versions](versions.txt)


**Contributors**

(Sorry, not yet ready)


**The usual disclaimer**

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
