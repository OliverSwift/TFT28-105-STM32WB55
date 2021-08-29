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

#define MEASUREMENTS 15
#define ERR_RANGE 20

extern SPI_HandleTypeDef hspi1;

static GPIO_TypeDef *_cs_port;
static uint16_t _cs_pin;
static GPIO_TypeDef *_irq_port;
static uint16_t _irq_pin;
static uint8_t _samplesPerMeasurement = 5;
static CalibrationMatrix _calibrationMatrix;
static int _irq = -1;

// Local functions Declarations
static bool isTouched();
static bool getMiddleXY(uint16_t *x, uint16_t *y);
static void getAverageXY(uint16_t *x, uint16_t *y);
static uint16_t getDisplayCoordinateX(uint16_t x_touch, uint16_t y_touch);
static uint16_t getDisplayCoordinateY(uint16_t x_touch, uint16_t y_touch);
static uint16_t calculateMiddleValue(uint16_t values[], uint8_t count);

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

static void enableIrq() {
	DmTouch_select();
	spiWrite(0x80); // Enable PENIRQ
	_irq = 1;
	DmTouch_unSelect();
}

static uint16_t readData12(uint8_t command) {
	uint8_t temp = 0;
	uint16_t value = 0;

	spiWrite(command); // Send command
	//delayMicroseconds(10); // Test

	// We use 7-bits from the first byte and 5-bit from the second byte
	temp = spiRead();
	value = temp<<8;
	temp = spiRead();
	value |= temp;
	value >>=3;
	value &= 0xFFF;
	return value;
}

static void readRawData(uint16_t *x, uint16_t *y) {
	DmTouch_select();
	*y = readData12(0x90);
	DmTouch_unSelect();
	DmTouch_select();
	*x = readData12(0xD0);
	DmTouch_unSelect();
}

static bool readTouchData(uint16_t *posX, uint16_t *posY) {
	if (!isTouched()) {
		return false;
	}

	getAverageXY(posX, posY);

	/*

  uint16_t touchX, touchY;
  getMiddleXY(&touchX,&touchY);

	 *posX = getDisplayCoordinateX(touchX, touchY);
	 *posY = getDisplayCoordinateY(touchX, touchY);

	 */

	return true;
}

static bool readTouchData2(uint16_t *posX, uint16_t *posY) {
	if (!isTouched()) {
		return false;
	}

	uint16_t touchX1, touchY1;
	uint16_t touchX2, touchY2;
	uint16_t touchX, touchY;

	getMiddleXY(&touchX1,&touchY1);
	delay(3);
	getMiddleXY(&touchX2,&touchY2);

	if (((touchX2 <= touchX1 && touchX1 < touchX2 + ERR_RANGE)||(touchX1 <= touchX2 && touchX2 < touchX1 + ERR_RANGE))
			&& ((touchY2 <= touchY1 && touchY1 < touchY2 + ERR_RANGE)||(touchY1 <= touchY2 && touchY2 < touchY1 + ERR_RANGE))) {
		touchX = (touchX1 + touchX2)>>1;
		touchY = (touchY1 + touchY2)>>1; 	
		*posX = getDisplayCoordinateX(touchX, touchY);
		*posY = getDisplayCoordinateY(touchX, touchY);
		return true;
	}
	return false;
}


static bool isSampleValid() {
	uint16_t sampleX,sampleY;
	readRawData(&sampleX,&sampleY);
	if (sampleX > 20 && sampleX < 4095 && sampleY > 20 && sampleY < 4095) {
		return true;
	} else {
		return false;
	}
}

static bool isTouched() {
	if (_irq == -1) {
		return isSampleValid();
	}

	return HAL_GPIO_ReadPin(_irq_port, _irq_pin) == GPIO_PIN_RESET;
}

static bool getMiddleXY(uint16_t *x, uint16_t *y) {
	bool haveAllMeasurements  = true;
	uint16_t valuesX[MEASUREMENTS];
	uint16_t valuesY[MEASUREMENTS];

	for (int i=0; i<MEASUREMENTS; i++) {
		getAverageXY(&valuesX[i], &valuesY[i]);
	}
	*x = calculateMiddleValue(valuesX, MEASUREMENTS);
	*y = calculateMiddleValue(valuesY, MEASUREMENTS);

	return haveAllMeasurements;
}

static void getAverageXY(uint16_t *x, uint16_t *y) {
	uint32_t sumX = 0;
	uint32_t sumY = 0;
	uint16_t sampleX,sampleY;

	readRawData(&sampleX,&sampleY);

	for (int i=0; i<_samplesPerMeasurement; i++) {
		readRawData(&sampleX,&sampleY);
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

static uint16_t getDisplayCoordinateX(uint16_t x_touch, uint16_t y_touch) {
	uint16_t Xd;
	float temp;
	temp = (_calibrationMatrix.a * x_touch + _calibrationMatrix.b * y_touch + _calibrationMatrix.c) / RESCALE_FACOTR;
	Xd = (uint16_t)(temp);
	if (Xd > 60000) {
		Xd = 0;
	}
	return Xd;
}

static uint16_t getDisplayCoordinateY(uint16_t x_touch, uint16_t y_touch) {
	uint16_t Yd;
	float temp;
	temp = (_calibrationMatrix.d * x_touch + _calibrationMatrix.e * y_touch + _calibrationMatrix.f) / RESCALE_FACOTR;
	Yd = (uint16_t)(temp);
	if (Yd > 60000) {
		Yd = 0;
	}
	return Yd;
}

static uint16_t calculateMiddleValue(uint16_t values[], uint8_t count) {
	uint16_t temp;

	for(uint8_t i=0; i<count-1; i++) {
		for(uint8_t j=i+1; j<count; j++) {
			if(values[j] < values[i]) {
				temp = values[i];
				values[i] = values[j];
				values[j] = temp;
			}
		}
	}

	if(count%2==0) {
		return((values[count/2] + values[count/2 - 1]) / 2.0);
	} else {
		return values[count/2];
	}
}

void setupDmTouch(DmTouch *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin) {
	_cs_port = cs_port;
	_cs_pin = cs_pin;
	_irq_port = irq_port;
	_irq_pin = irq_pin;

	handle->init = DmTouch_init;
	handle->enableIrq = enableIrq;
	handle->readTouchData = readTouchData;
	handle->readTouchData2 = readTouchData2;
	handle->isTouched = isTouched;
	handle->getMiddleXY = getMiddleXY;
	handle->setCalibrationMatrix = setCalibrationMatrix;
	handle->setPrecison = setPrecison;
}
