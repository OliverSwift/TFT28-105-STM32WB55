/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/

#include "DmTouch.h"
#include <stdlib.h>

extern SPI_HandleTypeDef hspi1;

static GPIO_TypeDef *_cs_port;
static uint16_t _cs_pin;
static GPIO_TypeDef *_irq_port;
static uint16_t _irq_pin;
static uint8_t _samplesPerMeasurement = 8;
static CalibrationMatrix _calibrationMatrix = {
		.a = 260, .b = 0, .c = -8,
		.d = 0,	.e = 360,	.f = -12
};
static bool _penirq = false;

// Local functions Declarations
static bool isTouched();
static void getAverageXY(uint16_t *x, uint16_t *y);

// Local Functions
static void DmTouch_select() {
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
}

static void DmTouch_unSelect() {
	HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

static void spiWrite(uint8_t data) {
	HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
}

static uint8_t spiRead() {
	uint8_t data;
	HAL_SPI_Receive(&hspi1, &data, 1, 1000);
	return data;
}

static void DmTouch_init() {
}

static void preparePenIRQ() {
	DmTouch_select();
	spiWrite(0x80); // Drivers Off
	_penirq = true;
	DmTouch_unSelect();
}

static uint16_t readData12(uint8_t command) {
	uint16_t value = 0;

	spiWrite(command); // Send command

	// 12bit value, [14:3]
	value = spiRead()<<5;
	value |= spiRead()>>3;
	return value;
}

static void readRawData(uint16_t *x, uint16_t *y) {
	DmTouch_select();
	*y = readData12(0x90);
	DmTouch_unSelect();  // MUST set /CS to 0 prior to next conversion

	DmTouch_select();
	*x = readData12(0xD0);
	DmTouch_unSelect();
}

static bool readTouchData(uint16_t *posX, uint16_t *posY) {
	uint16_t x,y;

	getAverageXY(&x, &y);

	*posX = (x * _calibrationMatrix.a + y * _calibrationMatrix.b) / RESCALE_FACTOR + _calibrationMatrix.c;
	*posY = (x * _calibrationMatrix.d + y * _calibrationMatrix.e) / RESCALE_FACTOR + _calibrationMatrix.f;

	delay(20); // PENIRQ delay
	if (!isTouched()) {
		return false;
	}

	return true;
}

static bool isTouched() {
	return HAL_GPIO_ReadPin(_irq_port, _irq_pin) == GPIO_PIN_RESET;
}

static void getAverageXY(uint16_t *x, uint16_t *y) {
	uint32_t sumX = 0;
	uint32_t sumY = 0;
	uint16_t sampleX, sampleY;
	uint16_t lastX, lastY;

#define MAX_DELTA 200

	readRawData(&lastX,&lastY);

	for (int i=0; i<_samplesPerMeasurement; i++) {
		bool ok = false;
		while(!ok) {
			readRawData(&sampleX,&sampleY);
			if (abs((signed)sampleX-(signed)lastX) < MAX_DELTA && abs((signed)sampleY-(signed)lastY) < MAX_DELTA) {
				ok = true;
			}
			lastX = sampleX;
			lastY = sampleY;
		}
		sumX += sampleX;
		sumY += sampleY;
	}

	*x = (uint32_t)sumX/_samplesPerMeasurement;
	*y = (uint32_t)sumY/_samplesPerMeasurement;
}

// Total number of samples = MEASUREMENTS * _samplesPerMeasurement
static void setPrecison(uint8_t samplesPerMeasurement) {
	_samplesPerMeasurement = samplesPerMeasurement;
}

static void setCalibrationMatrix(CalibrationMatrix calibrationMatrix) {
	_calibrationMatrix = calibrationMatrix;
}

void setupDmTouch(DmTouch *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin) {
	_cs_port = cs_port;
	_cs_pin = cs_pin;
	_irq_port = irq_port;
	_irq_pin = irq_pin;

	handle->init = DmTouch_init;
	handle->setPenIRQ = preparePenIRQ;
	handle->readTouchData = readTouchData;
	handle->isTouched = isTouched;
	handle->setCalibrationMatrix = setCalibrationMatrix;
	handle->setPrecison = setPrecison;
}
