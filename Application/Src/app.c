#include <stdio.h>
#include "main.h"
#include <DmTftIli9341.h>
#include <DmTouch.h>

DmTftIli9341 tft;
DmTouch touch;

static void drawMenu();
static void handleMenu(uint16_t x, uint16_t y);

static void drawCabin();
static void drawImage();

static bool firstPoint = true;
static void drawFree();
static void handleFree(uint16_t x, uint16_t y);

void appInit () {
	// DTft init
	setupDmTftIli9341(&tft, TFT_CS_GPIO_Port, TFT_CS_Pin, TFT_DC_GPIO_Port, TFT_DC_Pin);
	tft.init(240,320); // Tell the graphics layer about screen size
	drawMenu();

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
	DRAW_MENU,
	DRAW_CABIN,
	DRAW_IMAGE_JAPAN,
	DRAW_FREE,
} appState;

void appRun() {
	if (touchChanged == false) {
		return;
	}

	touchChanged = false;

	if (touchState == false) {
		firstPoint = true; // For next touch in handfree
		return;
	}

	// Dealing with DmTouch need to slow down SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_16);

	uint16_t x,y;

	touchState = touch.readTouchData(&x, &y);
	touch.enableIrq();

	// Might not be valid after sampling
	if (touchState == false) {
		firstPoint = true; // For next touch in handfree
		return;
	}

	//printf("Touch @ %d,%d\n", x,y);

	// Dealing with DmTft need to speed up SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_4);

	switch(appState) {
	case DRAW_MENU:
		handleMenu(x,y);
		break;
	case DRAW_CABIN:
		drawMenu();
		appState = DRAW_MENU;
		break;
	case DRAW_IMAGE_JAPAN:
		drawMenu();
		appState = DRAW_MENU;
		break;
	case DRAW_FREE:
		handleFree(x,y);
		break;
	}
}

typedef struct {
	uint16_t x,y;
	uint16_t w,h;
	const char *label;
	void (*cb)(void);
} Button;

static Button buttons[] = {
		// x    y    w    h  label        callback
		{ 40,  60, 160,  40, "The cabin", drawCabin},
		{ 40, 140, 160,  40, "Japan",     drawImage},
		{ 40, 220, 160,  40, "Free hand", drawFree},
};

#define NB_BUTTONS (sizeof(buttons)/sizeof(buttons[0]))

static void drawMenu() {
	tft.clearScreen(BLACK);
	tft.drawString(40, 10,"Menu");//Displays a string

	for(int b = 0; b < NB_BUTTONS; b++) {
		tft.drawRectangle(buttons[b].x, buttons[b].y, buttons[b].x + buttons[b].w - 1, buttons[b].y + buttons[b].h - 1, GREEN);
		tft.drawString(buttons[b].x + 10, buttons[b].y + buttons[b].h / 2 - 8, buttons[b].label);
	}
}

static void handleMenu(uint16_t x, uint16_t y) {
	for(int b = 0; b < NB_BUTTONS; b++) {
		if (x > buttons[b].x && x < buttons[b].x + buttons[b].w
				&& y > buttons[b].y && y < buttons[b].y + buttons[b].h) {
			buttons[b].cb();
			break;
		}
	}
}

static void drawCabin(){
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

	appState = DRAW_CABIN;
}

static void drawImage() {
	extern uint8_t japan_dat[];

	tft.drawImage(0,0,240,320, (uint16_t*)japan_dat);

	appState = DRAW_IMAGE_JAPAN;
}

static void drawFree() {
	tft.clearScreen(BLACK);
	tft.drawLine(0,300,240,300, RED);
	tft.drawString(30, 302, "Exit");

	appState = DRAW_FREE;
}

static void handleFree(uint16_t x, uint16_t y) {

	if (y > 300) {
		drawMenu();
		appState = DRAW_MENU;
		firstPoint = true;
		return;
	}

	static uint16_t lastX, lastY;

	if (!firstPoint) {
		tft.drawLine(lastX, lastY, x, y, BLUE);
	}

	lastX = x;
	lastY = y;
	firstPoint = false;
}

