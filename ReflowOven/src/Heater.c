#include "Heater.h"

//#define DAC
//#define PWM
#define STANDARD_PI

#define SPEED_MODE LEDC_LOW_SPEED_MODE
#define LED_CHANNEL LEDC_CHANNEL_0

#define ventilatorPIN 15
#define NORMAL 32
#define PRACOVNI 33

static const char *TAG_HEATER = "HEATER";

dac_oneshot_handle_t dac_handle1;
dac_oneshot_handle_t dac_handle2;

gptimer_config_t timer_oneSec;
gptimer_handle_t timer_handle;

gpio_config_t normal_led = {
        .pin_bit_mask   = (1ULL << NORMAL),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
};
gpio_config_t prac_led = {
        .pin_bit_mask   = (1ULL << PRACOVNI),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
};

gpio_config_t vent_config = {
        .pin_bit_mask   = (1ULL << ventilatorPIN),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
};
bool errorLevel = 0;

volatile uint32_t timer_seconds = 0;

bool timer_oneSec_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    timer_seconds++;

    return false;
}

//Inicializace DAC
void HeaterInit(){

    //inicializace DAC
    #if defined(DAC)
    dac_oneshot_config_t dac_config1;
    dac_oneshot_config_t dac_config2;

    dac_config1.chan_id = DAC_CHAN_0;
    dac_config2.chan_id = DAC_CHAN_1;

    esp_err_t ret;
    ret = dac_oneshot_new_channel(&dac_config1, &dac_handle1);
    if(ret != ESP_OK){
        ESP_LOGI(TAG_HEATER, "Dac1 Fail Initialization");
    }

    ret = dac_oneshot_new_channel(&dac_config2, &dac_handle2);
    if(ret != ESP_OK){
        ESP_LOGI(TAG_HEATER, "Dac2 Fail Initialization");
    }
    #elif defined(PWM)
    //inicializace PWM
    ledc_timer_config_t ledc_timer1 = {
        .speed_mode         = SPEED_MODE,
        .timer_num          = LEDC_TIMER_0,
        .duty_resolution    = LEDC_TIMER_12_BIT,
        .freq_hz            = 1,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer1));

    ledc_channel_config_t ledc_channel1 = {
        .gpio_num       = 4,
        .speed_mode     = SPEED_MODE,
        .channel        = LED_CHANNEL,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel1));
    ESP_LOGI(TAG_HEATER, "LEDC Init Done");

    #else

    gpio_config_t io_conf1 = {
        .pin_bit_mask   = (1ULL << GPIO_NUM_4),
        .mode           = GPIO_MODE_OUTPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf1);

    #endif

    //Inicializace 1s casovace
    timer_oneSec.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_oneSec.direction      = GPTIMER_COUNT_UP;
    timer_oneSec.resolution_hz  = 1000000; //1 MHz, 1 tick => 1 us 

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_oneSec, &timer_handle));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_oneSec_callback,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_handle, &cbs, NULL)); //userData = NULL mozna vymenit za pocet sekund??
    ESP_ERROR_CHECK(gptimer_enable(timer_handle)); //povoleni casovace

    gptimer_alarm_config_t alarm_config = {
        .reload_count               = 0,
        .alarm_count                = 1000000,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(timer_handle)); //zapnuti casovace

    //konfigurace pinu na ventilator
    gpio_config(&vent_config);

    //konfigurace signalnich LED
    gpio_config(&prac_led);
    gpio_config(&normal_led);

}

void ResetPI(struct PI_controller *controller){
    controller->output = 0; //nastavime vystup na nulu
    controller->prev_I = 0; //vyresetujeme integracni slozku
    controller->prev_D = 0; //vyresetujeme derivacni slozku
}

//funkce pro vypocet hodnoty PI regulatoru
struct PI_controller PI_calculation(struct PI_controller *controller){

    float out = 0.0;
    float not_satur = 0.0;
    float error = controller->setpoint - controller->temperature;
    #ifndef STANDARD_PI

