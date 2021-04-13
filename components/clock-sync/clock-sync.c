#include "clock-sync.h"
#include <time.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_sntp.h"

#define CLOCKTAG "Clock"

char *timeString;
char *dateString;
clock_sync_t* parsedClock;

void parse_clock(char* dateString, char* timeString);

void clock_task(void*pvParameter){
    initialize_sntp();
    while(1)
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900)) {
            ESP_LOGI(CLOCKTAG, "Time is not set yet. Getting time from NTP.");
            obtain_time();
            // update 'now' variable with current time
            time(&now);
        }
        // update 'now' variable with current time
        time(&now);

        char strftime_buf[64];
        char strftime_buf2[64];

        // set timezone
        setenv("TZ", "CET-1", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        // convert time to string
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo); // time
        strftime(strftime_buf2, sizeof(strftime_buf), "%x", &timeinfo); // date

        timeString = strftime_buf;
        dateString = strftime_buf2;

        parse_clock(dateString,timeString);

        //menu_update_date_time(dateString,timeString);

        vTaskDelay(5000 / portTICK_PERIOD_MS); // time between updates
    }

    vTaskDelete(NULL);
}

void time_sync_notification_cb(struct timeval *tv){
    ESP_LOGI(CLOCKTAG, "Notification of a time synchronization event");
}

void obtain_time(void){
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    // loop to see if time is set
    int retry = 0;
    const int retryCount = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retryCount) {
        ESP_LOGI(CLOCKTAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    // update time
    time(&now);
    localtime_r(&now, &timeinfo);
}

void initialize_sntp(void){
    ESP_LOGI(CLOCKTAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    sntp_init();
}

void parse_clock(char* dateString, char* timeString){
    parsedClock = (clock_sync_t*) malloc(sizeof(clock_sync_t));

    parsedClock->date = dateString;
    parsedClock->time = timeString;

}

clock_sync_t* get_clock(){
    return parsedClock;
}




