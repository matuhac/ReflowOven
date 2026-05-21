//Standardni knihovny
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "string.h"
//esp includy
#include "esp_log.h"
#include <esp_timer.h>
//FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
//Moje knihovny
#include <MAX6675.h>
#include "lcd.h"
#include "Heater.h"
#include "wifi.h"
#include "TaskSupport.h"
#include "menu.h"
//WIFI includy
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
//ostatni

//period of software tasks
#define HEATER_TIMER_PERIOD 1000
#define LCD_TIMER_PERIOD    100 //250
#define BUTTON_TASK_PERIOD  20
#define FILTER_TASK_PERIOD  250
//software timer periods
#define BUTTON_TIMER_PERIOD_1 500
#define BUTTON_TIMER_PERIOD_2 500
#define BUTTON_TIMER_PERIOD_3 200
#define BUTTON_TIMER_PERIOD_4 50
#define BUTTON_TIMER_PERIOD_5 50

#define WORK_TIMER_CYCLE 10000
//queue sizes
#define QUEUE_LENGTH        10
//stack sizes
#define STACK_TEPLOMER_TASK 2048*2
#define STACK_LCD_TASK      2048*2
#define STACK_MENU_TASK     2048*2

#define MAX_STRING_LENGTH 30

#define CELSIUS 0xDF //symbol for celsius degree

#define ALPHA 0.5

struct MAX Teplomer;

static const char *TAG = "MAIN";

//predani hodnoty teploty z HeaterTask do LCDtask, pridat frontu
QueueHandle_t   TempFilterQueue;
QueueHandle_t   HeaterCMDqueue; //prijem commandu z wifi
QueueHandle_t   TempHeaterToLCD;
//Tasky
TaskHandle_t    Heater_Comm_TaskHandle;
TaskHandle_t    Heater_Control_TaskHandle;
TaskHandle_t    MenuTaskHandle;
TaskHandle_t    TempFilterTaskHandle;
//Casovace
TimerHandle_t   TeplotaTimer;
TimerHandle_t   LCD_RefreshTimer;
TimerHandle_t   ButtonTimerUP;
TimerHandle_t   ButtonTimerDOWN;

TimerHandle_t WorkTimer_ON;
TimerHandle_t WorkTimer_OFF;

uint32_t OFF_Time = 10;
uint32_t ON_Time = 10;

//pro software casovace, upravuje jejich periody
uint16_t counter1 = 0;
uint16_t counter2 = 0;
uint32_t timerPresetArray[5] = {BUTTON_TIMER_PERIOD_1, BUTTON_TIMER_PERIOD_2, BUTTON_TIMER_PERIOD_3, BUTTON_TIMER_PERIOD_4, BUTTON_TIMER_PERIOD_5};
//ukazatele na pole casovacu
int8_t PresetUP = 0;
int8_t PresetDOWN = 0;

//promena drzi posledni validni udaj teploty
bool filter_initialized = false;
//pokud je detekovana
//extern bool ClientConnected;

//signalizace chyby
int8_t error = 0;


//jednotliva menu pro vykreslovani LCD displeje
struct menu *currentMenu;

extern struct menu introMenu;
extern struct menu startMenu;
extern struct menu settingsMenu;
extern struct menu stepPREHEAT;
extern struct menu stepSOAK;
extern struct menu stepREFLOW;
extern struct menu stepCOOLING;
extern struct menu heatingMenu;
extern struct menu errorMenu;
extern struct menu exitMenu;

extern volatile uint8_t cursor;
struct Button btn1;
struct Button btn2;
struct Button btn3;
struct Button btn4;

extern volatile uint32_t timer_seconds;

esp_timer_handle_t ms_timer_handle;
volatile uint32_t ms_timer = 0;

//pouze debug
uint64_t MenuTaskExec = 0;
uint64_t HeaterTaskExec = 0;
uint64_t FilterTaskExec = 0;

