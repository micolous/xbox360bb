/*
 * X-Box 360 big-button controller driver
 *
 * This is for the controllers that come with game-show style games
 * for the xbox 360.  They consist of an IR reciver with a USB cable,
 * marked on the bottom "Microsoft X-Box 360 Big Button IR" (fixme:
 * verify), and four seperate controller devices, each with a
 * different colour scheme: green, red, blue, and yellow (in that
 * order).
 *
 * To the best of my knowledge, these haven't been publicly reverse
 * engineered before.  The protocol is pretty simple, and is explained
 * in the comments below.  It's not terribly far from the normal xbox
 * protocol; there's an additional byte or three of metadata at the
 * beginning, which specifies what controller is talking, and one of
 * the otherwise unused bit is the centre-of-dpad button that normal
 * controllers don't have.
 *
 * What is different from the normal controller is the fact that there
 * are four of them, and that they use the repeat scheme that is
 * popular in the CIR world -- you keep getting reports of the same
 * buttons being held down until they aren't held anymore, and you
 * never get a report that tells you no buttons are held.
 *
 * Unlike a normal xbox360 controller, there are no LEDs (so far as I
 * know; I haven't actually disassembled one, but the game I played
 * with them didn't use it at all, if there is one, and it would have
 * been an obvious enhancement to the gameplay.)
 *
 * Copyright 2009 James Mastros <jam...@mastros.biz>
 * Copyright 2011-2016 Michael Farrell <micolous+lk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * This driver is based on:
 *  - the xpad driver -- general input & usb, some xbox
 *                    -- drivers/input/joystick/xpad.c
 *  - winbond-cir -- repeat handling
 *                -- drivers/input/misc/winbond-cir.c
 *
 * TODO:
 *  - smarter support for repeat.
 *  - Become certain of the correct keycode for the centre button.
 *
 * This is a MODIFIED version of the driver by Michael Farrell
 * <micolous+lk@gmail.com>:
 * - Changed it so that the directional buttons on the top button would act as
 *   send EV_ABS for the ABS_X and ABS_Y axises.  This allows Linux to detect
 *   the device as a joystick in joydev.c.
 * - Build fix for kernel >= 2.6.34 due to function renames
 *   ref: <https://issues.asterisk.org/print_bug_page.php?bug_id=17383>
 * - Build fix for kernel >= 3.2.0 due to module handling differences
 * - Improved code style
 *
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/usb/input.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/export.h>
#include <linux/module.h>

#define DRIVER_DESC "X-Box 360 Big Button (Scene It) driver"

/* No idea where this came from, it's copped from xpad.c */
#define XBOX360BB_PKT_LEN 32

/*
 * This might be overkill at this point.  I'm honestly not really
 * sure.
 */
static const struct xbox360bb_dev_options {
	u16 idVendor;
	u16 idProduct;
	char *name;
} xbox360bb_dev_options[] = {
	{ 0x045e, 0x02a0, "Microsoft X-Box 360 Big Button IR" },
	{ },
};

static const signed short xbox360bb_abs[] = {
	ABS_X,
	ABS_Y,
	-1
};

static const signed short xbox360bb_btn[] = {
	/* Byte 2 (zero-based) or report, MSB to LSB */
	/* 0x80 and 0x40 are unused */
	BTN_BACK,   /* 0x20 */
	BTN_START,  /* 0x10 */
	/* Right       0x08 */
	/* Left        0x04 */
	/* Down        0x02 */
	/* Up          0x01 */

	/* Byte 3 (zero-based) of report, MSB to LSB */
	BTN_Y,      /* 0x80 */
	BTN_X,      /* 0x40 */
	BTN_B,      /* 0x20 */
	BTN_A,      /* 0x10 */
	/*
	 * this bit isn't used for normal xbox controllers -- on a
	 * normal controller, you simply can't hit all the directions at
	 * the same time, and on a dance controller, you can only if
	 * you have four feet.
	 * There's no clear button defintion for this use, so we use
	 * thumbr.
	 * In any case, it's byte 4, 0x08.
	 */
	BTN_THUMBR, /* 0x08 */
	/* The big X, logo button.  xpad uses BTN_MODE. */
	BTN_MODE,   /* 0x04 */
	/* 0x02 and 0x01 seem to be unused. */
	/* end marker */
	-1
};

