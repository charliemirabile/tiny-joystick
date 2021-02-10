/* Mini MIDI Pitchbend Joystick
 * mitxela.com/projects/tiny_joystick
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "usbdrv.h"



/* ------------------------------------------------------------------------- */

const PROGMEM char deviceDescrMIDI[] = {	/* USB device descriptor */
	18,			/* sizeof(usbDescriptorDevice): length of descriptor in bytes */
	USBDESCR_DEVICE,	/* descriptor type */
	0x10, 0x01,		/* USB version supported */
	0,			/* device class: defined at interface level */
	0,			/* subclass */
	0,			/* protocol */
	8,			/* max packet size */
	USB_CFG_VENDOR_ID,	/* 2 bytes */
	USB_CFG_DEVICE_ID,	/* 2 bytes */
	USB_CFG_DEVICE_VERSION,	/* 2 bytes */
	1,			/* manufacturer string index */
	2,			/* product string index */
	0,			/* serial number string index */
	1,			/* number of configurations */
};

// B.2 Configuration Descriptor
const PROGMEM char configDescrMIDI[] = {	/* USB configuration descriptor */
	9,			/* sizeof(usbDescrConfig): length of descriptor in bytes */
	USBDESCR_CONFIG,	/* descriptor type */
	101, 0,			/* total length of data returned (including inlined descriptors) */
	2,			/* number of interfaces in this configuration */
	1,			/* index of this configuration */
	0,			/* configuration name string index */
	0,
	//USBATTR_BUSPOWER,
	USB_CFG_MAX_BUS_POWER / 2,	/* max USB current in 2mA units */

	// B.3 AudioControl Interface Descriptors
	// The AudioControl interface describes the device structure (audio function topology)
	// and is used to manipulate the Audio Controls. This device has no audio function
	// incorporated. However, the AudioControl interface is mandatory and therefore both
	// the standard AC interface descriptor and the classspecific AC interface descriptor
	// must be present. The class-specific AC interface descriptor only contains the header
	// descriptor.

	// B.3.1 Standard AC Interface Descriptor
	// The AudioControl interface has no dedicated endpoints associated with it. It uses the
	// default pipe (endpoint 0) for all communication purposes. Class-specific AudioControl
	// Requests are sent using the default pipe. There is no Status Interrupt endpoint provided.
	/* AC interface descriptor follows inline: */
	9,			/* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE,	/* descriptor type */
	0,			/* index of this interface */
	0,			/* alternate setting for this interface */
	0,			/* endpoints excl 0: number of endpoint descriptors to follow */
	1,			/* */
	1,			/* */
	0,			/* */
	0,			/* string index for interface */

	// B.3.2 Class-specific AC Interface Descriptor
	// The Class-specific AC interface descriptor is always headed by a Header descriptor
	// that contains general information about the AudioControl interface. It contains all
	// the pointers needed to describe the Audio Interface Collection, associated with the
	// described audio function. Only the Header descriptor is present in this device
	// because it does not contain any audio functionality as such.
	/* AC Class-Specific descriptor */
	9,			/* sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes */
	36,			/* descriptor type */
	1,			/* header functional descriptor */
	0x0, 0x01,		/* bcdADC */
	9, 0,			/* wTotalLength */
	1,			/* */
	1,			/* */

	// B.4 MIDIStreaming Interface Descriptors

	// B.4.1 Standard MS Interface Descriptor
	/* interface descriptor follows inline: */
	9,			/* length of descriptor in bytes */
	USBDESCR_INTERFACE,	/* descriptor type */
	1,			/* index of this interface */
	0,			/* alternate setting for this interface */
	2,			/* endpoints excl 0: number of endpoint descriptors to follow */
	1,			/* AUDIO */
	3,			/* MS */
	0,			/* unused */
	0,			/* string index for interface */

	// B.4.2 Class-specific MS Interface Descriptor
	/* MS Class-Specific descriptor */
	7,			/* length of descriptor in bytes */
	36,			/* descriptor type */
	1,			/* header functional descriptor */
	0x0, 0x01,		/* bcdADC */
	65, 0,			/* wTotalLength */

	// B.4.3 MIDI IN Jack Descriptor
	6,			/* bLength */
	36,			/* descriptor type */
	2,			/* MIDI_IN_JACK desc subtype */
	1,			/* EMBEDDED bJackType */
	1,			/* bJackID */
	0,			/* iJack */

	6,			/* bLength */
	36,			/* descriptor type */
	2,			/* MIDI_IN_JACK desc subtype */
	2,			/* EXTERNAL bJackType */
	2,			/* bJackID */
	0,			/* iJack */

	//B.4.4 MIDI OUT Jack Descriptor
	9,			/* length of descriptor in bytes */
	36,			/* descriptor type */
	3,			/* MIDI_OUT_JACK descriptor */
	1,			/* EMBEDDED bJackType */
	3,			/* bJackID */
	1,			/* No of input pins */
	2,			/* BaSourceID */
	1,			/* BaSourcePin */
	0,			/* iJack */

	9,			/* bLength of descriptor in bytes */
	36,			/* bDescriptorType */
	3,			/* MIDI_OUT_JACK bDescriptorSubtype */
	2,			/* EXTERNAL bJackType */
	4,			/* bJackID */
	1,			/* bNrInputPins */
	1,			/* baSourceID (0) */
	1,			/* baSourcePin (0) */
	0,			/* iJack */


	// B.5 Bulk OUT Endpoint Descriptors

	//B.5.1 Standard Bulk OUT Endpoint Descriptor
	9,			/* bLenght */
	USBDESCR_ENDPOINT,	/* bDescriptorType = endpoint */
	0x1,			/* bEndpointAddress OUT endpoint number 1 */
	3,			/* bmAttributes: 2:Bulk, 3:Interrupt endpoint */
	8, 0,			/* wMaxPacketSize */
	10,			/* bIntervall in ms */
	0,			/* bRefresh */
	0,			/* bSyncAddress */

	// B.5.2 Class-specific MS Bulk OUT Endpoint Descriptor
	5,			/* bLength of descriptor in bytes */
	37,			/* bDescriptorType */
	1,			/* bDescriptorSubtype */
	1,			/* bNumEmbMIDIJack  */
	1,			/* baAssocJackID (0) */


	//B.6 Bulk IN Endpoint Descriptors

	//B.6.1 Standard Bulk IN Endpoint Descriptor
	9,			/* bLenght */
	USBDESCR_ENDPOINT,	/* bDescriptorType = endpoint */
	0x81,			/* bEndpointAddress IN endpoint number 1 */
	3,			/* bmAttributes: 2: Bulk, 3: Interrupt endpoint */
	8, 0,			/* wMaxPacketSize */
	10,			/* bIntervall in ms */
	0,			/* bRefresh */
	0,			/* bSyncAddress */

	// B.6.2 Class-specific MS Bulk IN Endpoint Descriptor
	5,			/* bLength of descriptor in bytes */
	37,			/* bDescriptorType */
	1,			/* bDescriptorSubtype */
	1,			/* bNumEmbMIDIJack (0) */
	3,			/* baAssocJackID (0) */
};


