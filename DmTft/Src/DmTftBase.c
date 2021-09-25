/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/

#include "DmTftBase.h"

#include <stdlib.h>
#include <string.h>

#include "font-8x16.h"

#define constrain(val,min,max) ((val)<(min))?(min):((val)>(max))?(max):(val)

extern void TFTsetAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
extern void TFTsendData(uint16_t data);
extern void TFTsendBuffer(uint32_t size, uint8_t *buffer);
extern void TFTsendRepeatedData(uint32_t num, uint16_t data);
extern void TFTsendCommand(uint8_t index);
extern void TFTselect();
extern void TFTunSelect();

static uint16_t _width;
static uint16_t _height;

static uint16_t _bgColor;
static uint16_t _fgColor;

void GrSetWidth(uint16_t width) {
	_width = width;
}

void GrSetHeight(uint16_t height) {
	_height = height;
}

void GrSetTextColor(uint16_t background, uint16_t foreground) {
	_bgColor = background; _fgColor = foreground;
}

void setPixel(uint16_t x, uint16_t y, uint16_t color) {
	TFTselect();

	if (x>(_width-1) || x< 0 || y>(_height-1) || y< 0) {
		goto exit;
	}

	TFTsetAddress(x, y, x, y);

	TFTsendData(color);

	exit:
	TFTunSelect();
}

void GrClearScreen(uint16_t color) {
	TFTselect();

	TFTsetAddress(0,0,_width-1, _height-1);

	TFTsendRepeatedData(_height*_width, color);

	TFTunSelect();
}

void GrDrawHorizontalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	TFTselect();

	TFTsetAddress(x, y, x + length, y);

	TFTsendRepeatedData(length+1, color);

	TFTunSelect();
}

void GrDrawVerticalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	TFTselect();

	TFTsetAddress(x, y, x, y + length);

	TFTsendRepeatedData(length+1, color);

	TFTunSelect();
}

void GrDrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	int x = x1-x0;
	int y = y1-y0;
	int dx = abs(x), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y), sy = y0<y1 ? 1 : -1;
	int err = dx+dy, e2;  /* error value e_xy             */

	for (;;) {
		setPixel(x0,y0,color);
		e2 = 2*err;
		if (e2 >= dy) {      /* e_xy+e_x > 0                 */
			if (x0 == x1) {
				break;
			}
			err += dy; x0 += sx;
		}
		if (e2 <= dx) { /* e_xy+e_y < 0   */
			if (y0 == y1) {
				break;
			}
			err += dx; y0 += sy;
		}
	}
}

void GrDrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	// Make sure x0,y0 are in the top left corner
	if(x0 > x1) {
		x0 = x0^x1;
		x1 = x0^x1;
		x0 = x0^x1;
	}
	if(y0 > y1) {
		y0 = y0^y1;
		y1 = y0^y1;
		y0 = y0^y1;
	}

	GrDrawHorizontalLine(x0, y0, x1-x0, color);
	GrDrawHorizontalLine(x0, y1, x1-x0, color);
	GrDrawVerticalLine(x0, y0, y1-y0, color);
	GrDrawVerticalLine(x1, y0, y1-y0, color);
}

void GrFillRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	unsigned long numPixels=0;

	// Make sure x0,y0 are in the top left corner
	if(x0 > x1) {
		x0 = x0^x1;
		x1 = x0^x1;
		x0 = x0^x1;
	}
	if(y0 > y1) {
		y0 = y0^y1;
		y1 = y0^y1;
		y0 = y0^y1;
	}

	x0 = constrain(x0, 0, _width-1);
	x1 = constrain(x1, 0, _width-1);
	y0 = constrain(y0, 0, _height-1);
	y1 = constrain(y1, 0, _height-1);

	numPixels = (x1-x0+1);
	numPixels = numPixels*(y1-y0+1);

	TFTselect();

	TFTsetAddress(x0,y0,x1,y1);/* start to write to display ra */

	TFTsendRepeatedData(numPixels, color);

	TFTunSelect();
}

void GrDrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x = -r, y = 0, err = 2-2*r, e2;
	do {
		setPixel(x0-x, y0+y, color);
		setPixel(x0+x, y0+y, color);
		setPixel(x0+x, y0-y, color);
		setPixel(x0-x, y0-y, color);
		e2 = err;
		if (e2 <= y) {
			err += ++y*2+1;
			if (-x == y && e2 <= x) {
				e2 = 0;
			}
		}
		if (e2 > x) {
			err += ++x * 2 + 1;
		}
	} while (x <= 0);
}

void GrFillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
	int x = -r, y = 0, err = 2-2*r, e2;
	do {
		GrDrawVerticalLine(x0-x, y0-y, 2*y, color);
		GrDrawVerticalLine(x0+x, y0-y, 2*y, color);

		e2 = err;
		if (e2 <= y) {
			err += ++y * 2 + 1;
			if (-x == y && e2 <= x) {
				e2 = 0;
			}
		}
		if (e2 > x) {
			err += ++x*2+1;
		}
	} while (x <= 0);
}

void GrDrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	GrDrawLine(x0, y0, x1, y1, color);
	GrDrawLine(x0, y0, x2, y2, color);
	GrDrawLine(x1, y1, x2, y2, color);
}

void GrDrawPoint(uint16_t x, uint16_t y, uint16_t radius) {
	if (radius == 0) {
		TFTselect();

		TFTsetAddress(x,y,x,y);
		TFTsendData(_fgColor);

		TFTunSelect();
	} else {
		GrFillRectangle(x-radius,y-radius,x+radius,y+radius, _fgColor);
	}
}

static uint16_t charBuffer[FONT_CHAR_HEIGHT*FONT_CHAR_WIDTH];

void GrDrawChar(uint16_t x, uint16_t y, char ch, bool transparent) {
	TFTselect();

	const uint8_t *row;
	uint8_t pos, mask;
	uint16_t index, fg, bg;

	if ((x > (_width - FONT_CHAR_WIDTH)) || (y > (_height - FONT_CHAR_HEIGHT))) {
		goto exit;
	}

	fg = (_fgColor>>8) | (_fgColor<<8);
	bg = (_bgColor>>8) | (_bgColor<<8);

	index = getGlyphIndex(ch);
	row = &fb_font[index*FONT_CHAR_HEIGHT];

	if (!transparent) { // Clear background
		uint16_t *point = charBuffer;
		TFTsetAddress(x,y,x+FONT_CHAR_WIDTH-1,y+FONT_CHAR_HEIGHT-1);
		for(pos=0;pos<FONT_CHAR_HEIGHT;pos++) {
			for(mask=0x80; mask; mask>>=1) {
				if ((*row) & mask) {
					*point++ = fg;
				}
				else {
					*point++ = bg;
				}
			}
			row++;
		}
		TFTsendBuffer(sizeof(charBuffer), (uint8_t*)charBuffer);
	}
	else { //Draw directly without clearing background
		for(pos=0;pos<FONT_CHAR_HEIGHT;pos++) {
			for(int t=0, mask=0x80; t < FONT_CHAR_WIDTH; t++) {
				if ((*row) & mask) {
					TFTsetAddress(x + t, y + pos, x + t, y + pos);
					TFTsendData(_fgColor);
				}
				mask>>=1;
			}
		}
	}

	exit:
	TFTunSelect();
}

void GrDrawNumber(uint16_t x, uint16_t y, int num, int digitsToShow, bool leadingZeros) {
	bool minus = false;
	if (num < 0) {
		num = -num;
		minus = true;
	}
	for (int i = 0; i < digitsToShow; i++) {
		char c = ' ';
		if ((num == 0) && (i > 0)) {
			if (leadingZeros) {
				c = '0';
				if (minus && (i == (digitsToShow-1))) {
					c = '-';
				}
			} else if (minus) {
				c = '-';
				minus = false;
			}
		} else {
			c = '0' + (num % 10);
		}
		GrDrawChar(x + FONT_CHAR_WIDTH*(digitsToShow - i - 1), y, c, false);
		num = num / 10;
	}
}

void GrDrawString(uint16_t x, uint16_t y, const char *p) {
	while(*p!='\0')
	{
		if(x > (_width - FONT_CHAR_WIDTH)) {
			x = 0;
			y += FONT_CHAR_HEIGHT;
		}
		if(y > (_height - FONT_CHAR_HEIGHT)) {
			y = x = 0;
		}
		GrDrawChar(x, y, *p, false);
		x += FONT_CHAR_WIDTH;
		p++;
	}
}

void GrDrawStringCentered(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *p) {
	int len = strlen(p);
	uint16_t tmp = len * FONT_CHAR_WIDTH;
	if (tmp <= width) {
		x += (width - tmp)/2;
	}
	if (FONT_CHAR_HEIGHT <= height) {
		y += (height - FONT_CHAR_HEIGHT)/2;
	}
	GrDrawString(x, y, p);
}

void GrDrawStringInRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *p, uint16_t *lastY) {
	uint16_t lineWidth = width / FONT_CHAR_WIDTH;
	uint16_t ymax = y + height - FONT_CHAR_HEIGHT;

	while(*p!='\0')
	{
		uint16_t _x;
		uint16_t space;
		// Word wrap
		space = lineWidth;
		for(int w = 0; w < lineWidth; w++) {
			if (p[w] == ' ' || p[w] == 0) {
				space = w;
				if (p[w] == 0) break;
			}
		}

		_x = x;
		for(int w = 0; w < space; w++) {
			GrDrawChar(_x, y, *p, false);
			_x += FONT_CHAR_WIDTH;
			p++;
		}
		while (*p == ' ') p++;

		y += FONT_CHAR_HEIGHT;
		if(y > ymax) {
			break; // Clip
		}
	}
	if (*lastY) *lastY = y;
}

void GrDrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* data) {
	TFTselect();

	TFTsetAddress(x,y,x+width-1,y+height-1);

	TFTsendBuffer(width*height*sizeof(uint16_t), (uint8_t*)data);

	TFTunSelect();
}
