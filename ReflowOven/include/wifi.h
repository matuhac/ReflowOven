#pragma once

#include "esp_log.h"
#include "nvs_flash.h"
//wifi include
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
//Moje include
#include "TaskSupport.h"

#define MAX_CONNECTIONS 2

extern bool ClientConnected;

typedef struct {
    httpd_handle_t server;
    int            value;
} async_send_arg_t;

typedef struct {
    httpd_handle_t server;
    char message[30];
} async_send_arg_char_t;

//struktura definuje jeden CMD
typedef struct{
    const char *name; //jmeno CMD
    void (*handler)(char*); //pointer na funkci
}cmd_entry;

//Handleru commandu
void HandleHeater(char* cmd);
void HandleTemp(char* cmd);
void HandleLog(char* cmd);


void WifiInit();
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);     
esp_err_t websocket_handler(httpd_req_t *req);   
void async_send_callback(void *arg);    
void ws_send_int_to_all_clients(httpd_handle_t hd, int value);  
void ws_send_char_to_all_clients(httpd_handle_t hd, char msg[30]);
esp_err_t root_handler(httpd_req_t *req);   

void sendNumber();
void sendString(char msg[30]);
