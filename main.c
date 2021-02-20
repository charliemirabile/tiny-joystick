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

uchar usbFunctionSetup(uchar data[8])
{
	return 0;
}



// Oscillator Calibration
// Taken directly from EasyLogger: 
// https://www.obdev.at/products/vusb/easylogger.html

static void calibrateOscillator(void)
{
	uchar step = 128, trialValue = 0, optimumValue;
	int x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

	/* do a binary search: */
	do
	{
		OSCCAL = trialValue + step;
		x = usbMeasureFrameLength();	/* proportional to current real frequency */
		if(x < targetValue)		/* frequency still too low */
			trialValue += step;
		step >>= 1;
	}
	while(step > 0);

	/* We have a precision of +/- 1 for optimum OSCCAL here */
	/* now do a neighborhood search for optimum value */

	optimumValue = trialValue;

	optimumDev = x; /* this is certainly far away from optimum */

	for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++)
	{
		x = usbMeasureFrameLength() - targetValue;

		if(x < 0) x = -x;	//absolute value

		if(x < optimumDev)
		{
			optimumDev = x;
			optimumValue = OSCCAL;
		}
	}
	OSCCAL = optimumValue;
}

void usbEventResetReady(void)
{
	// Disable interrupts during oscillator calibration since
	// usbMeasureFrameLength() counts CPU cycles.
	cli();
	calibrateOscillator();
	sei();
}

//uchar note = 0;

void usbFunctionWriteOut(uchar * data, uchar len)
{/*
	if(data[0]==0x0B && data[1]==0xB0 && data[2]==100 && data[3]==0)
	{
		++note;
		if(note==128)
			note = 0;
	}
*/}

typedef enum
{
	CENTER,
	UP,
	DOWN,
	LEFT,
	RIGHT,
}
Position;

uchar get_pos(void)
{
	ADMUX = (1<<ADLAR) | 0b11; //select reading from PB3

	ADCSRA |= (1<<ADSC) | (1 << ADIF); //clear interrupt flag and start conversion

	while(!(ADCSRA & (1<<ADIF))) //busy loop waiting for conversion to finish
		;

	if(ADCH<32)
		return UP;

	if(ADCH>224)
		return DOWN;

	ADMUX = (1<<ADLAR) | 0b10; //select reading from PB4

	ADCSRA |= (1<<ADSC) | (1<< ADIF); //clear interrupt flag and start conversion

	while(!(ADCSRA & (1<<ADIF))) //busy loop waiting for conversion to finish
		;

	if(ADCH<32)
		return LEFT;

	if(ADCH>224)
		return RIGHT;

	return CENTER;
}

typedef union
{
	uchar bytes[4];
	struct
	{
		uchar cablenum:4;
		uchar codeindex:4;
		uchar msg_type:4;
		uchar channel:4;
		uchar msb1:1;
		union
		{
			uchar note:7;
			uchar controller:7;
			uchar program:7;
			uchar chan_pressure:7;
			uchar bend_lsb:7;
		};
		uchar msb2:1;
		union
		{
			uchar velocity:7;
			uchar pressure:7;
			uchar value:7;
			uchar bend_msb:7;
		};
	};	
}
midimsg;



midimsg lookuptable[] = {
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=100, .value=100},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=101, .value=100},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=102, .value=100},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=103, .value=100},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=100, .value=0},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=101, .value=0},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=102, .value=0},
	(midimsg){.codeindex=0xB, .channel=0, .msg_type=0xB, .controller=103, .value=0},
/*	(midimsg){.bytes={0x0B, 0xB0, 100, 100}},
	(midimsg){.bytes={0x0B, 0xB0, 101, 100}},
	(midimsg){.bytes={0x0B, 0xB0, 102, 100}},
	(midimsg){.bytes={0x0B, 0xB0, 103, 100}},
	(midimsg){.bytes={0x0B, 0xB0, 100, 0}},
	(midimsg){.bytes={0x0B, 0xB0, 101, 0}},
	(midimsg){.bytes={0x0B, 0xB0, 102, 0}},
	(midimsg){.bytes={0x0B, 0xB0, 103, 0}},*/
};



//////// Main ////////////

void main(void)
{
	uchar last_pos = CENTER;
	wdt_disable();

	usbDeviceDisconnect();
	for(uchar i=0;i<250;i++)
	{
		//wdt_reset();
		_delay_ms(2);
	}
	usbDeviceConnect();
	

	usbInit();
	sei();
	ADCSRA = 1 << ADEN | 0b110; //enable ADC and set prescaler to 6 (divide by 64)

	for(;;)
	{		
		usbPoll();
		if(usbInterruptIsReady())
		{
			/*uchar pos = get_pos();
			if(pos != last_pos)
			{
				uchar move;
				if(last_pos)
					move=last_pos+3;
				else
					move=pos-1;
				last_pos = pos;
				usbSetInterrupt(lookuptable[move].bytes,sizeof(midimsg));
			}*/
			usbSetInterrupt((uchar[]){0x0B,0xB0,lookuptable[0].bytes[0]/16,lookuptable[0].bytes[0]%16},4);
		}

	}
}