//pomocny profil pro display hodnot v menu
struct reflow_profile_t profile_copy = {
    .name = "Kopie",
    .numberOfSteps = 4,
    .steps = {{25.0, 200}, {25.0, 5}, {30.0, 150}, {25, 100}},
};
//tato promena je klicova, drzi vetsinu hodnot pouzivanou peci, v podstate cely program ridi
//nove hodnoty - Kp = 0.7463, Ti = 284.6569, Td = 12.3916, N = 0.4899
//puvodni hodnoty - Kp = 1.5, Ti = 0.25, Td = 0.5 N = NIC
struct HeaterMachine Heater = {
    //             temp    Kp     Ti Ts prev set prev_err out   max  min , Kp = 0.7085, Ti = 270.2573
    //.controller = {20.0, 0.70, 25.0, 1.0, 0.0, 0.0, 0, 0, 4095, 0},
    .controller = {
        .Kp             = 35.29,
        .Ti             = 490.0,
        .Td             = 8.41,
        .Ts             = 10.0,
        .N              = 0.11,
        .Tt             = 2.5,
        .dyn_omez       = 0.0,
        .temperature    = 20, //pocatecni hodnota, nahradit merenim
        .setpoint       = 0,
        .out_max        = 4095,
        .out_min        = 0,
        .output         = 0, //pocatecni hodnota vystupu
        .prev_err       = 0,
        .prev_prev_err  = 0,
        .prev_I         = 0, //sumator, bude nahrazen 
        .prev_D         = 0,  
    },
    //.reflow_profile =  {"MainProfile", 4, {{50.0, 200}, {50.0, 60}, {100.0, 150}, {30.0, 100}}},
    .reflow_profile =  {"MainProfile", 4, {{100.0, 200}, {100.0, 60}, {30.0, 150}, {35, 100}}},
    .MachineState = IDLE,
    .profileState = PREHEAT,
};


//bool hodnoty
volatile bool LCD_Refresh = 0;
bool LCD_ClearMenu = 0;
//signalize zapnuti/vypnuti chodu
volatile bool HeaterSetOFF = 0;
volatile bool HeaterSetON = 0;

bool updateReflowParam = 1;
bool updateControllerParam = 1;

//deklarace funkci
void QueueDeclaration(); //deklarace front
void TaskDeclaration(); //deklarace Tasku
void TimerDeclaration(); //deklarace software casovacu.
//Callbacky
void HeaterTimerCallback(TimerHandle_t xTimer);
void LCD_TimerCallback(TimerHandle_t xTimer);
void ButtonUP_TimerCallback(TimerHandle_t xTimer);
void ButtonDOWN_TimerCallback(TimerHandle_t xTimer);

void WorkTimer_ON_TimerCallback(TimerHandle_t xTimer);
void WorkTimer_OFF_TimerCallback(TimerHandle_t xTimer);

//Tasky
void HeaterTask_Comm(void *pvParameters);
void HeaterTask_Control(void *pvParameters);
void MenuTask(void *pvParameters);
void SD_Task(void *pvParameters);
void FilterTask(void *pvParameters);

void MoveToParentMenu();
void LCD_Draw();
float Filter(float new_sample, float prev_sample);

void timer_callback(void* arg);

void app_main() {

    MAX_Init(&Teplomer, -1, 19, 18, 5); //inicializace struktury pro teplomer
    HeaterInit();
    lcd_init();
    gpio_init();
    MenuInit();

    //high resolution timer
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "1ms_Timer",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &ms_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(ms_timer_handle, 1000));

    QueueDeclaration();
    TaskDeclaration();
    TimerDeclaration();
    WifiInit();


}

void QueueDeclaration(){

    TempFilterQueue = xQueueCreate(1, sizeof(float));
    if(TempFilterQueue == NULL){
        ESP_LOGI(TAG, "TempFilter queue creation failed");
        return;
    }

    HeaterCMDqueue = xQueueCreate(QUEUE_LENGTH, MAX_STRING_LENGTH);
    if(HeaterCMDqueue == NULL){
        ESP_LOGI(TAG, "CMD queue creation failed");
        return;
    }

    TempHeaterToLCD = xQueueCreate(1, sizeof(float));
    if(TempHeaterToLCD == NULL){
        ESP_LOGI(TAG, "TempHeaterToLCD queue creation failed");
        return;
    }
}

void TaskDeclaration(){
    BaseType_t ret;
    //vytvoreni Tasku
    ret = xTaskCreate(HeaterTask_Comm, "heaterTask_Comm", STACK_TEPLOMER_TASK, NULL, 7, &Heater_Comm_TaskHandle);
    if(ret != pdPASS){
        ESP_LOGI(TAG, "Failed to create HeaterTask_COMM");
        return;
    }

    ret = xTaskCreate(HeaterTask_Control, "heaterTask_Control", STACK_TEPLOMER_TASK, NULL, 8, &Heater_Control_TaskHandle);
    if(ret != pdPASS){
        ESP_LOGI(TAG, "Failed to create HeaterTask_CONTROL");
        return;
    }

    ret = xTaskCreate(MenuTask, "MenuTask", STACK_MENU_TASK, NULL, 5, &MenuTaskHandle);
    if(ret != pdPASS){
        ESP_LOGI(TAG, "Failed to create MenuTask");
        return;
    }

    ret = xTaskCreate(FilterTask, "FilterTask", STACK_MENU_TASK, NULL, 6, &TempFilterTaskHandle);
    if(ret != pdPASS){
        ESP_LOGI(TAG, "Failed to create FilterTask");
        return;
    }
}

