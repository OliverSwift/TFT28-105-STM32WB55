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

static void inplaceUTF8toLatin1(uint8_t *text) {
	uint8_t *current = text;
	uint32_t utf_code;

	printf("Buffer: ");
	for(uint8_t *c = text; *c; c++) {
		printf("%02X ", *c);
	}
	printf("\n");

	while(*current) {
		if ((*current & 0xE0) == 0xC0) {
			utf_code  = (current[0] & 0x0F) << 6;
			utf_code |= (current[1] & 0x3F);
			current+=2;
		} else if ((*current & 0xF0) == 0xE0) {
			utf_code  = (current[0] & 0x0F) << 12;
			utf_code |= (current[1] & 0x3F) << 6;
			utf_code |= (current[2] & 0x3F);
			current+=3;
		} else if ((*current & 0xF8) == 0xF0) {
			utf_code  = (current[0] & 0x0F) << 18;
			utf_code |= (current[1] & 0x3F) << 12;
			utf_code |= (current[2] & 0x3F) << 6;
			utf_code |= (current[3] & 0x3F);
			current+=4;
		} else {
			utf_code = current[0] & 0x7F;
			current++;
		}

		if (utf_code < 256) {
			*text = utf_code;
		} else if (utf_code == 0x2019) {
			*text = '\'';
		} else {
			*text = '?';
		}
		text++;
	}
	*text = 0;
}

static ANCS_Notification notification;

static void appUpdate() {
	// Dealing with DmTft need to speed up SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_4);

	appState = DRAW_NOTIFICATION;
	// Draw notification information
	tft.clearScreen(BLACK);

	tft.setTextColor(BLACK, WHITE);
	tft.drawString(40, 10,"   ANCS demo   ");

	if (notification.title[0]) {
		tft.setTextColor(BLACK, BLUE);
		switch(notification.categoryId) {
		case CategoryIDIncomingCall:
			tft.drawStringCentered(0, 30, 240, 20, "Incoming call");
			break;
		case CategoryIDMissedCall:
			tft.setTextColor(BLACK, BRIGHT_RED);
			tft.drawStringCentered(0, 30, 240, 20, "Missed call");
			break;
		case CategoryIDNews:
			tft.drawStringCentered(0, 30, 240, 20, "News");
			break;
		case CategoryIDEmail:
			tft.drawStringCentered(0, 30, 240, 20, "Mail");
			break;
		case CategoryIDSocial:
			tft.drawStringCentered(0, 30, 240, 20, "Message");
			break;
		default:
			tft.drawStringCentered(0, 30, 240, 20, "Misc.");
			break;
		}

		tft.setTextColor(BLACK, YELLOW);
		tft.drawString(10, 60, notification.title);

		tft.setTextColor(BLACK, 0xC618);
		tft.drawString(0, 100, notification.message);
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
		notification.title[0] = 0;
		UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_UPDATE_EVT_ID, CFG_SCH_PRIO_0);
		break;
	}
}

// Called by ANCS client to notify
void TFTShowNotification(ANCS_Notification *lastNotification) {
	notification = *lastNotification;
	inplaceUTF8toLatin1((uint8_t*)notification.title);
	inplaceUTF8toLatin1((uint8_t*)notification.message);
	UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_UPDATE_EVT_ID, CFG_SCH_PRIO_0);
}
