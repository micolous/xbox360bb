= xbox360bb =

A driver for the Xbox 360 Big Button controllers / receiver.

Copyright 2009 James Mastros <jam...@mastros.biz>

Copyright 2011-2012 Michael Farrell <http://micolous.id.au>

Licensed under the GPLv2+.

== Devices Supported ==

 * `045e:02a0`: Microsoft Corporation Xbox360 Big Button IR

*Note*: this will not give support for the controllers via LIRC, nor does the reciever act as a generic infrared reciever (CIR).  You can only use Xbox 360 Big Button controllers with it.

When a receiver is attached to the computer, it will appear as four joystick devices -- one for each colour controller:

 * Microsoft X-Box 360 Big Button IR green controller
 * Microsoft X-Box 360 Big Button IR red controller
 * Microsoft X-Box 360 Big Button IR blue controller
 * Microsoft X-Box 360 Big Button IR yellow controller

== Building and installing ==

You should install the kernel headers on your system first.  If you have kernel sources for your running kernel, you can skip this.

On Debian this is done with:

    # apt-get install linux-headers-amd64 build-essential

Substitute `amd64` for your kernel's architecture.  This could be something like `i686` or `ppc`.

Then clone the git repository into `/usr/src`:

    # cd /usr/src
	# git clone git://github.com/micolous/xbox360bb.git

Then to build and install the module, run:

    # cd /usr/src/xbox360bb
	# make
	# make install

== Loading the module ==

Once you have installed the module, you can activate it with:

    # modprobe -v xbox360bb
	
If you want to automatically start the module on boot, you can add it to `/etc/modules`, or add the modprobe line to `/etc/rc.local`.

== Using the controller ==

You can use the controllers with any program on Linux that supports joysticks.

You can use the `jstest` program to test the controllers, however be aware that the controller will show up with many buttons, so you will need to widen your terminal.
	