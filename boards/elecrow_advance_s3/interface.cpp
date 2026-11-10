#include "core/powerSave.h"
#include "core/utils.h"
#include <interface.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ساخت یک درگاه SPI کاملا مستقل برای تاچ تا به صفحه نمایش دست نزنه
SPIClass touchSPI(FSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

/***************************************************************************************
** Function name: _setup_gpio()
** Location: main.cpp
** Description:   initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // روشن کردن فوری نور پس زمینه صفحه نمایش
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // بالا نگه داشتن CS کارت حافظه و تاچ برای جلوگیری از تداخل موقع بوت
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    bruceConfig.colorInverted = 0;
}

/***************************************************************************************
** Function name: _post_setup_gpio()
** Location: main.cpp
** Description:   second stage gpio setup to make a few functions work
***************************************************************************************/
void _post_setup_gpio() {
    // راه اندازی درگاه اختصاصی تاچ روی پین های 12 و 13 و 11 و 46
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    if (!touch.begin(touchSPI)) {
        Serial.println("Touch IC not Started");
    } else {
        Serial.println("Touch IC Started");
        // این همون خطیه که ارور داد! Pins جا افتاده بود که اضافه شد 👇
        touch.setRotation(bruceConfigPins.rotation);
    }

    // تنظیم PWM نور صفحه (سازگار با هر دو نسخه هسته ESP32)
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(TFT_BL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcWrite(TFT_BL, 255);
#else
    ledcSetup(TFT_BRIGHT_CHANNEL, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
    ledcAttachPin(TFT_BL, TFT_BRIGHT_CHANNEL);
    ledcWrite(TFT_BRIGHT_CHANNEL, 255);
#endif
}

/***************************************************************************************
** Function name: getBattery()
** location: display.cpp
** Description:   Delivers the battery value from 1-100
***************************************************************************************/
int getBattery() { return 0; }

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    int dutyCycle;
    if (brightval == 100) dutyCycle = 255;
    else if (brightval == 75) dutyCycle = 130;
    else if (brightval == 50) dutyCycle = 70;
    else if (brightval == 25) dutyCycle = 20;
    else if (brightval == 0) dutyCycle = 0;
    else dutyCycle = ((brightval * 255) / 100);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(TFT_BL, dutyCycle);
#else
    ledcWrite(TFT_BRIGHT_CHANNEL, dutyCycle);
#endif
}

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm = millis();
    if (!(millis() - tm > 200 || LongPress)) return;

    if (touch.touched()) {
        TS_Point p = touch.getPoint();

        // تبدیل مختصات خام تاچ XPT2046 (بین 200 تا 3800) به اندازه صفحه (480x320)
        int tx = map(p.x, 200, 3800, 0, tftWidth - 1);
        int ty = map(p.y, 200, 3800, 0, tftHeight - 1);

        // محدود کردن اعداد داخل کادر صفحه
        if (tx < 0) tx = 0;
        if (tx >= tftWidth) tx = tftWidth - 1;
        if (ty < 0) ty = 0;
        if (ty >= tftHeight) ty = tftHeight - 1;

        tm = millis();
        if (!wakeUpScreen()) AnyKeyPress = true;
        else return;

        touchPoint.x = tx;
        touchPoint.y = ty;
        touchPoint.pressed = true;
        touchHeatMap(touchPoint);
    } else {
        touchPoint.pressed = false;
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** location: mykeyboard.cpp
** Btn logic to turn off the device (name is odd btw)
**********************************************************************/
void checkReboot() {}
