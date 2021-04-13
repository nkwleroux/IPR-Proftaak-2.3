#ifndef http_request_H
#define http_request_H

typedef struct {
    double temp;
    double feels_like;
    double temp_min; //Not used
    double temp_max; //Not used
    int humidity;
    char* country_code;
    char* city;
} weatherAPI_t;

/**
 * @brief  API Task which requests data from the weather api.
 * @param  pvParameter: Parameter used for the xCreateTask method.
 */
void http_api_request_task(void * pvParameter);

/**
 * @brief  Sets the selected city.
 * @param  index: The city index in the list of cities.
 */
void http_select_city(int index);

/**
 * @brief  Getter for the parsed api data.
 * @return parsedResponse
 */
weatherAPI_t* http_get_parsed_api_response(void);

/**
 * @brief  Getter for the city selection list.
 * @return citySelection
 */
char** http_get_city_selection_list(void);

/**
 * @brief  Getter for the city selection list size.
 * @return citySelectionListSize
 */
int http_get_city_selection_list_size();

#endif // http-request