/*
 * Xbox 360 has a vendor-specific class, so we cannot match it with only
 * USB_INTERFACE_INFO (also specifically refused by USB subsystem), so we
 * match against vendor id as well. Wired Xbox 360 devices have protocol 1,
 * wireless controllers have protocol 129, and big button controllers
 * have protocol 4.
 */
#define XBOX360BB_VENDOR_PROTOCOL(vend, pr) \
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR | \
		       USB_DEVICE_ID_MATCH_INT_INFO, \
	.idVendor = (vend), \
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC, \
	.bInterfaceSubClass = 93, \
	.bInterfaceProtocol = (pr)
#define XBOX360BB_VENDOR(vend) \
	{ XBOX360BB_VENDOR_PROTOCOL(vend, 4) }

static const char * const xbox360bb_controller_colors[] = {
	" green controller",
	" red controller",
	" blue controller",
	" yellow controller"
};




/* Circular dependency */
struct xbox360bb;

struct xbox360bb_controller {
	/* Points back to the parent usb_xbox360bb struct */
	struct xbox360bb *receiver;
	/* 0..3 */
	int controller_number;
	struct input_dev *idev;
	unsigned char last_report[2];
	struct timer_list timer_keyup;
};

struct xbox360bb {
	/* USB side */
	struct usb_device *udev;	/* usb device */
	struct urb *irq_in;		/* urb for interrupt in report */
	unsigned char *raw_data;	/* input data */
	/* TODO: rename to raw_data_dma */
	dma_addr_t idata_dma;

	/* input side */

	/* the number of input devices which are open.  Should always
	   be between 0 and 4, and FIXME should have locking? */
	int idev_open_count;

	/* These are in a sub-struct per controller, so that we have
	 * something nice to point to as user data in the callback
	 * functions that lets us know which controller we are
	 * handling in a nice, clean manner.  OTOH, directly embedding
	 * the struct lets us not worry about that messy allocation
	 * stuff
	 */
	struct xbox360bb_controller controller[4];
};

/* xbox360bb_keydown
 *
 * Note that a key has gone down, or is still down.
 * While currently this is quite simple, it should shortly become
 * quite a bit more complex.  FIXME: Take code for this from
 * drivers/input/misc/winbond-cir.c, bcir_keyup & such.
 *
 *
 */
static void xbox360bb_keydown(struct xbox360bb_controller *controller,
			      int button, int val)
{
	input_report_key(controller->idev, button, val);
}

/* xbox360bb_keyup
 *
 * This is the timer callback function.  If we reach this, that means
 * we haven't had a report at all for this controller in some time, so
 * we should consider *all* keys that it had down as up.
 */
static void xbox360bb_keyup(struct timer_list * t)
{
	int i;
	struct xbox360bb_controller *controller =
		(struct xbox360bb_controller *)from_timer(controller, t, timer_keyup);

	pr_info("timer callback for controller %d\n",
		controller->controller_number);

	/* FIXME: Is there a quick, simple way to keyup all currently
	 * down keys at once? */
	for (i = 0; xbox360bb_btn[i] >= 0; i++)
		input_report_key(controller->idev, xbox360bb_btn[i], 0);

	/* Also reset the X and Y axis */
	for (i = 0; xbox360bb_abs[i] >= 0; i++)
		input_report_abs(controller->idev, xbox360bb_abs[i], 0);

	input_sync(controller->idev);
	controller->last_report[0] = 0;
	controller->last_report[1] = 0;
}

/*
 * xbox360bb_usb_process_packet
 *
 * Given the actual payload of a packet, make it into events.  This is
 * the actual core of this module; everything else is plumbing.
 */
