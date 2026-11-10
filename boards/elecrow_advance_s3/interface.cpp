#include <core/power.h>
#include <globals.h>
#include <interface.h>
#include <SPI.h>

SPIClass touchSPI(HSPI);

void _setup_gpio() {
    pinMode(46, OUTPUT);
    digitalWrite(46, LOW);

    pinMode(45, OUTPUT);
    digitalWrite(45, HIGH);
    touchSPI.begin(12, 13, 11, 45);
}

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
#include <soc/soc_caps.h>

void _post_setup_gpio() {
    brightness = 100;
    tft.setBrightness(brightness);
}

static uint16_t readXPT2046(uint8_t cmd) {
    touchSPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(45, LOW);
    touchSPI.transfer(cmd);
    uint8_t b1 = touchSPI.transfer(0);
    uint8_t b2 = touchSPI.transfer(0);
    digitalWrite(45, HIGH);
    touchSPI.endTransaction();
    return ((b1 << 8) | b2) >> 3;
}

void TouchFooter(uint16_t color) {
    tft.drawRect(0, tftHeight - 20, tftWidth / 3, 20, color);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.drawCentreString("PREV", tftWidth / 6, tftHeight - 4, 1);

    tft.drawRect(tftWidth / 3, tftHeight - 20, tftWidth / 3, 20, color);
    tft.drawCentreString("SEL", tftWidth / 2, tftHeight - 4, 1);

    tft.drawRect((tftWidth / 3) * 2, tftHeight - 20, tftWidth / 3, 20, color);
    tft.drawCentreString("NEXT", (tftWidth / 6) * 5, tftHeight - 4, 1);
}

void TouchFooter2(uint16_t color) {
    tft.drawStartButton();
}

void InputHandler(void) {
    static unsigned long tm = 0;
    if (millis() - tm < 150) return;

    int z = readXPT2046(0xB1);
    if (z > 200 && z < 4000) {
        int raw_x = readXPT2046(0xD1);
        int raw_y = readXPT2046(0x91);

        if (raw_x > 100 && raw_y > 100) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

            int x = map(raw_y, 200, 3800, 0, tftWidth);
            int y = map(raw_x, 200, 3800, 0, tftHeight);
            if (x < 0) x = 0;
            if (x >= tftWidth) x = tftWidth - 1;
            if (y < 0) y = 0;
            if (y >= tftHeight) y = tftHeight - 1;

            touchPoint.x = x;
            touchPoint.y = y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        }
    }
END:
    yield();
}

void powerOff() {}
void checkReboot() {}