void TimerDeclaration(){
    //vytvoreni soft. casovace
    TeplotaTimer = xTimerCreate("TeplotaTimer", pdMS_TO_TICKS(HEATER_TIMER_PERIOD), pdTRUE, NULL, HeaterTimerCallback);
    if(TeplotaTimer == NULL){
        ESP_LOGI(TAG, "Failed to create timer");
        return;
    }

    if(xTimerStart(TeplotaTimer, pdMS_TO_TICKS(100)) != pdPASS){
        ESP_LOGI(TAG, "Failed to start timer");
        return;
    }

    LCD_RefreshTimer = xTimerCreate("LCD_Timer", pdMS_TO_TICKS(LCD_TIMER_PERIOD), pdTRUE, NULL, LCD_TimerCallback);
    if(LCD_RefreshTimer == NULL){
        ESP_LOGI(TAG, "Failed to start LCD timer");
        return;
    }
    if(xTimerStart(LCD_RefreshTimer, pdMS_TO_TICKS(100)) != pdPASS){
        ESP_LOGI(TAG, "Failed to start LCD timer");
        return;
    }

    ButtonTimerUP = xTimerCreate("ButtonUP_Timer", pdMS_TO_TICKS(BUTTON_TIMER_PERIOD_1), pdTRUE, NULL, ButtonUP_TimerCallback);
    if(ButtonTimerUP == NULL){
        ESP_LOGI(TAG, "Failed to start LCD timer");
        return;
    }
    ButtonTimerDOWN = xTimerCreate("ButtonDOWN_Timer", pdMS_TO_TICKS(BUTTON_TIMER_PERIOD_1), pdTRUE, NULL, ButtonDOWN_TimerCallback);
    if(ButtonTimerDOWN == NULL){
        ESP_LOGI(TAG, "Failed to start LCD timer");
        return;
    }

    //pracovni casovace
    WorkTimer_ON = xTimerCreate("WorkON_Timer", pdMS_TO_TICKS(ON_Time), pdTRUE, NULL, WorkTimer_ON_TimerCallback);
    if(WorkTimer_ON == NULL){
        ESP_LOGI(TAG, "Failed to start ON timer");
        return;
    }

    WorkTimer_OFF = xTimerCreate("WorkOFF_Timer", pdMS_TO_TICKS(OFF_Time), pdTRUE, NULL, WorkTimer_OFF_TimerCallback);
    if(WorkTimer_OFF == NULL){
        ESP_LOGI(TAG, "Failed to start OFF timer");
        return;
    }
}

void HeaterTimerCallback(TimerHandle_t xTimer){
    if(Heater_Comm_TaskHandle != NULL){
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyGive(Heater_Comm_TaskHandle); //povoleni TeplomerTasku
        //ESP_LOGI(TAG, "EXEC 7");
    }
}

void LCD_TimerCallback(TimerHandle_t xTimer){
    LCD_Refresh = 1;
}

void ButtonUP_TimerCallback(TimerHandle_t xTimer){
    if(cursor == 1){
        if(profile_copy.steps[cursor].duration >= 1000) {profile_copy.steps[cursor].duration = 1000;}
        else{profile_copy.steps[cursor].duration += 1;}
    }
    else{
        if(profile_copy.steps[cursor].stepTemperature >= 300) {profile_copy.steps[cursor].stepTemperature = 300;}
        else{profile_copy.steps[cursor].stepTemperature += 1;}
    }
}

void ButtonDOWN_TimerCallback(TimerHandle_t xTimer){
    if(cursor == 1){
        if(profile_copy.steps[cursor].duration <= 0) {profile_copy.steps[cursor].duration = 0;}
        else{profile_copy.steps[cursor].duration -= 1;}
    }
    else{
        if(profile_copy.steps[cursor].stepTemperature <= 0) {profile_copy.steps[cursor].stepTemperature = 0;}
        else{profile_copy.steps[cursor].stepTemperature -= 1;}
    }
}

void WorkTimer_ON_TimerCallback(TimerHandle_t xTimer){
    //ESP_LOGI("CALLBACK", "ON TIMER CALLBACK");
    //vypni casovac
    if(xTimerStop(WorkTimer_ON, 2) != pdPASS) {}//{ESP_LOGI("CALLBACK", "Failed to stop ON Timer");};;
    //vypni GPIO
    gpio_set_level(GPIO_NUM_4, 0);
    //zapni OFF timer
    if(OFF_Time > 0){
        if(xTimerChangePeriod(WorkTimer_OFF, pdMS_TO_TICKS(OFF_Time), 2) != pdPASS){}// {ESP_LOGI("CALLBACK", "Failed to change period of OFF Timer");};
        if(xTimerStart(WorkTimer_OFF, 2) != pdPASS) {}//{ESP_LOGI("CALLBACK", "Failed to Start OFF Timer");};
    }
    else{
        xTaskNotifyGive(Heater_Control_TaskHandle); //
    }
}

