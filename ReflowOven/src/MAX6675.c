#include "MAX6675.h"
//#include "driver/spi_master.h"

#define SPI_HOST_MINE SPI2_HOST

spi_device_handle_t max6675_handle;

BaseType_t ret;


void MAX_Init(struct MAX *unit, int MOSI, int MISO, int SCKL, int CS){
    //Nastveni Pinu
    unit->MOSI_PIN = MOSI;
    unit->MISO_PIN = MISO;
    unit->SCKL_PIN = SCKL;
    unit->CS_PIN = CS;
    //unit->ret = 0;
    unit->temperature = 0;

    //Inicializace SPI
    spi_bus_config_t buscfg = {
        .miso_io_num        = unit->MISO_PIN,
        .mosi_io_num        = unit->MOSI_PIN,
        .sclk_io_num        = unit->SCKL_PIN,
        .quadhd_io_num      = -1,
        .quadwp_io_num      = -1,
        .max_transfer_sz    = 2    
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1*1000*1000,
        .mode           = 0,
        .spics_io_num   = unit->CS_PIN,
        .queue_size     = 7, 
    //  .flags          = SPI_DEVICE_HALFDUPLEX, //chyba jen pro vysilani, ale my chceme jen prijimat
    };

    spi_bus_initialize(SPI_HOST_MINE, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_bus_add_device(SPI_HOST_MINE, &devcfg, &max6675_handle);
    ESP_ERROR_CHECK(ret);
}

float maxReadTemp(struct MAX *unit){
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t rx_data[2] = {0};

    t.length = 16;
    t.rx_buffer = rx_data;

    ret = spi_device_transmit(max6675_handle, &t);

    if(ret != ESP_OK){
        return -1.0; //spi komunikace selhala
    }
    uint16_t raw = (rx_data[0] << 8) | rx_data[1];

    //nic neprijato - chyba
    if(raw == 0){
        return -1.0;
    }

    //indikace, ze termoclanek je nepripojeny
    if(raw & 0x4){
        return -2.0;
    }

    raw >>= 3;
    float temperature = raw * 0.25;
    unit->temperature = temperature;
    return temperature;
}
