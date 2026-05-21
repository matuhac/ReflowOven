#pragma once
#include "driver/spi_master.h"
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

typedef struct MAX{
    //spi_device_handle_t max6675_handle;
    int MOSI_PIN;
    int MISO_PIN;
    int SCKL_PIN;
    int CS_PIN;

    float temperature;
};

void MAX_Init(struct MAX *unit, int MOSI, int MISO, int SCKL, int CS);
float maxReadTemp(struct MAX *unit);