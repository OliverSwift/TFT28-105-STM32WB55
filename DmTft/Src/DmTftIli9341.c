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
#include "DmTftIli9341.h"
#include "dm_platform.h"

static GPIO_TypeDef *_cs_port;
static uint16_t _cs_pin;
static GPIO_TypeDef *_dc_port;
static uint16_t _dc_pin;

extern SPI_HandleTypeDef hspi1;

void TFTselect(){
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
}

void TFTunSelect() {
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

void TFTwriteBus(uint8_t data) {
	HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
}

void TFTsendCommand(uint8_t index) {
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_RESET);

	TFTwriteBus(index);
}

void TFTsend8BitData(uint8_t data) {
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);

	TFTwriteBus(data);
}

void TFTsendBuffer(uint32_t size, const uint8_t *buffer) {
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);

	while (size) {
		if (size > 32767) {
			HAL_SPI_Transmit(&hspi1, (uint8_t*)buffer, 32768, 1000);
			size -= 32768;
			buffer += 32768;
		} else {
			HAL_SPI_Transmit(&hspi1, (uint8_t*)buffer, size, 1000);
			break;
		}
	}
}

void TFTsendData(uint16_t data) {
	uint16_t bData = (data>>8) | (data<<8);

	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, (uint8_t*)&bData, sizeof(bData), 1000);
}

#define BLOCK_SIZE 1024
static uint16_t block[BLOCK_SIZE];

void TFTsendRepeatedData(uint32_t num, uint16_t value) {
	uint32_t i;
	uint32_t size = num;
	uint16_t word = ((value>>8)&0xff) | (value<<8);

	// Fill block with repeated value
	if (size > BLOCK_SIZE) size = BLOCK_SIZE;
	for(i=0; i < size; i++) {
		block[i] = word;
	}

	// Prepare for data
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET); // Data

	// Send data by block
	while (1) {
		if (num > BLOCK_SIZE) {
			HAL_SPI_Transmit(&hspi1, (uint8_t *)block, sizeof(block), 1000);
			num -= BLOCK_SIZE;
		} else {
			HAL_SPI_Transmit(&hspi1, (uint8_t *)block, num*sizeof(uint16_t), 1000);
			break;
		}
	}
}

void TFTsetAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	TFTsendCommand(0x2A); // Set Column
	TFTsendData(x0);
	TFTsendData(x1);

	TFTsendCommand(0x2B);  // Set Page
	TFTsendData(y0);
	TFTsendData(y1);

	TFTsendCommand(0x2c);
}

static void DmTftIli9341_init(uint16_t width, uint16_t height) {
	GrSetWidth(width);
	GrSetHeight(height);
	GrSetTextColor(BLACK, WHITE);
#if defined (DM_TOOLCHAIN_ARDUINO)
	_pinCS  = portOutputRegister(digitalPinToPort(_cs));
	_bitmaskCS  = digitalPinToBitMask(_cs);
	_pinDC  = portOutputRegister(digitalPinToPort(_dc));
	_bitmaskDC  = digitalPinToBitMask(_dc);
	pinMode(_cs,OUTPUT);
	pinMode(_dc,OUTPUT);

	sbi(_pinCS, _bitmaskCS);

	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz (full! speed!)
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	_spiSettings = SPCR;
#elif defined (DM_TOOLCHAIN_MBED)
	_pinCS = new DigitalOut((PinName)_cs);
	_pinDC = new DigitalOut((PinName)_dc);
	sbi(_pinCS, _bitmaskCS);

	_spi = new SPI((PinName)_mosi, (PinName)_miso, (PinName)_clk);
	_spi->format(8,0);
	_spi->frequency(8000000); // Max SPI speed for display is 10 and for 17 for LPC15xx
#endif

	TFTselect();
	//delay(135); // This much delay needed??

	// ILI9341 init
	TFTsendCommand(0x11);
	delay(120);

	// Init commands and parameters
	const uint8_t inits[] = {
			// CMD, Num, parameters
			0xCF,   3,   0x00, 0xc3, 0x30,    			// Power control B  PCEQ=1
			0xED,   4,   0x64, 0x03, 0x12, 0x81, 		// Power on sequence control
			0xE8,   3,   0x85, 0x10, 0x79, 				// Driver timing control A
			0xCB,   5,   0x39, 0x2C, 0x00, 0x34, 0x02, 	// Power control A
			0xF7,   1,   0x20, 							// Pump ratio control  DDVDH=2xVCI
			0xEA,   2,   0x00, 0x00, 					// Driver timing control B
			0xC0,   1,   0x22, 							// Power Control 1 VRH[5:0] -> 4.55V
			0xC1,   1,   0x11, 							// Power Control 2 SAP[2:0] -> ???; BT[3:0] -> VGL=7*VCI VGH=-3*VCI
			0xC5,   2,   0x3d, 0x20, 					// VCOM Control 1  VMH[6:0] -> 4.225V; VML[6:0] -> -1.7V
			0xC7,   1,   0xAA, 							// VCOM Control 2  nVM[7] -> 1; VMF[6:0] -> VMH – 25 VML – 25
			0x36,   1,   0x08, 							// Memory Access Control = BGR (565)  (weird, seems to be the other around)
			0x3A,   1,   0x55, 							// Pixel format set = RGB=MCU=16bits per pixel
			0xB1,   2,   0x00, 0x13, 					// Frame control DIVA=00h (freq=fosc) RTNA=13h (100Hz)
			0xB6,   2,   0x0A, 0xA2, 					// Display Function Control (SS=1, rest is default)
			0xF2,   1,   0x00, 							// Enable 3Gamma -> NO
			0x00 // END
	};

	const uint8_t *p = inits;

	while(p[0]) {
		TFTsendCommand(p[0]);
		TFTsendBuffer(p[1], p+2);
		p += 2 + p[1];
	}

	TFTsendCommand(0x11);    //Exit Sleep
	delay(120);
	TFTsendCommand(0x29);    //Display on
	delay(50);

	// Clear screen
	TFTsetAddress(0,0,width-1, height-1); // All screen

	TFTsendRepeatedData(height*width, BLACK);

	TFTunSelect();
}

void setupDmTftIli9341(DmTftIli9341 *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dc_port, uint16_t dc_pin) {
	_cs_port = cs_port;
	_cs_pin = cs_pin;
	_dc_port = dc_port;
	_dc_pin = dc_pin;

	handle->init = DmTftIli9341_init;
	handle->clearScreen = GrClearScreen;
	handle->drawChar = GrDrawChar;
	handle->drawCircle = GrDrawCircle;
	handle->drawHorizontalLine = GrDrawHorizontalLine;
	handle->drawImage = GrDrawImage;
	handle->drawLine = GrDrawLine;
	handle->drawNumber = GrDrawNumber;
	handle->drawPoint = GrDrawPoint;
	handle->drawRectangle = GrDrawRectangle;
	handle->drawString = GrDrawString;
	handle->drawStringCentered = GrDrawStringCentered;
	handle->drawTriangle = GrDrawTriangle;
	handle->drawVerticalLine = GrDrawVerticalLine;
	handle->fillCircle = GrFillCircle;
	handle->fillRectangle = GrFillRectangle;
}

/*********************************************************************************************************
  END FILE
 *********************************************************************************************************/