void WorkTimer_OFF_TimerCallback(TimerHandle_t xTimer){
    //ESP_LOGI("CALLBACK", "OFF TIMER CALLBACK");
    //vypni OFF timer
    if(xTimerStop(WorkTimer_OFF, 2) != pdPASS) {}//{ESP_LOGI("CALLBACK", "Failed to stop OFF Timer");};;
    //notify Control Task
    if(Heater_Control_TaskHandle != NULL){
        xTaskNotifyGive(Heater_Control_TaskHandle); //
    }
}


void HeaterTask_Comm(void *pvParameters){

    char received_cmd[MAX_STRING_LENGTH];

    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for timer

        //uint64_t time = esp_timer_get_time();

        if(error < 0){
            Heater.MachineState = ERROR; //chyba detekovana
        }
        
        if(xQueueReceive(HeaterCMDqueue, received_cmd, pdMS_TO_TICKS(5))){
            ESP_LOGI(TAG, "%s", received_cmd);


            if(strcmp(received_cmd, "ON") == 0){
                HeaterSetON = 1;
                ms_timer = 0;
            }
            else if(strcmp(received_cmd, "OFF") == 0){
                HeaterSetOFF = 1;
            }

            if(Heater.MachineState != IDLE) continue; //pokud jsme uprostred ohrevu, tak parametry nemenime, prikazy ignorujeme
            
            char *cmd = strtok(received_cmd, ":");
            if(strcmp(cmd, "SET") == 0){

                cmd = strtok(NULL, ":"); //ziskej cislo kroku
                int stepNum = atoi(cmd);
                cmd = strtok(NULL, ":"); //parametr
                int newParam = atoi(cmd);
                updateReflowParam = 1;

                if(stepNum == 1){
                    Heater.reflow_profile.steps[stepNum].duration = newParam;
                }
                else if(stepNum == 0){
                    Heater.reflow_profile.steps[stepNum].stepTemperature = newParam;
                    Heater.reflow_profile.steps[stepNum+1].stepTemperature = newParam;
                }
                else if(stepNum == 2){
                    Heater.reflow_profile.steps[stepNum].stepTemperature = newParam;
                }

                //Heater.reflow_profile = HeaterParseCMD(cmd, &Heater.reflow_profile);

                //Kontrola nastavenych hodnot zda jsou v rozsahu
                if(cursor == 1 && Heater.reflow_profile.steps[cursor].duration <= 0){
                    Heater.reflow_profile.steps[cursor].duration = 0;
                }
                else if(cursor != 1 && Heater.reflow_profile.steps[cursor].stepTemperature <= 0){
                    Heater.reflow_profile.steps[cursor].stepTemperature = 0;
                }

                if(cursor == 1 && Heater.reflow_profile.steps[cursor].duration > 1000){
                    Heater.reflow_profile.steps[cursor].duration = 1000;
                }
                else if(cursor != 1 && Heater.reflow_profile.steps[cursor].stepTemperature > 300){
                    Heater.reflow_profile.steps[cursor].stepTemperature = 300;
                }  
            }
            //uprava regulatoru
            else if(strcmp(cmd, "CONT") == 0){
                cmd = strtok(NULL, ":"); //ziskej parametr regulatoru ke zmene
                updateControllerParam = 1;
                    if(strcmp(cmd, "P") == 0){
                        cmd = strtok(NULL, ":"); //ziskej hodnotu parametru
                        float param = atof(cmd);
                        Heater.controller.Kp = param;
                    }
                    else if(strcmp(cmd, "I") == 0){
                        cmd = strtok(NULL, ":"); //ziskej hodnotu parametru
                        float param = atof(cmd);
                        Heater.controller.Ti = param;
                    }
                    else if(strcmp(cmd, "D") == 0){
                        cmd = strtok(NULL, ":"); //ziskej hodnotu parametru
                        float param = atof(cmd);
                        Heater.controller.Td = param;
                    }
            }

        }

        //Change State of Machine
        if(HeaterSetON){Heater.MachineState = HEATING; HeaterSetON = 0; LCD_ClearMenu = 1; Heater.reflow_profile.steps[1].stepTemperature = Heater.reflow_profile.steps[0].stepTemperature; xTaskNotifyGive(Heater_Control_TaskHandle);} //LCD_ClearMenu = 1; currentMenu = &heatingMenu;
        if(HeaterSetOFF){Heater.MachineState = HEATING; Heater.profileState = COOLING; HeaterSetOFF = 0; ResetPI(&Heater.controller); currentMenu = &heatingMenu; LCD_ClearMenu = 1; }
        

        //odeslani hodnoty teploty do LCD Tasku
        //xQueueSend(TempHeaterToLCD, &Heater.controller.temperature, pdMS_TO_TICKS(5));


        //wifi debug, odesilani stavu do klienta
        char msg[30];
        snprintf(msg, 30, "HeaterState:%d", Heater.MachineState);
        sendString(msg);

        float output = ((float)Heater.controller.output/4095.0)*100.0;
        snprintf(msg, 30, "VYKON:%.2f", output);
        sendString(msg);
        snprintf(msg, 30, "SETP:%ld", Heater.controller.setpoint);
        sendString(msg);


        snprintf(msg, 30, "PREH:%.2f", Heater.reflow_profile.steps[0].stepTemperature);
        sendString(msg);
        snprintf(msg, 30, "SOAK:%d", Heater.reflow_profile.steps[1].duration);
        sendString(msg);
        snprintf(msg, 30, "REFL:%.2f", Heater.reflow_profile.steps[2].stepTemperature);
        sendString(msg);
        

        snprintf(msg, 30, "STATE:%d", Heater.profileState);
        sendString(msg);
        /*
        if(updateControllerParam || ClientConnected){
            updateControllerParam = 0;
            ClientConnected = 0;
            snprintf(msg, 30, "CONT:P:%.2f", Heater.controller.Kp);
            sendString(msg);
            snprintf(msg, 30, "CONT:Ti:%.2f", Heater.controller.Ti);
            sendString(msg);
            snprintf(msg, 30, "CONT:Td:%.2f", Heater.controller.Td);
            sendString(msg);
        }
        */

        //HeaterTaskExec = esp_timer_get_time() - time;
        //ESP_LOGI("HEATER", "Heater Execution Time: %lld", HeaterTaskExec);
        //char cBuffer[512];
        //vTaskList(cBuffer);
        //printf("%s", cBuffer);
        //eTaskGetState(HeaterTaskHandle);
    }
}

