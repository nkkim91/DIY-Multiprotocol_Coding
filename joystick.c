/*
 * jstest.c  Version 1.2
 *
 * Copyright (c) 1996-1999 Vojtech Pavlik
 *
 * Sponsored by SuSE
 */

/*
 * This program can be used to test all the features of the Linux
 * joystick API, including non-blocking and select() access, as
 * well as version 0.x compatibility mode. It is also intended to
 * serve as an example implementation for those who wish to learn
 * how to write their own joystick using applications.
 */

/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <linux/input.h>
#include <linux/joystick.h>

#include "axbtnmap.h"
//#include "serial_input.h"
#include "common.h"

char *axis_names[ABS_MAX + 1] = {
"X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder", 
"Wheel", "Gas", "Brake", "?", "?", "?", "?", "?",
"Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat3Y",
"?", "?", "?", "?", "?", "?", "?", 
};

char *button_names[KEY_MAX - BTN_MISC + 1] = {
"Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7", "Btn8", "Btn9", "?", "?", "?", "?", "?", "?",
"LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn", "TaskBtn", "?", "?", "?", "?", "?", "?", "?", "?",
"Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2", "PinkieBtn", "BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5", "BaseBtn6", "BtnDead",
"BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR", "BtnTL2", "BtnTR2", "BtnSelect", "BtnStart", "BtnMode", "BtnThumbL", "BtnThumbR", "?",
"?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", 
"WheelBtn", "Gear up",
};

#define NAME_LENGTH 128
#define JS_NORMAL
//#define JS_EVENT
//#define JS_OLD

int nJSDevFD;
unsigned char gcAxes = 2;
unsigned char gcButtons = 2;
int gnVersion = 0x000800;
char gcName[NAME_LENGTH] = "Unknown";
uint16_t gusBtnmap[BTNMAP_SIZE];	/* KEY_MAX_LARGE(0x2FF) - BTN_MISC(0x100) + 1 */
uint8_t gucAxmap[AXMAP_SIZE];	/* ABS_MAX(0x3F) + 1 */
int gnBtnmapok = 1;

extern union ChannelPacketData_b gstTxChannelPacketData, gstTempTxChannelPacketData;
extern unsigned int gunDebugMask;