    //inkrementalni forma
    //float error = controller->setpoint - controller->temperature;
    float P = controller->Kp * (error - controller->prev_err);
    float I = ((controller->Kp * controller->Ts)/controller->Ti)*error;
    float D = ((controller->Kp*controller->Td)/controller->Ts)*(error - 2*controller->prev_err + controller->prev_prev_err);
    controller->prev_prev_err = controller->prev_err;
    controller->prev_err = error;

    out = controller->output + P + I + D;

    #else
    //Standardni forma 
    controller->prev_I += error; //integrace
    float P = controller->Kp * error;
    float I = controller->Kp * (controller->dyn_omez + controller->Ts * controller->prev_I/controller->Ti);
    float sub_D = (error * controller->Kp * controller->N + exp((-controller->Ts/controller->Td) * controller->N) * controller->prev_D);
    float D = sub_D - controller->prev_D; 
    controller->prev_D = sub_D;

    not_satur = P + I + D;
    //out = controller->Kp * (error + controller->prev_I/controller->Ti);

    //Saturace integracni slozky
    if(controller->prev_I > controller->out_max*2.0){
        controller->prev_I = controller->out_max*2.0;
    }
    else if (controller->prev_I < controller->out_min){
        controller->prev_I = controller->out_min;
    }

    #endif

    //Saturace vystupu
    if(not_satur > controller->out_max){
        out = controller->out_max;
    }
    else if (not_satur < controller->out_min){
        out = controller->out_min;
    }
    else{
        out = not_satur;
    }
    //dynamicke omezeni integracni slozky
    //controller->dyn_omez = (out - not_satur)/controller->Tt;

    //zapsani hodnoty na vystup
    controller->output = out;


    //ESP_LOGI("HEATER", "SetPoint: %ld Temprature: : %f", controller->setpoint, controller->temperature);
    //ESP_LOGI("HEATER", "ERROR: %f P: %f I: %f, prev_err: %f", error, P, I, controller->prev_err);
    //ESP_LOGI("HEATER", "OUTPUT: %ld", controller->output);
    #if defined(PWM)
    ledc_set_duty(SPEED_MODE, LED_CHANNEL, controller->output);
    ledc_update_duty(SPEED_MODE, LED_CHANNEL);
    #endif

    return *controller;
}

struct HeaterMachine Heater_StateMachine(struct HeaterMachine *Machine){
    switch(Machine->MachineState)
    {
    case IDLE:
        //nastaveni pocatecniho kroku profilu na PREHEAT
        Machine->profileState = PREHEAT;
        //V IDLE rezimu je vystup vypnuty
        ResetPI(&Machine->controller);
        #if defined (PWM)
        ledc_set_duty(SPEED_MODE, LED_CHANNEL, 0);
        ledc_update_duty(SPEED_MODE, LED_CHANNEL);
        #endif
        gpio_set_level(NORMAL, 1);
        gpio_set_level(PRACOVNI, 0);
        gpio_set_level(ventilatorPIN, 0);
        break;
    case HEATING:
        *Machine = GainSchedulling(Machine);
        Heater_ProfileStateMachine(Machine); //stavovy automat ohrevu

        /*
        //pouze pro mereni prechod char. jinak smazat
        if(Machine->controller.temperature >275 || Machine->profileState == COOLING){
            Machine->controller.output = 0; // 50 % vykonu
        }
        else{
            Machine->controller.output = 4095;
        }
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, Machine->controller.output);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        */

        gpio_set_level(NORMAL, 0);
        gpio_set_level(PRACOVNI, 1);
        gpio_set_level(ventilatorPIN, 1);
        break;
    case ERROR:
        //v error rezimu vypneme vystupy
        ResetPI(&Machine->controller);
        #if defined(PWM)
        ledc_set_duty(SPEED_MODE, LED_CHANNEL, 0);
        ledc_update_duty(SPEED_MODE, LED_CHANNEL);
        #endif

        if(errorLevel){
            gpio_set_level(PRACOVNI, 1);
            gpio_set_level(NORMAL, 0);
            errorLevel = 0;
        }
        else{
            gpio_set_level(PRACOVNI, 0);
            gpio_set_level(NORMAL, 1); 
            errorLevel = 1;
        }
        gpio_set_level(ventilatorPIN, 0);
        break;
    }