static void xbox360bb_usb_process_packet(struct xbox360bb *xbox360bb, u16 cmd,
					 unsigned char *data)
{
	/* Byte 2 of the input is what controller we've got, zero
	 * based: green, red, blue, yellow. */
	struct xbox360bb_controller *controller;
	signed int x = 0;
	signed int y = 0;

	if (data[2] > 3) {
		pr_warn("Argh, xbox360bb controller number out of range: %d",
			data[2]);
		/* Should we stop processing completely, rather then
		just this report?  Depends on how severe the error is,
		and I currently have no way of telling.  There's
		unlikely to be worse results then just the driver
		hitting random buttons -- this is the only place where
		we use input from the controller as an index in kernel
		space. On the other hand, what controller do we
		attribute the report to? */
		return;
	}
	controller = &(xbox360bb->controller[data[2]]);

	pr_debug("%d ms currently remaining on timer\n",
		 jiffies_to_msecs(controller->timer_keyup.expires-jiffies));

	/* Arm the timer (or move it forward).  In a quick test, 188
	 * ms is longest between two reports.  250 should give us
	 * plenty of time for highly loaded systems. */
	mod_timer(&controller->timer_keyup, jiffies + msecs_to_jiffies(250));

	/* Short-circuit further processing if this report is the same
	 * as the previous one.  Just a cheap way of saving CPU time,
	 * mostly. */
	if (controller->last_report[0] == data[3] &&
	    controller->last_report[1] == data[4]) {
		pr_debug("Ignoring duplicate report\n");
		return;
	}
	controller->last_report[0] = data[3];
	controller->last_report[1] = data[4];

	/* dpad as absolute axis... */
	y = (data[3] & 0x01) ? -1  : y;
	y = (data[3] & 0x02) ? 1 : y;
	x = (data[3] & 0x04) ? -1  : x;
	x = (data[3] & 0x08) ? 1 : x;
	input_report_abs(controller->idev, ABS_X, x);
	input_report_abs(controller->idev, ABS_Y, y);

	/* start/back buttons */
	xbox360bb_keydown(controller, BTN_START,  data[3] & 0x10);
	xbox360bb_keydown(controller, BTN_BACK,   data[3] & 0x20);
	xbox360bb_keydown(controller, BTN_MODE,   data[4] & 0x04);

	/* This is the only button that is rather novel vs the normal
	 * controller.  The corsponding bit for the normal controllers
	 * isn't used, as far as I can see.  It's like the thumb
	 * buttons, so call it the right one, randomly.
	 * (Normally the thumb button goes with an analog stick, not a dpad.)
	 */
	xbox360bb_keydown(controller, BTN_THUMBR, data[4] & 0x08);


	/* buttons A,B,X,Y,TL,TR and MODE */
	xbox360bb_keydown(controller, BTN_A,	  data[4] & 0x10);
	xbox360bb_keydown(controller, BTN_B,	  data[4] & 0x20);
	xbox360bb_keydown(controller, BTN_X,	  data[4] & 0x40);
	xbox360bb_keydown(controller, BTN_Y,	  data[4] & 0x80);

	input_sync(controller->idev);
}

static void xbox360bb_usb_irq_in(struct urb *urb)
{
	struct xbox360bb *xbox360bb = urb->context;
	int retval, status;

	status = urb->status;


	switch (status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		pr_debug("%s - urb shutting down with status: %d",
			 __func__, status);
		return;
	default:
		pr_debug("%s - nonzero urb status received: %d",
			 __func__, status);
		goto exit;
	}

	xbox360bb_usb_process_packet(xbox360bb, 0, xbox360bb->raw_data);

exit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		pr_err("%s - usb_submit_urb failed with result %d",
		       __func__, retval);
}

/* Input side device opened.  We'll never see two overlapping opens
 * for the same controller; the kernel handles the multiplexing for
 * us.
 */
static int xbox360bb_input_open(struct input_dev *idev)
{
	struct xbox360bb_controller *controller = input_get_drvdata(idev);
	struct xbox360bb *xbox360bb = controller->receiver;
	int error;

	if (xbox360bb->idev_open_count == 0) {
		pr_debug("In open, submitting URB\n");
		error = usb_submit_urb(xbox360bb->irq_in, GFP_KERNEL);
		if (error) {
			pr_warn("...error = %d\n", error);
			return -EIO;
		}
		pr_debug("...passed\n");
	} else {
		pr_debug("Not trying to submit URB; it should already be running (idev_open_count=%d)\n",
			xbox360bb->idev_open_count);
	}
	xbox360bb->idev_open_count++;
	if (xbox360bb->idev_open_count > 4)
		pr_warn("idev_open_count=%d is out of range after open\n",
			xbox360bb->idev_open_count);

	return 0;
}