int js_open(char *device)
{
	int nRet = 0;
	
	if ((nJSDevFD = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
		printf("Error - open (%s)\n", device);
		nRet = -1;
	}

	return nRet;
}

int js_close(void)
{
	int nRet = 0;

	close(nJSDevFD);

	return nRet;
}

int js_init (void)
{
	int32_t i;

	ioctl(nJSDevFD, JSIOCGVERSION, &gnVersion);
	ioctl(nJSDevFD, JSIOCGAXES, &gcAxes);
	ioctl(nJSDevFD, JSIOCGBUTTONS, &gcButtons);
	ioctl(nJSDevFD, JSIOCGNAME(NAME_LENGTH), gcName);

	getaxmap(nJSDevFD, gucAxmap);
	getbtnmap(nJSDevFD, gusBtnmap);

	if( gunDebugMask & NK_DEBUG_INFO ) {
		printf("Driver version is %d.%d.%d.\n",
			gnVersion >> 16, (gnVersion >> 8) & 0xff, gnVersion & 0xff);
	}

	/* Determine whether the button map is usable. */
	for (i = 0; gnBtnmapok && i < gcButtons; i++) {
		if (gusBtnmap[i] < BTN_MISC || gusBtnmap[i] > KEY_MAX) {	/* BTN_MISC : 0x100, KEY_MAX : 0x2FF, valid button is btw. 0x100 ~ 0x2FF */
			gnBtnmapok = 0;
			break;
		}
	}
	if (!gnBtnmapok) {
		/* btnmap out of range for names. Don't print any. */
		printf("It's not fully compatible with your kernel. Unable to retrieve button map!");
		printf("Joystick (%s) has %d axes and %d buttons.\n", gcName, gcAxes, gcButtons);
	} else {
		printf("Joystick (%s) has %d axes (", gcName, gcAxes);
		for (i = 0; i < gcAxes; i++)
			printf("%s%s", i > 0 ? ", " : "", axis_names[gucAxmap[i]]);	/* gucAxmap[i] = 0, 1, 6 */
		puts(")");
		printf("and %d buttons (", gcButtons);
		for (i = 0; i < gcButtons; i++) {
			printf("%s%s", i > 0 ? ", " : "", button_names[gusBtnmap[i] - BTN_MISC]);	/* (0x100 ~ 0x2FF) - 0x100 */
		}
		puts(").");
	}

	return 0;
}

int js_input(unsigned int *unChannelData)
{
	int i;

	int *axis;
	char *button;
#if defined(JS_NORMAL) || defined(JS_EVENT)
	struct js_event js;
#elif defined(JS_OLD) 
	struct JS_DATA_TYPE js; 
#endif

#ifdef JS_NORMAL

	axis = calloc(gcAxes, sizeof(int));
	button = calloc(gcButtons, sizeof(char));

	if (read(nJSDevFD, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
		if( gunDebugMask & NK_DEBUG_SPOT ) {
			printf("read() - Data not ready/available\n");
		}
		return 1;
	}

	switch(js.type & ~JS_EVENT_INIT) {
		case JS_EVENT_BUTTON:	/* 0x01 */
			button[js.number] = js.value;
			break;
		case JS_EVENT_AXIS:		/* 0x02 */
			axis[js.number] = js.value;
			break;
		case JS_EVENT_INIT:		/* 0x80 */
			break;
	}

	if( gunDebugMask & NK_DEBUG_DEBUG ) {
		if (gcAxes) {
			printf("Axes: ");
			for (i = 0; i < gcAxes; i++)
				printf("%2d:%6d ", i, axis[i]);
		}

		if (gcButtons) {
			printf("Buttons: ");
			for (i = 0; i < gcButtons; i++)
				printf("%2d:%s ", i, button[i] ? "on " : "off");
		}
		printf("\n");
		fflush(stdout);
	}
#elif defined(JS_EVENT)
	if (read(nJSDevFD, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
		if( gunDebugMask & NK_DEBUG_SPOT ) {
			printf("read() - Data not ready/available\n");
		}
		return 1;
	}
	if( js.type == JS_EVENT_AXIS ) {	/* Axes = 2 */
		switch(js.number) {
			case 0 : 	/* 1st X axis */
				if( gunDebugMask & NK_DEBUG_DEBUG ) {
					printf("js.type = 2, js.number = 0, Set Rudder \n");
				}
				gstTempTxChannelPacketData.stChannelPacketData.stChannelData.ucRudder = ((65535 - (js.value + 32767)) >> 8) ;
				break;

			case 1 :	/* 1st Y axis */
				if( gunDebugMask & NK_DEBUG_DEBUG ) {
					printf("js.type = 2, js.number = 1, Set Elevator\n");
				}
				gstTempTxChannelPacketData.stChannelPacketData.stChannelData.ucElevator = ((js.value + 32767) >> 8);	/*  */
				break;

			default:
				break;
		}
	}

	if( js.type == JS_EVENT_BUTTON ) {	/* Button = 1 */

	}

	if( gunDebugMask & NK_DEBUG_DEBUG ) {
		printf("Event: type %d, time %d, number %d, value %d\n",
			js.type, js.time, js.number, js.value);
		fflush(stdout);
	}
#elif defined(JS_OLD)

	if (read(nJSDevFD, &js, JS_RETURN) != JS_RETURN) {
		if( gunDebugMask & NK_DEBUG_SPOT ) {
			printf("read() - Data not ready/available\n");
		}
		return 1;
	}

	if( gunDebugMask & NK_DEBUG_DEBUG ) {
		printf("Axes: X:%3d Y:%3d Buttons: A:%s B:%s\n",
			js.x, js.y, (js.buttons & 1) ? "on " : "off", (js.buttons & 2) ? "on " : "off");
	}

	gstTempTxChannelPacketData.stChannelPacketData.stChannelData.ucRudder = js.x;
	gstTempTxChannelPacketData.stChannelPacketData.stChannelData.ucElevator = js.y;

	usleep(10000);

#else
#error "JS mode Not defined !!"
#endif

}