    return *Machine;
}

struct HeaterMachine Heater_ProfileStateMachine(struct HeaterMachine *Machine){

    switch(Machine->profileState){
        case PREHEAT:
        Machine->controller.setpoint =  Machine->reflow_profile.steps[0].stepTemperature;
        if(Machine->controller.temperature >= Machine->controller.setpoint){
            Machine->profileState = SOAK;
            timer_seconds = 0;
        }
        Machine->controller = PI_calculation(&Machine->controller);   //vypocet PI regulatoru
        break;

        case SOAK:
        //zde musime udrzet teplotu nejakou dobu
        Machine->controller.setpoint = Machine->reflow_profile.steps[1].stepTemperature;
        if(timer_seconds >= Machine->reflow_profile.steps[1].duration){
            Machine->profileState = REFLOW;
            Machine->controller.setpoint = Machine->reflow_profile.steps[2].stepTemperature;
        }
        Machine->controller = PI_calculation(&Machine->controller);   //vypocet PI regulatoru
        break;

        case REFLOW:
        Machine->controller.setpoint = Machine->reflow_profile.steps[2].stepTemperature;
        Machine->controller = PI_calculation(&Machine->controller);    //vypocet PI regulatoru
        if(Machine->controller.temperature >= Machine->controller.setpoint){
            Machine->profileState = COOLING;
            Machine->controller.setpoint = Machine->reflow_profile.steps[3].stepTemperature;
            Machine->controller.output = 0;
        }
        break;

        case COOLING:
        Machine->controller.setpoint = Machine->reflow_profile.steps[3].stepTemperature;

        Machine->controller.output = 0;
        #if defined(PWM)
        ledc_set_duty(SPEED_MODE, LED_CHANNEL, 0);
        ledc_update_duty(SPEED_MODE, LED_CHANNEL);
        #endif
        if(Machine->controller.temperature <= Machine->controller.setpoint){
            ResetHeater(Machine);
        }
        break;
    }

    return *Machine;
}

struct HeaterMachine ResetHeater(struct HeaterMachine *Machine){
    Machine->MachineState = IDLE;
    Machine->profileState = PREHEAT;
    ResetPI(&Machine->controller);
    #if defined(PWM)
    ledc_set_duty(SPEED_MODE, LED_CHANNEL, 0);
    ledc_update_duty(SPEED_MODE, LED_CHANNEL);
    #endif

    return *Machine;
}

struct reflow_profile_t HeaterParseCMD(char *cmd, struct reflow_profile_t *profile){
    cmd = strtok(NULL, ":"); //ziskej cislo kroku
    int stepNum = atoi(cmd);
    cmd = strtok(NULL, ":"); //ziskej teplotu
    int newTemp = atoi(cmd);
    cmd = strtok(NULL, ""); //ziskej cas
    int newTime = atoi(cmd);
    //ESP_LOGI(TAG, "SET TEMPERATURE: StepNum: %d, NewTemp: %d, NewTime: %d", stepNum, newTemp, newTime);

    profile->steps[stepNum].duration = (uint16_t)newTime; //ulozime proijate hodnoty do profilu
    profile->steps[stepNum].stepTemperature = (float)newTemp;

    return *profile;
}

struct HeaterMachine GainSchedulling(struct HeaterMachine *Machine){
    //v REFLOW stavu, posilime I slozku a upravime jeji saturaci
    if(Machine->profileState == REFLOW){
        Machine->controller.Ti = 400.0;
        //Machine->controller.out_max = 4641.54; //zvysime limit saturace, aby I slozka mohla dosahnout vyssi hodnoty
        Machine->controller.out_max = 4095.0; //standardni limit
        //vypoctena hodnota z (Kp*Ts)/Ti * out_max = 4095, vycisleno out_max -> 4641.54
    }
    else{
        Machine->controller.Ti = 490.0;
        Machine->controller.out_max = 4095.0; //standardni limit
    }
    return *Machine;
}