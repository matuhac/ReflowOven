#include "lcd.h"

//#define LCD_ADDR 		(0x27 << 1)
#define LCD_ADDR 		0x27
#define LCD_ENABLE 		0x04
#define LCD_BACKLIGHT 	0x08
#define LCR_RW 			0x00
#define LCD_RS 			0x01
#define LCD_Delay		200 //200 ms
#define LCD_TIMEOUT     25 // ms

#define I2C_Delay 1000

//parametry komunikace
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000 
#define I2C_PORT I2C_NUM_0

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t lcd_handle;

uint8_t row_offset[4] = {0x00, 0x40, 0x14, 0x54};


void lcd_sendPulse(uint8_t data){
    uint8_t buffer1[1] = {data};
    uint8_t buffer2[1] = {data | LCD_ENABLE};
    //uint8_t buffer[3] = {data, data | LCD_ENABLE, data};

    //i2c_master_transmit(lcd_handle, buffer, 3, LCD_TIMEOUT);

    i2c_master_transmit(lcd_handle, buffer1, 1, LCD_TIMEOUT);
    i2c_master_transmit(lcd_handle, buffer2, 1, LCD_TIMEOUT);
    i2c_master_transmit(lcd_handle, buffer1, 1, LCD_TIMEOUT);
}

void lcd_sendNibble(uint8_t nibble, uint8_t control){
    uint8_t data = nibble | LCD_BACKLIGHT | control;
    lcd_sendPulse(data);
}

void lcd_sendByte(uint8_t byte, uint8_t mode){
    uint8_t high = byte & 0xF0;
    uint8_t low = (byte << 4) & 0xF0;

    lcd_sendNibble(high, mode);
    lcd_sendNibble(low, mode);
}

void lcd_init(){
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
        //.trans_queue_depth = 1000
    };

    i2c_new_master_bus(&bus_config, &bus_handle);

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .flags.disable_ack_check = 0
    };

    i2c_master_bus_add_device(bus_handle, &dev_config, &lcd_handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_sendNibble(0x03 << 4, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_sendNibble(0x03 << 4, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_sendNibble(0x03 << 4, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_sendNibble(0x02 << 4, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_sendCommand(0x28); //function set, 4 bit, 5x8
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_sendCommand(0x08); //display off
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_sendCommand(0x01); //clear display
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_sendCommand(0x06); //entry mode set
    //lcd_sendCommand(0x0F); //display on, blikani kurzoru ON
    lcd_sendCommand(0x0C); //display on, blikani kurzoru OFF
}

void lcd_sendCommand( uint8_t cmd){
    lcd_sendByte(cmd, 0);
}

void lcd_setCursor(uint8_t col, uint8_t row){
    if(row > 3) {row = 3;}
    lcd_sendCommand(0x80 | (row_offset[row] + col));
}

void lcd_writeData(uint8_t data){
    lcd_sendByte(data, LCD_RS);
}

void lcd_writeChar(char c){
    lcd_writeData((uint8_t) c);
}

void lcd_writeString(const char *str){
    while(*str){
        lcd_writeData((uint8_t)(*str));
        str++;
    }
}


void init(){
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    i2c_new_master_bus(&bus_config, &bus_handle);

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_bus_add_device(bus_handle, &dev_config, &lcd_handle);
}