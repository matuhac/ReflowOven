#include "wifi.h"
#include "index_html.h"

#define SSID "ESP_AP"
#define PASSWORD "12345678"

#define MAX_STRING_LENGTH 30

static const char *TAG = "WS_APP";
static httpd_handle_t server = NULL;

uint16_t count = 0;
bool ClientConnected = 0;

 //pole Commandu
 uint8_t cmd_number = 4; //pocet commandu
const cmd_entry CMD_TABLE[] = {
    {"HEATER"   , HandleHeater},
    {"TEMP"     , HandleTemp},
    {"TIME"     , HandleTemp},
    {"LOG"      , HandleLog},
};

//Funkce pro initializaci WiFi periferii v app_main()
void WifiInit(){
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(WIFI_EVENT, IP_EVENT_AP_STAIPASSIGNED, &ip_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid           = SSID,
            .password       = PASSWORD,
            .max_connection = MAX_CONNECTIONS,
            .authmode       = WIFI_AUTH_WPA2_PSK,
            .channel        = 1,
            .beacon_interval= 50, //pridano dodatecne
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_set_ps(WIFI_PS_NONE); //zakazat spanek

    esp_wifi_start();
}

/* Wifi handler*/
void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    //Start WIFI
    if (event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "AP started, starting HTTP server...");

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        httpd_start(&server, &config);
        //Definice WebSocket Handleru
        httpd_uri_t ws_uri = {
            .uri          = "/ws",
            .method       = HTTP_GET,
            .handler      = websocket_handler,   // definovan nize
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);

        //Tato cast obstara poslani html kodu klientovi
        httpd_uri_t root_uri ={ //mozna chyba,
            .uri     = "/",
            .method  = HTTP_GET,
            .handler = root_handler,
        };
        httpd_register_uri_handler(server, &root_uri);
    }
    //Vypnuti wifi
    else if (event_id == WIFI_EVENT_AP_STOP) {
        ESP_LOGI(TAG, "AP stopped, stopping HTTP server...");
        httpd_stop(server);
        server = NULL;
    }
    //Detekce pripojeni klienta
    else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station connected, AID=%d", event->aid);
        //ClientConnected = 1;
    }
    //Detekce odpojeni klienta
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station disconnected, AID=%d", event->aid);
    }
}

/* Handler IP eventu */
void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    // Prirazeni IP adresy klientovi
    if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "IP assigned to station: " IPSTR, IP2STR(&event->ip));
    }
}

/* WebSocket Handler - pro prijem a synchronni odesilani dat */
esp_err_t websocket_handler(httpd_req_t *req)
{
    //Potvrzeni ze websocket pripinei existuje
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket handshake completed");
        return ESP_OK;
    }

    // --PRIJEM DAT--

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    //First call: delka dat 0, pouze oznami velikost prichoziho framu
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) return ret;

    //pokud je oznamena delka 0, nic jsme neprijali
    if(ws_pkt.len == 0){
        return ESP_OK;
    }
    //Alokace bufferu pro prichozi data
    uint8_t *buf = calloc(1, ws_pkt.len + 1);
    ws_pkt.payload = buf;
    //Samotny prijem
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) { free(buf); return ret; }

    // --PARSOVANI DAT --> BUDE SE UPRAVOVAT --
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {

        char *payload_str = (char *)ws_pkt.payload;

        char* cmd = strtok(payload_str, ":"); //najdeme header commandu

        for(uint8_t i = 0; i < cmd_number; i++){ //v loopu budeme prijaty header porovnat s tabulkou headeru

            if(strcmp(CMD_TABLE[i].name, cmd) == 0) {                        
                //ESP_LOGI(TAG, "Command: %s", CMD_TABLE[i].name); //vypis jmena zavolaneho commandu
                cmd = strtok(NULL, ""); //zbytek stringu
                //ESP_LOGI(TAG, "Zbytek cmd: %s" , cmd);

                CMD_TABLE[i].handler(cmd);  //command nalezen, zavolame funkci, bude nahrazeno, pouze debug

                free(buf);
                return ESP_OK;
            }
        }

        ESP_LOGI(TAG, "Invalid command header");
    }

    free(buf); // Dealokace bufferu

    //Nize bude nadefinovano Synchronni(periodicke) odesilani data
    //Nebo budu volat ten AsyncSender
    //vyresit ACK - pomocu ws_send_int_to_client()??

    return ESP_OK;
}