static void xbox360bb_input_close(struct input_dev *idev)
{
	struct xbox360bb_controller *controller = input_get_drvdata(idev);
	struct xbox360bb *xbox360bb = controller->receiver;

	if (xbox360bb->idev_open_count == 1) {
		pr_debug("In close, killing urb\n");
		usb_kill_urb(xbox360bb->irq_in);
	} else {
		pr_debug("In close, not killing urb, open count=%d\n",
			 xbox360bb->idev_open_count);
	}
	xbox360bb->idev_open_count--;
	if (xbox360bb->idev_open_count < 0)
		pr_warn("Argh, idev_open_count less then 0: %d\n",
			xbox360bb->idev_open_count);
}

static int xbox360bb_usb_probe(struct usb_interface *intf,
			       const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct xbox360bb *xbox360bb;
	struct input_dev *input_dev;
	struct usb_endpoint_descriptor *ep_irq_in;
	const struct xbox360bb_dev_options *dev_options = NULL;

	int options_i;
	int controller_i;
	int btn_i, abs_i;
	int error = -ENOMEM;

	pr_info("xbox360bb_usb_probe vendor=0x%x, product=0x%x\n",
		le16_to_cpu(udev->descriptor.idVendor),
		le16_to_cpu(udev->descriptor.idProduct));

	/* Find the xbox360bb_device entry for this one.  (FIXME:
	   Should we get rid of this?  We only have one, presently,
	   and no options we need to look up in here anyway.) */
	for (options_i = 0;
	     xbox360bb_dev_options[options_i].idVendor;
	     options_i++) {
		dev_options = &xbox360bb_dev_options[options_i];
		if ((le16_to_cpu(udev->descriptor.idVendor) ==
		     dev_options->idVendor) &&
		    (le16_to_cpu(udev->descriptor.idProduct) ==
		     dev_options->idProduct))
			break;
	}

	xbox360bb = kzalloc(sizeof(struct xbox360bb), GFP_KERNEL);
	if (!xbox360bb)
		goto fail1;

	/* Init the USB stuff */
	xbox360bb->udev = udev;

	xbox360bb->raw_data = usb_alloc_coherent(udev, XBOX360BB_PKT_LEN,
						 GFP_KERNEL,
						 &xbox360bb->idata_dma);

	if (!xbox360bb->raw_data)
		goto fail2;

	xbox360bb->irq_in = usb_alloc_urb(0, GFP_KERNEL);
	if (!xbox360bb->irq_in)
		goto fail3;

	ep_irq_in = &intf->cur_altsetting->endpoint[0].desc;
	usb_fill_int_urb(xbox360bb->irq_in, udev,
			 usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
			 xbox360bb->raw_data, XBOX360BB_PKT_LEN,
			 xbox360bb_usb_irq_in,
			 xbox360bb, ep_irq_in->bInterval);
	xbox360bb->irq_in->transfer_dma = xbox360bb->idata_dma;
	xbox360bb->irq_in->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	xbox360bb->irq_in->dev = xbox360bb->udev;

	usb_set_intfdata(intf, xbox360bb);

	/* Init the input stuff */
	for (controller_i = 0; controller_i < 4; controller_i++) {
		int name_size;
		/* input_dev name/phys are const char *, so we need
		 * to warn all over, or use a temporary.  */
		char *name;
		char *phys;
		struct xbox360bb_controller *controller =
			&(xbox360bb->controller[controller_i]);

		pr_info("making input dev %d\n", controller_i);

		controller->controller_number = controller_i;
		controller->receiver = xbox360bb;

		timer_setup(&(controller->timer_keyup), xbox360bb_keyup, 0);

		input_dev = input_allocate_device();
		if (!input_dev)
			goto fail4;
		controller->idev = input_dev;

		name_size = strlen(dev_options->name) +
			    strlen(xbox360bb_controller_colors[controller_i]) +
			    1;
		name = kzalloc(name_size, GFP_KERNEL);
		if (!name)
			goto fail4;

		/* Don't bother checking returns, we explicitly sized
		 * input_dev->name so it will fit, and the worst that
		 * will happen with str*l*(cpy|cat) is truncation.
		 */
		strlcpy(name, dev_options->name, name_size);
		strlcat(name, xbox360bb_controller_colors[controller_i],
			name_size);

		pr_info("... name='%s'\n", name);
		input_dev->name = name;

		/* Right, now need to do the same with phys, more or less. */
		/* 64 is taken from xpad.c.  I hope it's long enough. */
		phys = kzalloc(64, GFP_KERNEL);
		if (!phys)
			goto fail4;
		usb_make_path(udev, phys, 64);
		snprintf(phys, 64, "%s/input%d", phys, controller_i);
		pr_info("... phys='%s'\n", phys);
		input_dev->phys = phys;

		/* Static data */
		input_dev->dev.parent = &intf->dev;
		pr_info("... input_set_drvdata\n");
		input_set_drvdata(input_dev, controller);
		/* Set the input device vendor/product/version from
		   the usb ones. */
		usb_to_input_id(udev, &input_dev->id);
		input_dev->open  = xbox360bb_input_open;
		input_dev->close = xbox360bb_input_close;

		/* This is probably horribly inefficent, but it's
		   one-time init code.  Keep it easy to read, until
		   profiling says it's *actually* a problem. */
		input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
		for (btn_i = 0; xbox360bb_btn[btn_i] >= 0; btn_i++)
			set_bit(xbox360bb_btn[btn_i], input_dev->keybit);

		for (abs_i = 0; xbox360bb_abs[abs_i] >= 0; abs_i++) {
			set_bit(xbox360bb_abs[abs_i], input_dev->absbit);
			input_set_abs_params(input_dev, xbox360bb_abs[abs_i],
					     -1, 1, 0, 0);
		}

		pr_info("... input_register_device\n");
		error = input_register_device(input_dev);
		pr_info("returned from input_register_device, error=%d\n",
			error);

		if (error)
			goto fail4;
	}

	return 0;

	/* FIXME: We currently leak in failure cases numbered above
	 * fail3. */
fail4:
	pr_warn("Aaargh, hit fail4!\n");
	/* need to check which bits of the input stuff have been
	 * allocated, because it's all loopy. */
fail3:
	usb_free_urb(xbox360bb->irq_in);
fail2:
	usb_free_coherent(udev, XBOX360BB_PKT_LEN, xbox360bb->raw_data,
		xbox360bb->idata_dma);
fail1:
	return error;
}

