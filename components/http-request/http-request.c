#include "http-request.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "cJSON.h"

#include <math.h>
#include <string.h>

#define APITAG "API"

#define WEB_SERVER "api.openweathermap.org"
#define WEB_PORT "80"

char request[500];

int citySelectionListSize = 4;
char response[1024];
weatherAPI_t *parsedResponse;
char **citySelection = NULL;
char *selectedCity;
int _socket;

void parse_get_response(void);
void print_response(void);
double convert_kelvin_to_Celsius(double kelvin);
void set_request_string(char * city);

void init_cities(){

    citySelection = calloc(citySelectionListSize, 20);

    citySelection[0] = CONFIG_API_CITY;
    citySelection[1] = "Rotterdam";
    citySelection[2] = "London";
    citySelection[3] = "Amsterdam";

    selectedCity = citySelection[0];

    int pos = -1;
    int i;

    for (i = 0; i < citySelectionListSize; i++)
    {
        if(strcmp(citySelection[i],CONFIG_API_CITY) == 0){
            pos = i;
            break;
        }
    }

    //If config added city is the same as default cities, then it will be removed.
    if (pos != -1){

        citySelectionListSize--;

        char** tempArray = calloc(citySelectionListSize, 20);

        for (i = pos; i < citySelectionListSize; i++)
        {
            tempArray[i] = citySelection[i + 1];
        }

        free(citySelection);
        citySelection = calloc(citySelectionListSize, 20);
                        
        for (i = 0; i < citySelectionListSize; i++)
        {
            citySelection[i] = tempArray[i];
        }

        free(tempArray);
    }
}

void set_request_string(char * city){
    memset(request, 0, sizeof(request));
    strcat(request,"GET ");
    strcat(request,"http://api.openweathermap.org/data/2.5/weather?q=");
    strcat(request,city);
    strcat(request,"&appid=");
    strcat(request,CONFIG_API_KEY);
    strcat(request," HTTP/1.0\r\n");
    strcat(request,"Host: ");
    strcat(request,WEB_SERVER);
    strcat(request,":");
    strcat(request,WEB_PORT);
    strcat(request,"\r\n");
    strcat(request,"User-Agent: esp-idf/1.0 esp32\r\n");
    strcat(request,"\r\n");

    //ESP_LOGI(APITAG,"request response = %s", request);
}

void api_request(void * pvParameter){

    if(citySelection == NULL){
        init_cities();
    }

    set_request_string(selectedCity);
    
    while (1)
    {    
        memset(response, 0, sizeof(response));

        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };

        struct addrinfo *res;
        struct in_addr *addr;
        int r;
        char recv_buf[64];

        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(APITAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        /* Code to print the resolved IP. */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(APITAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        _socket = socket(res->ai_family, res->ai_socktype, 0);
        if(_socket < 0) {
            ESP_LOGE(APITAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(APITAG, "... allocated socket");

        if(connect(_socket, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(APITAG, "... socket connect failed errno=%d", errno);
            close(_socket);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(APITAG, "... connected");
        freeaddrinfo(res);

        if (write(_socket, request, strlen(request)) < 0) {
            ESP_LOGE(APITAG, "... socket send failed");
            close(_socket);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(APITAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(APITAG, "... failed to set socket receiving timeout");
            close(_socket);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(APITAG, "... set socket receiving timeout success");

        int json = 0;
        int index = 0;

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(_socket, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                if(recv_buf[i]=='{' || json){
                    json = 1;
                    response[index] = recv_buf[i];
                    index++;
                    putchar(recv_buf[i]);
                }
            }
        } while(r > 0);
 
        parse_get_response();     

        ESP_LOGI(APITAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(_socket);

        vTaskDelay(60000/portTICK_RATE_MS); // delay of 1 min

    }
}

void select_city(int index){
    selectedCity = citySelection[index];
    close(_socket);
}

char** city_selection_list(){
    return citySelection;
}

int city_selection_list_size(){
    return citySelectionListSize;
}

char* http_request_get_response(){
    return &response[0]; 
}

void parse_get_response(void){

    if(parsedResponse != NULL){
        free(parsedResponse);
    }
    
    parsedResponse = (weatherAPI_t*) malloc(sizeof(weatherAPI_t));
    
    cJSON *_root = cJSON_Parse(response);
    cJSON *_main = cJSON_GetObjectItem(_root,"main");
    double _temp = cJSON_GetObjectItem(_main,"temp")->valuedouble;
    _temp = convert_kelvin_to_Celsius(_temp);
    parsedResponse->temp = _temp;

    double _feels_like = cJSON_GetObjectItem(_main,"feels_like")->valuedouble;
    _feels_like = convert_kelvin_to_Celsius(_feels_like);
    parsedResponse->feels_like = _feels_like;
    
    double _humidity = cJSON_GetObjectItem(_main,"humidity")->valuedouble;
    parsedResponse->humidity = _humidity;

    cJSON *_sys = cJSON_GetObjectItem(_root,"sys");
    char* _country = cJSON_GetObjectItem(_sys,"country")->valuestring;
    parsedResponse->country_code = _country;

    char* _name = cJSON_GetObjectItem(_root,"name")->valuestring;
    parsedResponse->city = _name;

    //print_response();
}

double convert_kelvin_to_Celsius(double kelvin){
    return kelvin - 273.15;
}

weatherAPI_t* get_parsed_response(){
    return parsedResponse;
}

//debug
void print_response(){
    ESP_LOGI(APITAG,"temp = %.2lf C\n", parsedResponse->temp);
    ESP_LOGI(APITAG,"feels_like = %.2lf C\n", parsedResponse->feels_like);
    ESP_LOGI(APITAG,"humidity = %d%% \n", parsedResponse->humidity);
    ESP_LOGI(APITAG,"country_code = %s \n", parsedResponse->country_code);
    ESP_LOGI(APITAG,"city = %s \n", parsedResponse->city);
}



