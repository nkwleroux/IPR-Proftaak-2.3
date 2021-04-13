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
 * @brief  Requests data from the weather api.
 */
void api_request_task(void * pvParameter);

weatherAPI_t* get_parsed_response(void);

void select_city(int index);

char** city_selection_list(void);

int city_selection_list_size();

#endif // http-request