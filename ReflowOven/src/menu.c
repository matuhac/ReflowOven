#include "menu.h"

#define PIN1 GPIO_NUM_25
#define PIN2 GPIO_NUM_14  
#define PIN3 GPIO_NUM_27
#define PIN4 GPIO_NUM_26
#define DEBOUNCE_DELAY 50000ULL //50ms

#define BTN_ACTIVE_STATE    0
#define BTN_INACTIVE_STATE  1

volatile bool zmena1 = 0;
volatile bool zmena2 = 0;

volatile uint64_t currentTime1;
volatile uint64_t currentTime2;

volatile uint8_t cursor = 0;

struct menu introMenu;
struct menu startMenu;
struct menu settingsMenu;
struct menu stepPREHEAT;
struct menu stepSOAK;
struct menu stepREFLOW;
struct menu stepCOOLING;
struct menu heatingMenu;
struct menu errorMenu;
struct menu exitMenu;


void button1_isr(void *arg){
    if(currentTime1 + DEBOUNCE_DELAY < esp_timer_get_time()){
        currentTime1 = esp_timer_get_time();
        zmena1 = 1;
    }
}

void button2_isr(void *arg){
    if(currentTime2 + DEBOUNCE_DELAY < esp_timer_get_time()){
        currentTime2 = esp_timer_get_time();
        zmena2 = 1;
    }
}

void button1_output(){
    if(zmena1 == 1){
        zmena1 = 0;
        (cursor >= 3) ?  3 : cursor++;
        ESP_LOGI("Menu", "Cursor1 %d", cursor);
    }
}

void button2_output(){
    if(zmena2 == 1){
        zmena2 = 0;
        (cursor <= 0) ?  0 : cursor--;
        ESP_LOGI("Menu", "Cursor2 %d", cursor);
    }
}

void gpio_init(){
    gpio_config_t io_conf1 = {
        .pin_bit_mask   = (1ULL << PIN1),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,
        .intr_type      = GPIO_INTR_POSEDGE,
    };
    gpio_config_t io_conf2 = {
        .pin_bit_mask   = (1ULL << PIN2),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,
        .intr_type      = GPIO_INTR_POSEDGE,
    };
    gpio_config_t io_conf3 = {
        .pin_bit_mask   = (1ULL << PIN3),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,
        .intr_type      = GPIO_INTR_POSEDGE,
    };
    gpio_config_t io_conf4 = {
        .pin_bit_mask   = (1ULL << PIN4),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,
        .intr_type      = GPIO_INTR_POSEDGE,
    };

    gpio_config(&io_conf1);
    gpio_config(&io_conf2);
    gpio_config(&io_conf3);
    gpio_config(&io_conf4);

    //gpio_install_isr_service(0);
    //gpio_isr_handler_add(PIN1, button1_isr, NULL);
    //gpio_isr_handler_add(PIN2, button2_isr, NULL);
}

//precte hodnotu na vstupu pinu
bool ReadButtonState(struct Button *btn){
    return (bool)gpio_get_level(btn->pin);
}

void ButtonStateMachine(struct Button *btn){
    bool state = ReadButtonState(btn);
    switch(btn->state){
        case RELEASED:
            btn->state = (state==BTN_ACTIVE_STATE) ? PRESSED : RELEASED;
            //ESP_LOGI("GPIO", "RELEASED STATE");
            break;
        case PRESSED:
            btn->state = (state==BTN_ACTIVE_STATE) ? HOLD : RELEASED;
            //ESP_LOGI("GPIO", "PRESSED STATE");
            break;
        case HOLD:
            btn->state = (state==BTN_ACTIVE_STATE) ? HOLD : RELEASED;
            //ESP_LOGI("GPIO", "HOLD STATE");
            break;
    }
}



