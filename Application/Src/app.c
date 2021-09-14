#include <stdio.h>
#include "main.h"
#include <DmTftIli9341.h>
#include <DmTouch.h>
#include "stm32_seq.h"
#include "app.h"

DmTftIli9341 tft;
DmTouch touch;

// Menu functions
static void drawMenu();
static void handleMenu(uint16_t x, uint16_t y);

// The cabin and image functions
static void drawCabin();
static void drawImage();

// The freehand drawing tool functions
static bool firstPoint = true;
static uint16_t color = RED;
static void drawFree();
static void handleFree(uint16_t x, uint16_t y);

// BLE scanner functions
static void drawBLE();
static void handleBLE(uint16_t x, uint16_t y);

// Initialization of TFT and Touch interfaces
void touchApp_Init () {
	// DmTft init
	setupDmTftIli9341(&tft, TFT_CS_GPIO_Port, TFT_CS_Pin, TFT_DC_GPIO_Port, TFT_DC_Pin);
	tft.init(240,320); // Tell the graphics layer about screen size
	drawMenu();

	// DmTouch
	setupDmTouch(&touch, T_CS_GPIO_Port, T_CS_Pin, T_IRQ_GPIO_Port, T_IRQ_Pin);

	UTIL_SEQ_RegTask(1<<CFG_TASK_TOUCHSCREEN_EVT_ID, UTIL_SEQ_RFU, appRun);

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
	UTIL_SEQ_SetTask(1 << CFG_TASK_TOUCHSCREEN_EVT_ID, CFG_SCH_PRIO_0);

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, touchState?GPIO_PIN_SET:GPIO_PIN_RESET);
}

static enum {
	DRAW_MENU,
	DRAW_CABIN,
	DRAW_IMAGE_JAPAN,
	DRAW_FREEHAND,
	DRAW_BLE
} appState;

// Called whenever the PENIRQ changes state
void appRun() {
	if (touchState == false) {
		firstPoint = true; // For next touch in freehand tool
		return;
	}

	// Dealing with DmTouch need to slow down SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_16);

	uint16_t x,y;

	touchState = touch.readTouchData(&x, &y);
	touch.setPenIRQ();

	// Might not be valid after sampling
	if (touchState == false) {
		firstPoint = true; // For next touch in freehand
		return;
	}

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
	case DRAW_FREEHAND:
		handleFree(x,y);
		break;
	case DRAW_BLE:
		handleBLE(x,y);
		break;
	}
}

// MENU

typedef struct {
	uint16_t x,y;
	uint16_t w,h;
	const char *label;
	void (*cb)(void);
} Button;

