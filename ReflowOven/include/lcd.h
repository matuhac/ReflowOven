#pragma once

#include "driver/i2c_master.h"
#include "esp_task.h"

enum MenuState{
    BASIC,  //menu nema cislice
    CHANGE, //zmena cislic
    START,  ///START menu
    PROCESS,//ohrevove menu
    ERR, //chybove menu
    EXIT,
};

struct menu{
    char lines[4][20]; //4 linky displeje, 20 znaku
    //char *lines[4];
    struct menu *childMenus[4];
    struct menu *parentMenu;

    int maxCursor;
    int minCursor;

    enum MenuState state;
};

void lcd_sendPulse(uint8_t data);
void lcd_sendNibble(uint8_t nibble, uint8_t control);
void lcd_sendByte(uint8_t byte, uint8_t mode);
void lcd_init();

void lcd_sendCommand( uint8_t cmd);
void lcd_setCursor(uint8_t col, uint8_t row);

void lcd_writeData(uint8_t data);
void lcd_writeChar(char c);
void lcd_writeString(const char *str);


void init();