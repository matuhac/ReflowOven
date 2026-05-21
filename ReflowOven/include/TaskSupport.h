#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


//predani hodnoty teploty z HeaterTask do LCDtask, pridat frontu
extern QueueHandle_t   HeaterCMDqueue;
extern QueueHandle_t   TempHeaterToLCD;
//Tasky
extern TaskHandle_t    HeaterTaskHandle;
extern TaskHandle_t    MenuTaskHandle; //
//Casovace
extern TimerHandle_t   TeplotaTimer;   //timer pro heaterTask
extern TimerHandle_t   LCD_RefreshTimer; //periodicky refresh LCD
extern TimerHandle_t   ButtonTimerUP;
extern TimerHandle_t   ButtonTimerDOWN;

