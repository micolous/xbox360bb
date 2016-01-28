# xbox360bb #

A driver for the Xbox 360 Big Button (aka "Scene It") controllers / receiver.

Copyright 2009 James Mastros <jam...@mastros.biz>

Copyright 2011-2016 Michael Farrell <http://micolous.id.au>

Licensed under the GPLv2+.

## Devices Supported ##

 * `045e:02a0`: Microsoft Corporation Xbox360 Big Button IR

**Note**: this will not give support for the controllers via LIRC, nor does the receiver act as a generic infrared receiver (CIR).  You can only use Xbox 360 Big Button controllers with it.  No other Xbox infrared accessories will function with the receiver.

When a receiver is attached to the computer, it will appear as four joystick devices -- one for each colour controller:

 * Microsoft X-Box 360 Big Button IR green controller
 * Microsoft X-Box 360 Big Button IR red controller
 * Microsoft X-Box 360 Big Button IR blue controller
 * Microsoft X-Box 360 Big Button IR yellow controller

## Kernels supported ##

This should build cleanly on Linux 2.6.32 to 4.2.  It should build on later kernels too (but sometimes will need patching if things change).

I've tested this module on `x86`, `amd64`, and `armhf`.

## Building and installing ##

### Installing kernel headers ###

You should install the kernel headers on your system first.  If you have kernel sources for your running kernel, you can skip this.

On **Debian** this is done with:

	# apt-get install linux-headers-amd64 build-essential

Substitute `amd64` for your kernel's architecture.  This could be something like `i686` or `ppc`.  ARM systems typically have their machine name specified as they rarely use a common kernel.

On **Ubuntu** this is done with:

	# apt-get install linux-headers-generic build-essential

Substitute `generic` with the series of kernel you are using.  Normally this is `generic`, sometimes this is `generic-lts-wily` (if you use a backported kernel from Wily), or `lowlatency`.

### Cloning the repository ###

Then clone the git repository into `/usr/src`:

	# cd /usr/src
	# git clone https://github.com/micolous/xbox360bb.git

Then to build and install the module, run:

	# cd /usr/src/xbox360bb
	# make
	# make install

**Note:** you could clone the repository to another directory, however it is customary to push it to `/usr/src`.

## Loading the module ##

Once you have installed the module, you can activate it with:

	# modprobe -v xbox360bb

If you want to automatically start the module on boot, you can add it to `/etc/modules`, or add the modprobe line to `/etc/rc.local`.

## Using the controller ##

You can use the controllers with any program on Linux that supports joysticks.

You can use the `jstest` program to test the controllers, however be aware that the controller will show up with many buttons, so you will need to widen your terminal.  This is located in the `joystick` package on Debian.

## Other OS support ##

I'm not interested in porting this to non-Linux platforms.

However, because I rank highly in Google Searches and get emails as a result, here are some pointers:

### All platforms (using Linux virtualised) ###

You can also use VirtualBox (or other virtualisers) on a non-Linux host with a Linux guest, sharing the USB receiver to the guest (it's USB product/vendor ID is `045e:02a0`), then loading the `xbox360bb` kernel module in the Linux guest.

Then you can write some script which runs in the Linux guest to send the joystick events from the Big Button over a TCP socket.

This isn't an elegant solution at all.

### All platforms (using the "Jackbox approach") ###

If you're making a quiz-type game using these controllers, you may wish to consider the [Jackbox Games](http://jackboxgames.com/) approach instead.

When you start the game, put a URL (and QR code) on the screen where people can join your game with their smartphone's web browser.

Then, allow people to use their phone to join the game, and have some AJAX or WebSockets and a little JavaScript to render elements on the phone for people to press on the fly, and fire events back to your game.

### OSX support ###

There's no driver for this.

[There is an open source driver for the Xbox 360 gamepads](https://github.com/electric-monk/360Controller).  It needs some modifications to make this work.  Get hacking. :)

### Windows support ###

The Big Button receiver uses very a non-standard interface, using **neither** the Xbox 360 gamepad interface or USB HID.  The standard Microsoft Xbox controller drivers **will not work with this controller**.

There is some support for the controller in [Microsoft's XNA Game Studio SDK](http://xbox.create.msdn.com/en-US/) version 3, however it is only supported on the Xbox 360, not on Windows.  Unlike the Windows SDK, you **also** require an XNA Creators Club paid subscription in order to deploy code to the Xbox 360.

If you want to use a similar controller on Windows, sell your Big Button controller and buy a Playstation Buzz controller instead.  [There is an existing C# library for interfacing with the controllers](https://github.com/bbeardsley/BuzzIO), and they are of a similar form factor (although wired rather than IR), which you can use for quiz-show type games.

