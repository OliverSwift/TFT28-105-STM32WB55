/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/

#include "DmTftIli9341.h"
#include "DmTftBase.h"
#include "dm_platform.h"

static GPIO_TypeDef *_cs_port;
static uint16_t _cs_pin;
static GPIO_TypeDef *_dc_port;
static uint16_t _dc_pin;

extern SPI_HandleTypeDef hspi1;

void select(){
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
}

void unSelect() {
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

void writeBus(uint8_t data) {
  HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
}

void sendCommand(uint8_t index) {
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_RESET);

	writeBus(index);
}

void send8BitData(uint8_t data) {
	HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);

	writeBus(data);
}

void sendData(uint16_t data) {
  uint16_t bData = (data>>8) | (data<<8);

  HAL_GPIO_WritePin(_dc_port, _dc_pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(&hspi1, (uint8_t*)&bData, sizeof(bData), 1000);
}

#define BLOCK_SIZE 1024
static uint16_t block[BLOCK_SIZE];

void sendRepeatedData(uint32_t num, uint16_t value) {
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

void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  sendCommand(0x2A); // Set Column
  sendData(x0);
  sendData(x1);

  sendCommand(0x2B);  // Set Page
  sendData(y0);
  sendData(y1);

  sendCommand(0x2c);
}

void DmTftIli9341_init(uint16_t width, uint16_t height) {
	setWidth(width);
	setHeight(height);
  setTextColor(BLACK, WHITE);
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
  select();
  delay(135); // This much delay needed??

  // ILI9341 init
  sendCommand(0x11);
  delay(120);

  sendCommand(0xCF);
  send8BitData(0x00);
  send8BitData(0xc3);
  send8BitData(0X30);

  sendCommand(0xED);
  send8BitData(0x64);
  send8BitData(0x03);
  send8BitData(0X12);
  send8BitData(0X81);

  sendCommand(0xE8);
  send8BitData(0x85);
  send8BitData(0x10);
  send8BitData(0x79);

  sendCommand(0xCB);
  send8BitData(0x39);
  send8BitData(0x2C);
  send8BitData(0x00);
  send8BitData(0x34);
  send8BitData(0x02);

  sendCommand(0xF7);
  send8BitData(0x20);

  sendCommand(0xEA);
  send8BitData(0x00);
  send8BitData(0x00);

  sendCommand(0xC0);    //Power control
  send8BitData(0x22);   //VRH[5:0]

  sendCommand(0xC1);    //Power control
  send8BitData(0x11);   //SAP[2:0];BT[3:0]

  sendCommand(0xC5);    //VCM control
  send8BitData(0x3d);
  send8BitData(0x20);

  sendCommand(0xC7);    //VCM control2
  send8BitData(0xAA); //0xB0

  sendCommand(0x36);    // Memory Access Control
  send8BitData(0x08);

  sendCommand(0x3A);
  send8BitData(0x55);

  sendCommand(0xB1);
  send8BitData(0x00);
  send8BitData(0x13);

  sendCommand(0xB6);    // Display Function Control
  send8BitData(0x0A);
  send8BitData(0xA2);

  sendCommand(0xF6);
  send8BitData(0x01);
  send8BitData(0x30);

  sendCommand(0xF2);    // 3Gamma Function Disable
  send8BitData(0x00);

  sendCommand(0x26);    //Gamma curve selected
  send8BitData(0x01);

  sendCommand(0xE0);    //Set Gamma
  send8BitData(0x0F);
  send8BitData(0x3F);
  send8BitData(0x2F);
  send8BitData(0x0C);
  send8BitData(0x10);
  send8BitData(0x0A);
  send8BitData(0x53);
  send8BitData(0XD5);
  send8BitData(0x40);
  send8BitData(0x0A);
  send8BitData(0x13);
  send8BitData(0x03);
  send8BitData(0x08);
  send8BitData(0x03);
  send8BitData(0x00);

  sendCommand(0XE1);    //Set Gamma
  send8BitData(0x00);
  send8BitData(0x00);

  send8BitData(0x10);
  send8BitData(0x03);
  send8BitData(0x0F);
  send8BitData(0x05);
  send8BitData(0x2C);
  send8BitData(0xA2);
  send8BitData(0x3F);
  send8BitData(0x05);
  send8BitData(0x0E);
  send8BitData(0x0C);
  send8BitData(0x37);
  send8BitData(0x3C);
  send8BitData(0x0F);

  sendCommand(0x11);    //Exit Sleep
  delay(120);
  sendCommand(0x29);    //Display on
  delay(50);

  // Clear screen
  setAddress(0,0,width-1, height-1); // All screen

  sendRepeatedData(height*width, BLACK);

  unSelect();
}

void newDmTftIli9341(DmTftIli9341 *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dc_port, uint16_t dc_pin) {
	_cs_port = cs_port;
	_cs_pin = cs_pin;
	_dc_port = dc_port;
	_dc_pin = dc_pin;

	handle->init = DmTftIli9341_init;
	handle->clearScreen = clearScreen;
	handle->drawChar = drawChar;
	handle->drawCircle = drawCircle;
	handle->drawHorizontalLine = drawHorizontalLine;
	handle->drawImage = drawImage;
	handle->drawLine = drawLine;
	handle->drawNumber = drawNumber;
	handle->drawPoint = drawPoint;
	handle->drawRectangle = drawRectangle;
	handle->drawString = drawString;
	handle->drawStringCentered = drawStringCentered;
	handle->drawTriangle = drawTriangle;
	handle->drawVerticalLine = drawVerticalLine;
	handle->fillCircle = fillCircle;
	handle->fillRectangle = fillRectangle;
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

