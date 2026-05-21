#pragma once

#include "string.h"

#include "driver/gpio.h"
#include <esp_timer.h>
#include "esp_log.h"

#include "lcd.h"
#include "menuSupport.h"


enum ButtonState{
    RELEASED,
    PRESSED,
    HOLD,
};

struct Button{
    enum ButtonState state;
    uint8_t pin;
};

void button1_isr(void *arg);
void button2_isr(void *arg);

void button1_output();
void button2_output();

void gpio_init();

bool ReadButtonState(struct Button *btn);
void ButtonStateMachine(struct Button *btn);

void MenuInit();

