/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/

#ifndef DM_TFT_ILI9341_h
#define DM_TFT_ILI9341_h

#include "DmTftBase.h"
#include "dm_platform.h"

typedef struct {
	  void (*init)(uint16_t width, uint16_t height);

	  void (*setTextColor)(uint16_t background, uint16_t foreground);

	  void (*clearScreen)(uint16_t color);

	  void (*drawLine)(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
	  void (*drawVerticalLine)(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
	  void (*drawHorizontalLine)(uint16_t x, uint16_t y, uint16_t length, uint16_t color);

	  void (*drawRectangle)(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
	  void (*fillRectangle)(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

	  void (*drawCircle)(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
	  void (*fillCircle)(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

	  void (*drawTriangle)(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

	  void (*drawPoint)(uint16_t x, uint16_t y, uint16_t radius);

	  void (*drawChar)(uint16_t x, uint16_t y, char ch, bool transparent);
	  void (*drawNumber)(uint16_t x, uint16_t y, int num, int digitsToShow, bool leadingZeros);
	  void (*drawString)(uint16_t x, uint16_t y, const char *p);
	  void (*drawStringCentered)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char *p);

	  void (*drawImage)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* data);

} DmTftIli9341;

void newDmTftIli9341(DmTftIli9341 *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dc_port, uint16_t dc_pin);

#endif