void HeaterTask_Control(void *pvParameters){

    while(1){

        //ESP_LOGI("CONTROL", "Control Task Enter");

        //cteni teploty
        xQueueReceive(TempFilterQueue, &Heater.controller.temperature, 1);

        //Stavovy automat ohrivace
        enum HeaterState cur_state = Heater.MachineState;
        Heater = Heater_StateMachine(&Heater);
        if(cur_state == HEATING && Heater.MachineState == IDLE){
            currentMenu = &exitMenu; LCD_ClearMenu = 1; //automat skoncil
        }

        if(Heater.MachineState == HEATING){

            if(Heater.profileState != COOLING){
                float percent = (float)Heater.controller.output/4095.0;
                ON_Time = (uint32_t)(percent * WORK_TIMER_CYCLE);
                OFF_Time = (uint32_t)(WORK_TIMER_CYCLE - ON_Time);
            }
            else{
                OFF_Time = WORK_TIMER_CYCLE;
                ON_Time = 0;
            }

            //ESP_LOGI("CONTROL", "On Time: %ld", ON_Time);
            //ESP_LOGI("CONTROL", "Off Time: %ld", OFF_Time);

            if(ON_Time > 0){
                //zapni GPIO
                gpio_set_level(GPIO_NUM_4, 1);
                if(xTimerChangePeriod(WorkTimer_ON, pdMS_TO_TICKS(ON_Time), 10) != pdPASS) {ESP_LOGI("CONTROL", "Failed to CHANGE PERIOD OF ON_TIMER");};
                if(xTimerStart(WorkTimer_ON, 10) != pdPASS) {ESP_LOGI("CONTROL", "Failed to Start ON_TIMER");};
            }
            else{
                gpio_set_level(GPIO_NUM_4, 0);
                if(xTimerChangePeriod(WorkTimer_OFF, pdMS_TO_TICKS(OFF_Time), 10) != pdPASS)  {ESP_LOGI("CONTROL", "Failed to CHANGE PERIOD OF OFF_TIMER");};
                if(xTimerStart(WorkTimer_OFF, 10) != pdPASS) {ESP_LOGI("CONTROL", "Failed to Start OFF_TIMER");};
            }
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for timer
    }
}

void MenuTask(void *pvParameters){

    btn1.pin = GPIO_NUM_25;
    btn1.state = 0;

    btn2.pin = GPIO_NUM_14;
    btn2.state = 0;

    btn3.pin = GPIO_NUM_27;
    btn3.state = 0;

    btn4.pin = GPIO_NUM_26;
    btn4.state = 0;

    float tempToDisplay = 0.0;

    currentMenu = &introMenu;
 
    while(1){

        //uint64_t time = esp_timer_get_time();

        //precteme stav tlacitek
        ButtonStateMachine(&btn1);
        ButtonStateMachine(&btn2);
        ButtonStateMachine(&btn3);
        ButtonStateMachine(&btn4);
        if(xQueueReceive(TempHeaterToLCD, &tempToDisplay, 1)){} //pdMS_TO_TICKS(1)

        switch(Heater.MachineState){
            case IDLE:

                //reakce na tlacitka v jednotlivych menu
                switch(currentMenu->state){
                    case BASIC: 
                        //reakce na tlacitka
                        if(btn1.state == PRESSED) {(cursor <= currentMenu->minCursor) ? currentMenu->minCursor : cursor--;} //posun dolu
                        if(btn2.state == PRESSED) {(cursor >= currentMenu->maxCursor) ? currentMenu->maxCursor : cursor++;} //posun nahoru
                        if(btn3.state == PRESSED) {if(currentMenu->childMenus[cursor] != NULL)  {currentMenu = currentMenu->childMenus[cursor]; cursor = currentMenu->minCursor; LCD_ClearMenu = 1;}} //enter LCD_ClearMenu = 1;
                        if(btn4.state == PRESSED) {MoveToParentMenu();} //cancel LCD_ClearMenu = 1;

                        (cursor <= currentMenu->minCursor) ? currentMenu->minCursor : cursor;
                        (cursor >= currentMenu->maxCursor) ? currentMenu->maxCursor : cursor;

                        profile_copy.steps[0].stepTemperature = Heater.reflow_profile.steps[0].stepTemperature;
                        profile_copy.steps[1].duration        = Heater.reflow_profile.steps[1].duration;
                        profile_copy.steps[2].stepTemperature = Heater.reflow_profile.steps[2].stepTemperature;
                        profile_copy.steps[3].stepTemperature = Heater.reflow_profile.steps[3].stepTemperature;

                        snprintf(stepPREHEAT.lines[1], 20, "%s %.2f  ", stepPreheat_2, profile_copy.steps[0].stepTemperature);
                        snprintf(stepSOAK.lines[1],    20, "%s %d  ",   stepSoak_2,    profile_copy.steps[1].duration);
                        snprintf(stepREFLOW.lines[1],  20, "%s %.2f  ", stepReflow_2,  profile_copy.steps[2].stepTemperature);
                        snprintf(stepCOOLING.lines[1], 20, "%s %.2f  ", stepCooling_2, profile_copy.steps[3].stepTemperature);
                    
                        break;

                    case CHANGE: 

                        if(btn1.state == PRESSED) {if(cursor != 1) {profile_copy.steps[cursor].stepTemperature += 1;}else{profile_copy.steps[cursor].duration += 1;}} //pridat hodnotu
                        if(btn2.state == PRESSED) {if(cursor != 1) {profile_copy.steps[cursor].stepTemperature -= 1;}else{profile_copy.steps[cursor].duration -= 1;}} //odebrat hodnotu
                        if(btn3.state == PRESSED) {Heater.reflow_profile.steps[cursor].duration = profile_copy.steps[cursor].duration; Heater.reflow_profile.steps[cursor].stepTemperature = profile_copy.steps[cursor].stepTemperature; MoveToParentMenu();} //enter
                        if(btn4.state == PRESSED) {MoveToParentMenu();} //cancel
                        //casovac UP
                        if(btn1.state == PRESSED) {xTimerStart(ButtonTimerUP, pdMS_TO_TICKS(10)); xTimerChangePeriod(ButtonTimerUP, BUTTON_TIMER_PERIOD_1/portTICK_PERIOD_MS, pdMS_TO_TICKS(10)); PresetUP = 0;}
                        if(btn1.state == HOLD){counter1++; if(counter1 > 15){counter1 = 0;PresetUP++; if(PresetUP >= 4){PresetUP = 4;} xTimerChangePeriod(ButtonTimerUP, timerPresetArray[PresetUP]/portTICK_PERIOD_MS, pdMS_TO_TICKS(10));}}
                        if(btn1.state == RELEASED){xTimerStop(ButtonTimerUP, pdMS_TO_TICKS(10));}
                        //casovac DOWN
                        if(btn2.state == PRESSED) {xTimerStart(ButtonTimerDOWN, pdMS_TO_TICKS(10)); xTimerChangePeriod(ButtonTimerDOWN, BUTTON_TIMER_PERIOD_1/portTICK_PERIOD_MS, pdMS_TO_TICKS(10)); PresetDOWN = 0;}
                        if(btn2.state == HOLD){counter2++; if(counter2 > 15){counter2 = 0; PresetDOWN++; if(PresetDOWN >= 4){PresetDOWN = 4;} xTimerChangePeriod(ButtonTimerDOWN, timerPresetArray[PresetDOWN]/portTICK_PERIOD_MS, pdMS_TO_TICKS(10));}}
                        if(btn2.state == RELEASED){xTimerStop(ButtonTimerDOWN, pdMS_TO_TICKS(10));}


                        snprintf(stepPREHEAT.lines[1], 20, "%s %.2f  ", stepPreheat_2, profile_copy.steps[0].stepTemperature);
                        snprintf(stepSOAK.lines[1],    20, "%s %d  ",   stepSoak_2,    profile_copy.steps[1].duration);
                        snprintf(stepREFLOW.lines[1],  20, "%s %.2f  ", stepReflow_2,  profile_copy.steps[2].stepTemperature);
                        snprintf(stepCOOLING.lines[1], 20, "%s %.2f  ", stepCooling_2, profile_copy.steps[3].stepTemperature);

                        //snprintf(stepPREHEAT.lines[1], 20, "%s %.2f  ", stepPreheat_2, Heater.reflow_profile.steps[0].stepTemperature);
                        //snprintf(stepSOAK.lines[1],    20, "%s %d  ",   stepSoak_2,    Heater.reflow_profile.steps[1].duration);
                        //snprintf(stepREFLOW.lines[1],  20, "%s %.2f  ", stepReflow_2,  Heater.reflow_profile.steps[2].stepTemperature);
                        //snprintf(stepCOOLING.lines[1], 20, "%s %.2f  ", stepCooling_2, Heater.reflow_profile.steps[3].stepTemperature);

                        break;

                    case START:        
                        if(btn1.state == PRESSED) {(cursor <= currentMenu->minCursor) ? currentMenu->minCursor : cursor--;} //posun dolu
                        if(btn2.state == PRESSED) {(cursor >= currentMenu->maxCursor) ? currentMenu->maxCursor : cursor++;} //posun nahoru   
                        if(btn3.state == PRESSED) {HeaterSetON = 1; ms_timer = 0;} //currentMenu = &heatingMenu; LCD_ClearMenu = 1;
                        if(btn4.state == PRESSED) {MoveToParentMenu();} //cancel LCD_ClearMenu = 1;
                        break;
                    
                    case PROCESS:
                        if(btn4.state == PRESSED) {HeaterSetOFF = 1;}
                        break;

                    case ERR:
                        break;
                    
                    case EXIT:
                        if(btn1.state == PRESSED) {MoveToParentMenu();} //posun dolu
                        if(btn2.state == PRESSED) {MoveToParentMenu();} //posun nahoru   
                        if(btn3.state == PRESSED) {MoveToParentMenu();} //currentMenu = &heatingMenu; LCD_ClearMenu = 1;
                        if(btn4.state == PRESSED) {MoveToParentMenu();} //cancel LCD_ClearMenu = 1;

                        break;
                }
                break;

            case HEATING:

                currentMenu = &heatingMenu;
                float output = ((float)Heater.controller.output/4095.0)*100.0; //vykon k zobrazeni na displeji
                //casove udaje
                uint16_t ms      = ms_timer%1000;
                uint16_t seconds = (ms_timer/1000)%60;
                uint16_t minutes = ((ms_timer/1000)/60)%60;

                char msg[15];
                char sec[3];
                char min[3];
                if(seconds < 10)  {snprintf(sec, 3, "0%d", seconds);}
                else             {snprintf(sec, 3, "%d", seconds);}
                if(minutes < 10)  {snprintf(min, 3, "0%d", minutes);}
                else             {snprintf(min, 3, "%d", minutes);}
                snprintf(msg, 15, "      %s:%s", min, sec);


                switch(Heater.profileState){
                    case PREHEAT:   snprintf(heatingMenu.lines[0], 20, "%s %s", heatingLine_1_1, msg); break;
                    case SOAK:      snprintf(heatingMenu.lines[0], 20, "%s    %s ", heatingLine_1_2, msg); break;
                    case REFLOW:    snprintf(heatingMenu.lines[0], 20, "%s  %s ", heatingLine_1_3, msg); break;
                    case COOLING:   snprintf(heatingMenu.lines[0], 20, "%s %s ", heatingLine_1_4, msg); break;
                }
                snprintf(heatingMenu.lines[1], 20, "%s %.2f %cC  ", heatingLine_2, tempToDisplay, CELSIUS);
                snprintf(heatingMenu.lines[2], 20, "%s %ld.00 %cC   ",  heatingLine_3, Heater.controller.setpoint, CELSIUS);
                snprintf(heatingMenu.lines[3], 20, "%s %.2f %%    ",  heatingLine_4, output); //Heater.controller.output

      
                if(btn4.state == PRESSED) {HeaterSetOFF = 1; cursor = 0;} //cancel pro navrat          
                break;

            case ERROR:

                currentMenu = &errorMenu;
                if(btn4.state == PRESSED) { if(maxReadTemp(&Teplomer) > 0) {MoveToParentMenu(); error = 0; Heater.MachineState = IDLE;} } //cancel
            
                snprintf(errorMenu.lines[0], 20, "%s", errorLine_1);
                snprintf(errorMenu.lines[1], 20, "%s %d", errorLine_2, error);
                if(error == -1){
                    snprintf(errorMenu.lines[2], 20, "Chyba komunikace");
                }
                else if(error == -2){
                    snprintf(errorMenu.lines[2], 20, "Term. nepripojen");
                }
                snprintf(errorMenu.lines[3], 20, "%s", errorLine_4);
                break;
        }

        LCD_Draw();

        //MenuTaskExec = esp_timer_get_time() - time;
        //ESP_LOGI("MENU", "Menu Execution Time: %lld", MenuTaskExec);
        //eTaskGetState(MenuTaskHandle);

        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_PERIOD)); //puvodne bylo 20
    }
}

