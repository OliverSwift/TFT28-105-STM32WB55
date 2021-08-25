/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/
#ifndef DMTFTBASE_h
#define DMTFTBASE_h

#include <stdint.h>
#include <stdbool.h>

void GrSetWidth(uint16_t width);
void GrSetHeight(uint16_t height);
  
void GrSetTextColor(uint16_t background, uint16_t foreground);

void GrClearScreen(uint16_t color);

void GrDrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void GrDrawVerticalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void GrDrawHorizontalLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

void GrDrawRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void GrFillRectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

void GrDrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void GrFillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

void GrDrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void GrDrawPoint(uint16_t x, uint16_t y, uint16_t radius);

void GrDrawChar(uint16_t x, uint16_t y, char ch, bool transparent);
void GrDrawNumber(uint16_t x, uint16_t y, int num, int digitsToShow, bool leadingZeros);
void GrDrawString(uint16_t x, uint16_t y, const char *p);
void GrDrawStringCentered(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *p);

void GrDrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* data);
#endif




