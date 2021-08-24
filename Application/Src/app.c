#include <DmTftIli9341.h>

DmTftIli9341 tft;

void appInit () {
    newDmTftIli9341(&tft, TFT_CS_GPIO_Port, TFT_CS_Pin, TFT_DC_GPIO_Port, TFT_DC_Pin);

    tft.init(240,320);
    tft.drawString(5, 10,"  Romantic cabin");//Displays a string
    int x=100,y=100;
    tft.drawLine (x, y, x-80, y+30, YELLOW );//Draw line
    delay(1000);
    tft.drawLine (x, y, x+80, y+30, YELLOW );
    delay(1000);
    tft.drawLine (x-60, y+25, x-60, y+160, BLUE  );
    delay(1000);
    tft.drawLine (x+60, y+25, x+60, y+160, BLUE  );
    delay(1000);
    tft.drawLine (x-60, y+160, x+60, y+160,0x07e0  );
    delay(1000);
    tft.drawRectangle(x-40, y+50, x-20, y+70, 0x8418);//Draw rectangle
    delay(1000);
    tft.drawRectangle(x+40, y+50, x+20, y+70, 0x07ff);
    delay(1000);
    tft.fillRectangle(x-20, y+100, x+20, y+160, BRIGHT_RED);//Draw fill rectangle
    delay(1000);
    tft.drawLine (x, y+100, x, y+160, WHITE  );
    delay(1000);
    tft.fillCircle(x+100, y-30, 20, RED );
    delay(1000);
}
