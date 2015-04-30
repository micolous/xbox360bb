# xbox360bb #

A driver for the Xbox 360 Big Button (aka "Scene It") controllers / receiver.

Copyright 2009 James Mastros <jam...@mastros.biz>

Copyright 2011-2012 Michael Farrell <http://micolous.id.au>

Licensed under the GPLv2+.

## Devices Supported ##

 * `045e:02a0`: Microsoft Corporation Xbox360 Big Button IR

**Note**: this will not give support for the controllers via LIRC, nor does the receiver act as a generic infrared receiver (CIR).  You can only use Xbox 360 Big Button controllers with it.

When a receiver is attached to the computer, it will appear as four joystick devices -- one for each colour controller:

 * Microsoft X-Box 360 Big Button IR green controller
 * Microsoft X-Box 360 Big Button IR red controller
 * Microsoft X-Box 360 Big Button IR blue controller
 * Microsoft X-Box 360 Big Button IR yellow controller

## Kernels supported ##

This should build cleanly on Linux 2.6.32 to 3.16.  It should build on later kernels too (but sometimes will need patching if things change).

I've tested this module on `x86`, `amd64`, and `armhf`.

## Building and installing ##

You should install the kernel headers on your system first.  If you have kernel sources for your running kernel, you can skip this.

On Debian this is done with:

	# apt-get install linux-headers-amd64 build-essential

Substitute `amd64` for your kernel's architecture.  This could be something like `i686` or `ppc`.  ARM systems typically have their machine name specified if they use a funny kernel.

Then clone the git repository into `/usr/src`:

	# cd /usr/src
	# git clone https://github.com/micolous/xbox360bb.git

Then to build and install the module, run:

	# cd /usr/src/xbox360bb
	# make
	# make install

## Loading the module ##

Once you have installed the module, you can activate it with:

	# modprobe -v xbox360bb
	
If you want to automatically start the module on boot, you can add it to `/etc/modules`, or add the modprobe line to `/etc/rc.local`.

## Using the controller ##

You can use the controllers with any program on Linux that supports joysticks.

You can use the `jstest` program to test the controllers, however be aware that the controller will show up with many buttons, so you will need to widen your terminal.  This is located in the `joystick` package on Debian.

## Windows support ##

I looked in to writing a Windows driver for this controller.  Unfortunately my lack of knowledge of Windows driver development meant that I found this too difficult, and I got nowhere with it.

Unfortunately Windows driver developers very rarely release any source code for their drivers, and the Big Button receiver uses very a non-standard interface, using neither the Xbox 360 gamepad interface or USB HID.

There is some support for the controller in Microsoft's XNA game development toolkit from version 3, however it is only supported on the Xbox 360, not on Windows.

You can also use VirtualBox on a Windows host with a Linux guest, sharing the USB receiver to the guest (it's USB product/vendor ID is `045e:02a0`), then loading the `xbox360bb` kernel module in the Linux guest.

It may be possible to write a userspace driver for the controller in Windows based on `libusb`.  I'd imagine that `libusb` is similar to Linux's USB kernel interface, so porting it may be possible.  However this solution is far from ideal, and I haven't investigated this further.

My suggestion: [tell Microsoft how you feel about this omission](http://forums.create.msdn.com/forums/t/5485.aspx?PageIndex=1). :)

