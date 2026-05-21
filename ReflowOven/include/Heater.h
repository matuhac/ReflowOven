//Knihovna bude obsahovat funkce pro realizaci ohrevu, PI regulator atd.
#pragma once

#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include <math.h>

#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"

//include DAC
#include "driver/dac_oneshot.h"

#include "driver/ledc.h"
#include "driver/gpio.h"

//PREHEAT, SOAK, REFLOW, COOLDOWN
#define MAX_PROFILE_STEPS 4

struct PI_controller{
    float temperature; // aktualni hodnota teploty
    //konstanty regulace
    float Kp;
    float Ti;
    float Td;
    float N;
    float Ts;
    float Tt;
    float dyn_omez;

    float prev_I; //sumator
    float prev_D;
    float prev_err;
    float prev_prev_err; //pouze pro D slozku
    uint32_t setpoint; //zadana hodnota
    uint32_t output; //0-255 pro DAC, 0-4095 pro PWM
    //saturace vystupu
    float out_max; 
    float out_min;
};

//struktura definuje jeden krok profilu
typedef struct reflow_step_t
{
    float stepTemperature;
    uint16_t duration;
};

//struktura bude obsahovat vsechny potrebne kroky teplotniho profilu
typedef struct reflow_profile_t
{
    char name[32];
    uint8_t numberOfSteps;
    struct reflow_step_t steps[MAX_PROFILE_STEPS];

};

//enum pro rizeni stavu trouby
enum HeaterState {
    IDLE,
    HEATING,
    ERROR,
};

//enum pro rizeni jednotlivyck kroku ohrevu, mozna bude nahrazeno strukturou reflow_profile_t
enum ProfileState {
    PREHEAT,
    SOAK,
    REFLOW,
    COOLING
};

//Struktura sdruzuje veskere prvky ohrivace, profil, PI regulator, stavove automaty
typedef struct HeaterMachine
{
    struct PI_controller controller;
    struct reflow_profile_t reflow_profile;
    enum ProfileState profileState;
    enum HeaterState MachineState;
};

bool timer_oneSec_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
void HeaterInit();

void ResetPI(struct PI_controller *controller);
struct PI_controller PI_calculation(struct PI_controller *controller);

struct HeaterMachine Heater_StateMachine(struct HeaterMachine *Machine);
struct HeaterMachine Heater_ProfileStateMachine(struct HeaterMachine *Machine);
struct HeaterMachine ResetHeater(struct HeaterMachine *Machine);

struct HeaterMachine GainSchedulling(struct HeaterMachine *Machine); //funkce pro budouci implementaci Gain schedulingu??

struct reflow_profile_t HeaterParseCMD(char *cmd, struct reflow_profile_t *profile);