uchar usbFunctionDescriptor(usbRequest_t * rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE) {
		usbMsgPtr = (uchar *) deviceDescrMIDI;
		return sizeof(deviceDescrMIDI);
	}
	else {		/* must be config descriptor */
		usbMsgPtr = (uchar *) configDescrMIDI;
		return sizeof(configDescrMIDI);
	}
}


/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */

uchar usbFunctionSetup(uchar data[8]) {
	return 0xff;
}

uchar usbFunctionRead(uchar * data, uchar len) {	
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;

	return 7;
}

uchar usbFunctionWrite(uchar * data, uchar len) {
	return 1;
}


// Oscillator Calibration
// Taken directly from EasyLogger: 
// https://www.obdev.at/products/vusb/easylogger.html

static void calibrateOscillator(void) {
	uchar	step = 128;
	uchar	trialValue = 0, optimumValue;
	int		x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

	/* do a binary search: */
	do{
		OSCCAL = trialValue + step;
		x = usbMeasureFrameLength();	/* proportional to current real frequency */
		if(x < targetValue)				/* frequency still too low */
			trialValue += step;
		step >>= 1;
	}while(step > 0);
	/* We have a precision of +/- 1 for optimum OSCCAL here */
	/* now do a neighborhood search for optimum value */
	optimumValue = trialValue;
	optimumDev = x; /* this is certainly far away from optimum */
	for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
		x = usbMeasureFrameLength() - targetValue;
		if(x < 0)
			x = -x;
		if(x < optimumDev){
			optimumDev = x;
			optimumValue = OSCCAL;
		}
	}
	OSCCAL = optimumValue;
}

void usbEventResetReady(void) {
	/* Disable interrupts during oscillator calibration since
	 * usbMeasureFrameLength() counts CPU cycles.
	 */
	cli();
	calibrateOscillator();
	sei();
	//eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}


uchar should_be_on=0;

void usbMidiMessageIn(uchar * data)
{
	wdt_reset();
	if(data[0]==0x0B && data[1]==0xB0 && data[2]==100 && data[3]==0)
		should_be_on ^= 1;
}

void usbFunctionWriteOut(uchar * data, uchar len)
{
	usbMidiMessageIn(data);
	if (len==8) usbMidiMessageIn(data+4);
}


//////// Main ////////////

int main(void) {
	DDRB |= 1 << PB1;
	PORTB |= 1 << PB1;
	usbDeviceDisconnect();
	for(uchar i=0;i<250;i++)
	{  /* 500 ms disconnect */
		wdt_reset();
		_delay_ms(2);
	}
	usbDeviceConnect();
	
	wdt_enable(WDTO_1S);

	usbInit();
	sei();
	for(;;)
	{
		
		wdt_reset();
		usbPoll();
		if(usbInterruptIsReady())
		{
			
			if(should_be_on)
			{
				PORTB |= 1<<PB1;
			}
			else
			{
				PORTB &= ~(1<<PB1);
			}
			//usbSetInterrupt((uchar*)"\x09\x90\x2a\x2a",4);

		}

	}
	return 0;
}
