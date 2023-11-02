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

This should build cleanly on Linux 3.2.0 to 4.16.  It should build on later kernels too (but sometimes will need patching if things change).

If you want compatibility with kernels as early as Linux 2.6.32, please check out the `2.6.32-compat` tag from the [source repository](https://github.com/micolous/xbox360bb).  This really isn't recommended.

I've tested this module on `x86`, `amd64`, and `armhf`.

## Building and installing ##

You'll need to run all these commands as `root`.

If you're on a system using `sudo` (like **Raspbian** or **Ubuntu**), run
`sudo -i` first to become `root`.

The `bash` prompt changes from a `$` to `#` when you run as `root`:

```
user@pc:~$ sudo -i
[sudo] password for user: (hidden)
root@pc:~#
```

### Installing kernel headers and build tools

You'll need the kernel headers for your currently running kernel and build
tools (`make` and the compiler) on your system first.

If you have built your currently-running kernel from source, you can skip
this.

* On **Debian** this is done with:

  ```sh
  apt install linux-header-amd64 build-essential
  ```

  Substitute `amd64` for your kernel's architecture.  This could be
  something like `i686` or `ppc`.  ARM systems typically have their machine
  name specified as they rarely use a common kernel.

* [On **Raspbian** / **Raspberry PI OS**, see their instructions for
  installing kernel headers][rpi].

  Then install build tools with:

  ```sh
  apt install build-essential
  ```

* On **Ubuntu** this is done with:

  ```sh
  apt install linux-headers-generic build-essential
  ```

  Substitute `generic` with the series of kernel you are using.  Normally
  this is `generic`, sometimes this is `generic-lts-wily` (if you use a
  backported kernel from Wily), or `lowlatency`.

[rpi]: https://www.raspberrypi.com/documentation/computers/linux_kernel.html#kernel-headers

### Cloning the repository

Then clone the git repository into `/usr/src`:

	# cd /usr/src
	# git clone https://github.com/micolous/xbox360bb.git

**Note:** you could clone the repository to another directory, however it is customary to push it to `/usr/src`.

### Building and installing ###

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

[There is an open source driver for the Xbox 360 gamepads](https://github.com/360Controller/360Controller).  It needs some modifications to make this work.  Get hacking. :)

### Windows support ###

The Big Button receiver uses very a non-standard interface, using **neither** the Xbox 360 gamepad interface or USB HID.  The standard Microsoft Xbox controller drivers **will not work with this controller**.

There is some support for the controller in [Microsoft's XNA Game Studio SDK](http://xbox.create.msdn.com/en-US/) version 3, however it is only supported on the Xbox 360, not on Windows.  Unlike the Windows SDK, you **also** require an XNA Creators Club paid subscription in order to deploy code to the Xbox 360.

If you want to use a similar controller on Windows, sell your Big Button controller and buy a Playstation Buzz controller instead.  [There is an existing C# library for interfacing with the controllers](https://github.com/bbeardsley/BuzzIO), and they are of a similar form factor (although wired rather than IR), which you can use for quiz-show type games.