static void xbox360bb_usb_disconnect(struct usb_interface *intf)
{
	struct xbox360bb *xbox360bb = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	if (!xbox360bb)
		return;

	input_unregister_device(xbox360bb->controller[0].idev);
	input_unregister_device(xbox360bb->controller[1].idev);
	input_unregister_device(xbox360bb->controller[2].idev);
	input_unregister_device(xbox360bb->controller[3].idev);
	/* FIXME: free the timers? */
	usb_free_urb(xbox360bb->irq_in);
	usb_free_coherent(xbox360bb->udev, XBOX360BB_PKT_LEN,
			xbox360bb->raw_data, xbox360bb->idata_dma);
	kfree(xbox360bb);
}

static struct usb_device_id xbox360bb_usb_table[] = {
	XBOX360BB_VENDOR(0x045e),	/* Microsoft X-Box 360 controllers */
	{}
};

MODULE_DEVICE_TABLE(usb, xbox360bb_usb_table);

static struct usb_driver xbox360bb_usb_driver = {
	.name       = "xbox360bb",
	.probe      = xbox360bb_usb_probe,
	.disconnect = xbox360bb_usb_disconnect,
	.id_table   = xbox360bb_usb_table
};

static int __init xbox360bb_usb_init(void)
{
	int result = usb_register(&xbox360bb_usb_driver);

	if (result == 0)
		pr_info(DRIVER_DESC "\n");

	return result;
}

static void __exit xbox360bb_usb_exit(void)
{
	usb_deregister(&xbox360bb_usb_driver);
}

module_init(xbox360bb_usb_init);
module_exit(xbox360bb_usb_exit);

MODULE_AUTHOR("Michael Farrell <micolous+lk@gmail.com>");
MODULE_AUTHOR("James Mastros <jam...@mastros.biz>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