/* Callback funkce pro odesilani charu */
void async_send_callback(void *arg)
{
    //async_send_arg_t *send_arg = (async_send_arg_t *)arg;
    async_send_arg_char_t *send_arg = (async_send_arg_char_t*)arg;

    // payload buffer
    char payload_buf[30];
    //int  payload_len = snprintf(payload_buf, sizeof(payload_buf), "%d", send_arg->value);
    int  payload_len = snprintf(payload_buf, sizeof(payload_buf), "%s", send_arg->message);

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)payload_buf;
    ws_pkt.len     = payload_len;
    ws_pkt.type    = HTTPD_WS_TYPE_TEXT;

    //ESP_LOGI("WIFI", "%s", ws_pkt.payload);

    // seznam vsech pripojenych klientu
    size_t  clients     = 10;          // max pocet
    int     client_fds[10];
    httpd_get_client_list(send_arg->server, &clients, client_fds);

    for (int i = 0; i < clients; i++) {
        // postupne odesilame vsem klientum v seznamu
        if (httpd_ws_get_fd_info(send_arg->server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(send_arg->server, client_fds[i], &ws_pkt);
        }
    }

    free(send_arg);
}

/* Asynchronni prenos - pushne integer cislo do vsehc pripojenych klientu, mozno volat odkudkoliv */
void ws_send_int_to_all_clients(httpd_handle_t hd, int value)
{
    if (hd == NULL) return;

    async_send_arg_t *arg = malloc(sizeof(async_send_arg_t));
    arg->server = hd;
    arg->value  = value;

    httpd_queue_work(hd, async_send_callback, arg);
}

void ws_send_char_to_all_clients(httpd_handle_t hd, char msg[30]){
    if(hd == NULL) return;

    async_send_arg_char_t *arg = malloc(sizeof(async_send_arg_char_t));
    arg->server = hd;
    strncpy(arg->message, msg, sizeof(arg->message) - 1);
    //arg->message[sizeof(arg->message)-1] = '';

    //ESP_LOGI("WIFI", "%s", arg->message);

    httpd_queue_work(hd, async_send_callback, arg);
}

esp_err_t root_handler(httpd_req_t *req)
{
    //httpd_resp_set_type(req, "text/html");
    //httpd_resp_send(req, (const char *)index_html_start, index_html_len);
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}


void HandleHeater(char* cmd){
    //ESP_LOGI(TAG, "Heater: %s", cmd);
    char queueBuf[MAX_STRING_LENGTH];
    strncpy(queueBuf, cmd, MAX_STRING_LENGTH-1); //cmd je pointer, musime obsah zkopirovat do bufferu
    xQueueSend(HeaterCMDqueue, queueBuf, pdMS_TO_TICKS(1000)); //posli cmd do heater tasku na processing
}

void HandleTemp(char* cmd){
    //ESP_LOGI(TAG, "Temp: %s", cmd);
    char queueBuf[MAX_STRING_LENGTH];
    strncpy(queueBuf, cmd, MAX_STRING_LENGTH-1); //cmd je pointer, musime obsah zkopirovat do bufferu
    xQueueSend(HeaterCMDqueue, queueBuf, pdMS_TO_TICKS(1000)); //posli cmd do heater tasku na processing
}

void HandleLog(char* cmd){
    ESP_LOGI(TAG, "Log: %s", cmd);
}


void sendNumber(){
    count++;
    ws_send_int_to_all_clients(server, count);
}

void sendString(char msg[30]){
    //ESP_LOGI("WIFI", "%s", msg);
    ws_send_char_to_all_clients(server, msg);
}