void MoveToParentMenu(){

    if(currentMenu->parentMenu != NULL){
        currentMenu = currentMenu->parentMenu;
        LCD_ClearMenu = 1;
        cursor = currentMenu->minCursor;
    }

}


void LCD_Draw(){
    if(LCD_ClearMenu == 1){
        //lcd_sendCommand(0x01); //clear display
        //vTaskDelay(pdMS_TO_TICKS(10));

        lcd_setCursor(0, 0);
        for(int i = 0; i < 80; i++){
            lcd_writeChar(' ');
        }

        LCD_ClearMenu = 0;
    }

    if(LCD_Refresh == 1){
        //vykresli jednotlive linky menu
        for(uint8_t i = 0; i < 4; i++){
            lcd_setCursor(0, i);
            lcd_writeString(currentMenu->lines[i]);
        }
        LCD_Refresh = 0;
        //kurzor
        for(int i = 0; i < 4; i++){
            lcd_setCursor(19, i);
            lcd_writeChar(' ');
        }

        if(Heater.MachineState != IDLE || currentMenu->state == CHANGE || currentMenu->state == START || currentMenu->state == EXIT){
            return;
        }

        lcd_setCursor(19, cursor);
        lcd_writeChar((char)0x7F); //vykresli sipku
    }
}