void MenuInit(){
    //uvodni menu
    sprintf(introMenu.lines[0], introLine_1);
    sprintf(introMenu.lines[1], introLine_2);
    sprintf(introMenu.lines[2], noText);
    sprintf(introMenu.lines[3], noText);
    introMenu.childMenus[0] = &startMenu;
    introMenu.childMenus[1] = &settingsMenu;
    introMenu.childMenus[2] = NULL;
    introMenu.childMenus[3] = NULL;
    introMenu.parentMenu = NULL;
    introMenu.maxCursor = 1;
    introMenu.minCursor = 0;
    introMenu.state = BASIC;

    //start menu
    sprintf(startMenu.lines[0], startLine_1);
    sprintf(startMenu.lines[1], startLine_2);
    sprintf(startMenu.lines[2], startLine_3);
    sprintf(startMenu.lines[3], noText);
    startMenu.childMenus[1] = &heatingMenu;
    startMenu.childMenus[2] = &introMenu;
    startMenu.parentMenu = &introMenu;
    startMenu.maxCursor = 2;
    startMenu.minCursor = 1;
    startMenu.state = START;

    //Menu nastaveni kroku
    sprintf(settingsMenu.lines[0], settingsLine_1);
    sprintf(settingsMenu.lines[1], settingsLine_2);
    sprintf(settingsMenu.lines[2], settingsLine_3);
    sprintf(settingsMenu.lines[3], settingsLine_4);
    settingsMenu.childMenus[0] = &stepPREHEAT;
    settingsMenu.childMenus[1] = &stepSOAK;
    settingsMenu.childMenus[2] = &stepREFLOW;
    settingsMenu.childMenus[3] = &stepCOOLING;
    settingsMenu.parentMenu = &introMenu;
    settingsMenu.maxCursor = 3;
    settingsMenu.minCursor = 0;
    settingsMenu.state = BASIC;

    //nastaveni jednotlivych kroku
    sprintf(stepPREHEAT.lines[0], stepPreheat_1);
    sprintf(stepPREHEAT.lines[1], stepPreheat_2);
    sprintf(stepPREHEAT.lines[2], noText);
    sprintf(stepPREHEAT.lines[3], noText);
    stepPREHEAT.parentMenu = &settingsMenu;
    stepPREHEAT.maxCursor = 0;
    stepPREHEAT.minCursor = 0;
    stepPREHEAT.state = CHANGE;

    sprintf(stepSOAK.lines[0], stepSoak_1);
    sprintf(stepSOAK.lines[1], stepSoak_2);
    sprintf(stepSOAK.lines[2], noText);
    sprintf(stepSOAK.lines[3], noText);
    stepSOAK.parentMenu = &settingsMenu;
    stepSOAK.maxCursor = 1;
    stepSOAK.minCursor = 1;
    stepSOAK.state = CHANGE;

    sprintf(stepREFLOW.lines[0], stepReflow_1);
    sprintf(stepREFLOW.lines[1], stepReflow_2);
    sprintf(stepREFLOW.lines[2], noText);
    sprintf(stepREFLOW.lines[3], noText);
    stepREFLOW.parentMenu = &settingsMenu;
    stepREFLOW.maxCursor = 2;
    stepREFLOW.minCursor = 2;
    stepREFLOW.state = CHANGE;

    sprintf(stepCOOLING.lines[0], stepCooling_1);
    sprintf(stepCOOLING.lines[1], stepCooling_2);
    sprintf(stepCOOLING.lines[2], noText);
    sprintf(stepCOOLING.lines[3], noText);
    stepCOOLING.parentMenu = &settingsMenu;
    stepCOOLING.maxCursor = 3;
    stepCOOLING.minCursor = 3;
    stepCOOLING.state = CHANGE;

    //menu pri aktivnim ohrevu, zde se budou radky menit - budou zobrazovat udaje
    sprintf(heatingMenu.lines[0], heatingLine_1_1);
    sprintf(heatingMenu.lines[1], heatingLine_2);
    sprintf(heatingMenu.lines[2], heatingLine_3);
    sprintf(heatingMenu.lines[3], heatingLine_4);
    heatingMenu.parentMenu = &introMenu;
    heatingMenu.maxCursor = 1;
    heatingMenu.minCursor = 1;
    heatingMenu.state = PROCESS;

    sprintf(errorMenu.lines[0], errorLine_1);
    sprintf(errorMenu.lines[1], errorLine_2);
    sprintf(errorMenu.lines[2], errorLine_3);
    sprintf(errorMenu.lines[3], errorLine_4);
    errorMenu.parentMenu = &introMenu;
    errorMenu.maxCursor = 4;
    errorMenu.minCursor = 4;
    errorMenu.state = ERR;

    sprintf(exitMenu.lines[0], exitLine_1);
    sprintf(exitMenu.lines[1], exitLine_2);
    sprintf(exitMenu.lines[2], exitLine_3);
    sprintf(exitMenu.lines[3], exitLine_4);
    exitMenu.parentMenu = &introMenu;
    exitMenu.maxCursor = 0;
    exitMenu.minCursor = 0;
    exitMenu.state = EXIT;
}