static Button buttons[] = {
		// x    y    w    h  label        callback
		{ 40,  60, 160,  40, "The cabin", 	drawCabin},
		{ 40, 120, 160,  40, "Japan",     	drawImage},
		{ 40, 180, 160,  40, "Freehand",  	drawFree},
		{ 40, 240, 160,  40, "BLE scanner", drawBLE},
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

// CABIN

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

// THE JAPAN PICTURE

static void drawImage() {
	extern uint8_t japan_dat[];

	tft.drawImage(0,0,240,320, (uint16_t*)japan_dat);

	appState = DRAW_IMAGE_JAPAN;
}

// THE FREEHAND DRAWING TOOL

static void drawFree() {
	// Bottom toolbar
	tft.clearScreen(BLACK);
	tft.drawLine(0,300,240,300, RED);
	tft.drawString(30, 302, "Exit");

	// Color chooser
	int x = 120;
	tft.fillRectangle(x, 302, x+20, 320, RED);		x+=20;
	tft.fillRectangle(x, 302, x+20, 320, GREEN);	x+=20;
	tft.fillRectangle(x, 302, x+20, 320, BLUE);		x+=20;
	tft.fillRectangle(x, 302, x+20, 320, YELLOW);	x+=20;
	tft.fillRectangle(x, 302, x+20, 320, WHITE);	x+=20;

	appState = DRAW_FREEHAND;
}

static void handleFree(uint16_t x, uint16_t y) {

	if (y > 300) {
		if (x < 120) {
			drawMenu();
			appState = DRAW_MENU;
			firstPoint = true;
			return;
		} else {
			if (x < 140) {
				color = RED;
			} else if (x < 160) {
				color = GREEN;
			} else if (x < 180) {
				color = BLUE;
			} else if (x < 200) {
				color = YELLOW;
			} else if (x < 220) {
				color = WHITE;
			}
			firstPoint = true;
		}
	}

	static uint16_t lastX, lastY;

	if (!firstPoint) {
		tft.drawLine(lastX, lastY, x, y, color);
	}

	lastX = x;
	lastY = y;
	firstPoint = false;
}

// THE BLE SCANNER

#define MAX_ENTRIES 20
#define MAX_NAME_LENGTH 20
struct _ble_entry {
	uint8_t address[6];
	char local_name[MAX_NAME_LENGTH+1];
	uint8_t rssi;
};

static struct _ble_entry entries[MAX_ENTRIES];
static int nextMacIndex;
static uint16_t currentY;
static bool scanning = false;

#define NB_ENTRIES_PER_PAGE 8
static int currentPage, lastPage;

#define TOP_Y 20
#define BOTTOM_Y 300

static void drawBLE() {
	tft.clearScreen(BLACK);
	tft.drawString(0, 0,"BLE Scanner");
	tft.drawLine(0,TOP_Y-2,240,TOP_Y-2, BLUE);
	tft.drawLine(0,BOTTOM_Y,240,BOTTOM_Y, RED);

	nextMacIndex = 0;
	memset(entries, 0, sizeof(entries));

	// Ask BLE task to start active scanning
	scanning = true;

	appState = DRAW_BLE;
}

static void drawResultPage() {
	char line[30];
	int start,end;

	// Dealing with DmTft need to speed up SPI clock
	changeSPIClock(SPI_BAUDRATEPRESCALER_4);

	// Clear page
	tft.fillRectangle(0, TOP_Y, 240, BOTTOM_Y-1, BLACK);
	currentY = TOP_Y;

	start = currentPage * NB_ENTRIES_PER_PAGE;
	end = start + NB_ENTRIES_PER_PAGE;
	if (end > nextMacIndex) end = nextMacIndex;

	// Draw page entries
	for(int e = start; e < end; e++) {
		uint8_t *address = entries[e].address;

		snprintf(line, sizeof(line), "%02X:%02X:%02X:%02X:%02X:%02X   %4ddBm", address[5], address[4], address[3], address[2], address[1], address[0], entries[e].rssi-256);
		tft.setTextColor(BLACK, GREEN);
		tft.drawString(0, currentY, line);
		currentY += 16;

		if (entries[e].local_name[0]) {
			tft.setTextColor(BLACK, YELLOW);
			tft.drawString(10, currentY, entries[e].local_name);
			currentY += 16;
		}
		tft.setTextColor(BLACK, WHITE);
		currentY += 4;
	}

	// If more to come, invite to tap
	if (currentPage != lastPage) {
		tft.setTextColor(WHITE, BLACK);
		tft.drawString(20, currentY+10, "   tap to continue   ");
		tft.setTextColor(BLACK, WHITE);
	}
}

static void handleBLE(uint16_t x, uint16_t y) {
	if (scanning) return;

	if (y > BOTTOM_Y) {
		if (x > 120) {
			drawMenu(); // Hit Exit
			appState = DRAW_MENU;
		} else {
			drawBLE(); // Hit refresh
		}
	} else 	if (currentPage != lastPage) {
		// Show next result page when tapped
		currentPage++;
		drawResultPage();
	}
}

// Called from BLE application (app_ble.c) for each ADV_IND or SCAN_RSP packets
void logBLE(BLEInfo info, uint8_t rssi, uint8_t *address, const char *local_name) {
	char line[30];

	if (info == BLE_Entry) {
		if (nextMacIndex == MAX_ENTRIES) return;

		//Check if already seen
		for(int m = 0; m < nextMacIndex; m++) {
			if (memcmp(address, entries[m].address, 6)==0) {
				// Already seen but may refine visible name
				if (entries[m].local_name[0] == 0 && local_name[0]) {
					strcpy(entries[m].local_name, local_name);
				}
				return; // Already seen
			}
		}

		// New entry
		memcpy(entries[nextMacIndex].address, address, 6);
		strcpy(entries[nextMacIndex].local_name, local_name);
		entries[nextMacIndex].rssi = rssi;
		nextMacIndex++;

		// Dealing with DmTft need to speed up SPI clock
		changeSPIClock(SPI_BAUDRATEPRESCALER_4);
		snprintf(line, sizeof(line), "%d device%s found.", nextMacIndex, nextMacIndex>1?"s":"");
		tft.drawString(10, (TOP_Y+BOTTOM_Y)/2-8, line);
	}

	if (info == BLE_Stop) {
		currentPage = 0;
		lastPage = nextMacIndex / NB_ENTRIES_PER_PAGE;
		if (nextMacIndex % NB_ENTRIES_PER_PAGE == 0) {
			lastPage--;
		}
		drawResultPage();
		tft.drawString(0, BOTTOM_Y+1, "Refresh        Exit");
		scanning = false;
	}
}