void FilterTask(void *pvParameters){

    float temp_sample = 0;
    float prev_temp = 0;

    while(1){

        temp_sample = maxReadTemp(&Teplomer);

        //ESP_LOGI("FILTER", "%.2f", temp_sample);
        //detekce chyby na termoclanku
        if(temp_sample < 0){
            error = (int8_t)temp_sample;
            //ESP_LOGI("FILTER", "ERROR DETECTED");
        }


        temp_sample = Filter(temp_sample, prev_temp);
        prev_temp = temp_sample;

        xQueueOverwrite(TempFilterQueue, &temp_sample); //poslani teploty do regulatoru
        xQueueOverwrite(TempHeaterToLCD, &temp_sample); //poslani teploty do LCD displeje

        char msg[30];
        snprintf(msg, 30, "TEMP:%.2f", temp_sample);
        sendString(msg);
        if(Heater.MachineState == HEATING){
            //send data
            float seconds = (float)(ms_timer)/1000;
            float output = ((float)Heater.controller.output/4095.0)*100.0; //vykon k zobrazeni na displeji 
            //ESP_LOGI("FILTER", "TEMP: %.3f", temp_sample);
            snprintf(msg, 30, "DATA:%.3f:%.3f:%.3f", seconds, temp_sample, output);
            sendString(msg);
        }

        vTaskDelay(pdMS_TO_TICKS(FILTER_TASK_PERIOD));
    }

}

float Filter(float new_sample, float prev_sample){
    float temp = 0.0;
        if(!filter_initialized){
            temp = new_sample;
            filter_initialized = true;
            return temp;
        }
        temp = ALPHA * new_sample + (1.0 - ALPHA) * prev_sample;
        return temp;
}

//callback milisekundoveho casovace
void timer_callback(void* arg){
    ms_timer++;
}
