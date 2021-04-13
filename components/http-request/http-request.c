#include "http-request.h"
#include <math.h>
#include <string.h>
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

//Wifi
#include "wifi-connect.h"

#define APITAG "API"

#define WEB_SERVER "api.openweathermap.org"
#define WEB_PORT "80"

weatherAPI_t *parsedResponse;
char **citySelection = NULL;
char *selectedCity;
char request[500];
char response[1024];
int citySelectionListSize = 4;
int httpSocket; 

void init_cities(void);
void set_request_string(char * city);
void parse_get_response(void);
double convert_kelvin_to_Celsius(double kelvin);
void print_response(void);

void http_api_request_task(void * pvParameter)
{
    // Checks if citySelection is NULL. If NULL then initializes the list.
    if(citySelection == NULL){
        init_cities();
    }

    // Sets the request string.
    set_request_string(selectedCity);
    
    while (1)
    {    
        // Clears the response array.
        memset(response, 0, sizeof(response));

        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };

        struct addrinfo *res;
        struct in_addr *addr;
        int r;
        char recv_buf[64];

        // Check if wifi is connected. 
        if(wifi_is_connected() != -1){
            int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

            if(err != 0 || res == NULL) {
                ESP_LOGE(APITAG, "DNS lookup failed err=%d res=%p", err, res);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            /* Code to print the resolved IP. */
            addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
            ESP_LOGI(APITAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

            httpSocket = socket(res->ai_family, res->ai_socktype, 0);
            if(httpSocket < 0) {
                ESP_LOGE(APITAG, "... Failed to allocate socket.");
                freeaddrinfo(res);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            ESP_LOGI(APITAG, "... allocated socket");

            if(connect(httpSocket, res->ai_addr, res->ai_addrlen) != 0) {
                ESP_LOGE(APITAG, "... socket connect failed errno=%d", errno);
                close(httpSocket);
                freeaddrinfo(res);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }

            ESP_LOGI(APITAG, "... connected");
            freeaddrinfo(res);

            if (write(httpSocket, request, strlen(request)) < 0) {
                ESP_LOGE(APITAG, "... socket send failed");
                close(httpSocket);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
            ESP_LOGI(APITAG, "... socket send success");

            struct timeval receiving_timeout;
            receiving_timeout.tv_sec = 5;
            receiving_timeout.tv_usec = 0;
            if (setsockopt(httpSocket, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                    sizeof(receiving_timeout)) < 0) {
                ESP_LOGE(APITAG, "... failed to set socket receiving timeout");
                close(httpSocket);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
            ESP_LOGI(APITAG, "... set socket receiving timeout success");

            int json = 0;
            int index = 0;

            /* Read HTTP response */
            do {
                bzero(recv_buf, sizeof(recv_buf));
                r = read(httpSocket, recv_buf, sizeof(recv_buf)-1);
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
            close(httpSocket);
            
            vTaskDelay(60000/portTICK_RATE_MS); // delay of 1 min
        }else{
            vTaskDelay(5000/portTICK_RATE_MS);
        }

    }

    vTaskDelete(NULL);
}

/**
 * @brief  Initializes the citySelectionList.
 */
void init_cities()
{
    // Allocates memory to citySelection.
    citySelection = calloc(citySelectionListSize, 20);

    // Sets the list values.
    citySelection[0] = CONFIG_API_CITY;
    citySelection[1] = "Rotterdam";
    citySelection[2] = "London";
    citySelection[3] = "Amsterdam";

    // Sets the default selectedCity.
    selectedCity = citySelection[0];

    int pos = -1;
    int i;

    // Checks if the config default city is the in the list of default cities. 
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

/**
 * @brief  Sets the string for the get request.
 * @param  city: The city which you want the API data from. 
 */
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
}

void http_select_city(int index)
{
    // Sets the selected city and closes the current socket.
    selectedCity = citySelection[index];
    close(httpSocket);
}

/**
 * @brief  Parses the get response into the weatherAPI_t variable.
 */
void parse_get_response(void)
{
    // If parsedResponse is not NULL, free the parsedResponse
    if(parsedResponse != NULL){
        free(parsedResponse);
    }
    
    // Allocates memory to parsedResponse.
    parsedResponse = (weatherAPI_t*) malloc(sizeof(weatherAPI_t));
    
    // Parses the data and converts the data from Kelvin to Celsius.
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

}

/**
 * @brief  Helper method to convert Kelvin to Celsius.
 * @param  kelvin: Input
 * @return Converted value
 */
double convert_kelvin_to_Celsius(double kelvin)
{
    return kelvin - 273.15;
}

weatherAPI_t* http_get_parsed_api_response(void)
{
    return parsedResponse;
}

char** http_get_city_selection_list()
{
    return citySelection;
}

int http_get_city_selection_list_size()
{
    return citySelectionListSize;
}

// Method used only for debugging api responses.
void print_response(void)
{
    ESP_LOGI(APITAG,"temp = %.2lf C\n", parsedResponse->temp);
    ESP_LOGI(APITAG,"feels_like = %.2lf C\n", parsedResponse->feels_like);
    ESP_LOGI(APITAG,"humidity = %d%% \n", parsedResponse->humidity);
    ESP_LOGI(APITAG,"country_code = %s \n", parsedResponse->country_code);
    ESP_LOGI(APITAG,"city = %s \n", parsedResponse->city);
}



