#include <stdio.h>
#include "main.h"
#include <DmTftIli9341.h>
#include <DmTouch.h>
#include "stm32_seq.h"
#include "app.h"

DmTftIli9341 tft;
DmTouch touch;

static void appTouch();
static void appUpdate();
static void drawIntro();
static void handleTouch(uint16_t x, uint16_t y);

// Initialization of TFT and Touch interfaces
void touchApp_Init () {
	// DmTft init
	setupDmTftIli9341(&tft, TFT_CS_GPIO_Port, TFT_CS_Pin, TFT_DC_GPIO_Port, TFT_DC_Pin);
	tft.init(240,320); // Tell the graphics layer about screen size

	drawIntro();

	// DmTouch
	setupDmTouch(&touch, T_CS_GPIO_Port, T_CS_Pin, T_IRQ_GPIO_Port, T_IRQ_Pin);

	UTIL_SEQ_RegTask(1<<CFG_TASK_TOUCHSCREEN_TOUCH_EVT_ID, UTIL_SEQ_RFU, appTouch);
	UTIL_SEQ_RegTask(1<<CFG_TASK_TOUCHSCREEN_UPDATE_EVT_ID, UTIL_SEQ_RFU, appUpdate);

	// We'll use IRQ to detect touch
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = T_IRQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; // Catch either falling or raising signal
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	changeSPIClock(SPI_BAUDRATEPRESCALER_16); // Slow down SPI clock to talk to XPT2046
	touch.setPenIRQ();

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static bool touchState = false;

// IRQ Handler for touch
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	UNUSED(GPIO_Pin);
	touchState = (HAL_GPIO_ReadPin(T_IRQ_GPIO_Port, T_IRQ_Pin) == GPIO_PIN_RESET);
	UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_TOUCH_EVT_ID, CFG_SCH_PRIO_0);

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, touchState?GPIO_PIN_SET:GPIO_PIN_RESET);
}

static enum {
	DRAW_INTRO,
	DRAW_NOTIFICATION
} appState;

// Called whenever the PENIRQ changes state
static void appTouch() {
	if (touchState == false) {
		return;
	}

	// Dealing with DmTouch need to slow down SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_16);

	uint16_t x,y;

	touchState = touch.readTouchData(&x, &y);
	touch.setPenIRQ();

	// Might not be valid after sampling
	if (touchState == false) {
		return;
	}

	handleTouch(x,y);
}

static ANCS_Notification *notification = NULL;

static void appUpdate() {
	// Dealing with DmTft need to speed up SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_4);

	appState = DRAW_NOTIFICATION;
	// Draw notification information
	tft.clearScreen(BLACK);
	tft.drawString(40, 10,"ANCS");

	if (notification) {
		switch(notification->categoryId) {
		case CategoryIDIncomingCall:
			tft.drawString(40, 30, "Appel entrant");
			break;
		case CategoryIDMissedCall:
			tft.drawString(40, 30, "Appel manqué");
			break;
		case CategoryIDNews:
			tft.drawString(40, 30, "News");
			break;
		case CategoryIDEmail:
			tft.drawString(40, 30, "Mail");
			break;
		case CategoryIDSocial:
			tft.drawString(40, 30, "Social");
			break;
		case CategoryIDOther:
			tft.drawString(40, 30, "Autre");
			break;
		default:
			tft.drawString(40, 30, "Divers");
			break;
		}
		tft.drawString(10, 60, notification->title);
		tft.drawString(10, 80, notification->message);
	}
}

// INTRO

static void drawIntro() {
	tft.clearScreen(BLACK);
	tft.drawString(40, 10,"ANCS");
}

static void handleTouch(uint16_t x, uint16_t y) {
	switch(appState) {
	case DRAW_INTRO:
		break;
	case DRAW_NOTIFICATION:
		notification = NULL;
		UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_UPDATE_EVT_ID, CFG_SCH_PRIO_0);
		break;
	}
}

// Called by ANCS client to notify
void TFTShowNotification(ANCS_Notification *lastNotification) {
	notification = lastNotification;
	UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_UPDATE_EVT_ID, CFG_SCH_PRIO_0);
}
