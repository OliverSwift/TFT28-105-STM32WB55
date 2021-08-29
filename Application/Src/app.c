#include <stdio.h>
#include "main.h"
#include <DmTftIli9341.h>
#include <DmTouch.h>

DmTftIli9341 tft;
DmTouch touch;

void appInit () {
	// DTft init
    setupDmTftIli9341(&tft, TFT_CS_GPIO_Port, TFT_CS_Pin, TFT_DC_GPIO_Port, TFT_DC_Pin);
    tft.init(240,320); // Tell the graphics layer about screen size
    tft.drawString(30, 150,"Touch me!");

    // DmTouch
    setupDmTouch(&touch, T_CS_GPIO_Port, T_CS_Pin, T_IRQ_GPIO_Port, T_IRQ_Pin);

    // We'll use IRQ to detect touch
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = T_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; // Catch either falling or raising signal
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    changeSPIClock(SPI_BAUDRATEPRESCALER_16); // Slow down SPI clock to talk to XPT2046
    touch.enableIrq();

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static bool touchChanged = false;
static bool touchState = false;

// IRQ Handler for touch
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	UNUSED(GPIO_Pin);
	touchChanged = true;
	touchState = (HAL_GPIO_ReadPin(T_IRQ_GPIO_Port, T_IRQ_Pin) == GPIO_PIN_RESET);

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, touchState?GPIO_PIN_SET:GPIO_PIN_RESET);
}

static enum {
	DRAW_CABIN,
	DRAW_IMAGE_JAPAN
} appState;

void appRun() {
    extern uint8_t japan_dat[];
	/*
    static int count = 0;

    tft.drawNumber(5, 40, count++, 6, false);
    tft.fillRectangle(5, 60, 5 + count % 200, 64, GREEN);

	uint16_t x,y;

	if (touch.readTouchData(&x,&y)) {
		printf("Touch @ %d,%d\n", x,y);
	}
    */

    if (touchChanged == false) {
    	return;
    }

    touchChanged = false;

    if (touchState == false) {
    	return;
    }

    // Dealing with DmTft need to speed up SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_4);

	switch(appState) {
	case DRAW_CABIN:
		tft.clearScreen(BLACK);
	    tft.drawString(5, 10,"  Romantic cabin");//Displays a string
	    int x=100,y=100;
	    tft.drawLine (x, y, x-80, y+30, YELLOW );//Draw line
	    tft.drawLine (x, y, x+80, y+30, YELLOW );
	    tft.drawLine (x-60, y+25, x-60, y+160, BLUE  );
	    tft.drawLine (x+60, y+25, x+60, y+160, BLUE  );
	    tft.drawLine (x-60, y+160, x+60, y+160,0x07e0  );
	    tft.drawRectangle(x-40, y+50, x-20, y+70, 0x8418);//Draw rectangle
	    tft.drawRectangle(x+40, y+50, x+20, y+70, 0x07ff);
	    tft.fillRectangle(x-20, y+100, x+20, y+160, BRIGHT_RED);//Draw fill rectangle
	    tft.drawLine (x, y+100, x, y+160, WHITE  );
	    tft.fillCircle(x+100, y-30, 20, RED );

	    appState = DRAW_IMAGE_JAPAN;
		break;
	case DRAW_IMAGE_JAPAN:
	    tft.drawImage(0,0,240,320, (uint16_t*)japan_dat);

	    appState = DRAW_CABIN;
		break;
	}
}
