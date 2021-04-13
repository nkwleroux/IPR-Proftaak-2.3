#include "clock-sync.h"
#include <time.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_sntp.h"

#define CLOCKTAG "Clock"

clock_sync_t* parsedClock;
char *timeString;
char *dateString;

void obtain_time(void);
void initialize_sntp(void);
void parse_clock(char* dateString, char* timeString);

void clock_task(void*pvParameter)
{
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
            // Update 'now' variable with current time
            time(&now);
        }
        // Update 'now' variable with current time
        time(&now);

        char strftime_buf[64];
        char strftime_buf2[64];

        // Set timezone
        setenv("TZ", "CET-1", 1);
        tzset();
        localtime_r(&now, &timeinfo);

        // Convert time to string
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo); // time
        strftime(strftime_buf2, sizeof(strftime_buf), "%x", &timeinfo); // date

        timeString = strftime_buf;
        dateString = strftime_buf2;

        parse_clock(dateString,timeString);

        vTaskDelay(5000 / portTICK_PERIOD_MS); // Time between updates
    }

    vTaskDelete(NULL);
}

/**
 * @brief  Sets the time to the current time.
 */
void obtain_time(void)
{
    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    // Loop to see if time is set
    int retry = 0;
    const int retryCount = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retryCount) {
        ESP_LOGI(CLOCKTAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    // Update time
    time(&now);
    localtime_r(&now, &timeinfo);
}

/**
 * @brief  Callback for the notifications.
 */
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(CLOCKTAG, "Notification of a time synchronization event");
}

/**
 * @brief  Initializes sntp to get the time and date.
 */
void initialize_sntp(void)
{
    ESP_LOGI(CLOCKTAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    sntp_init();
}

/**
 * @brief  Parses date and time into clock_sync_t
 * @param  dateString: The date in string format.
 * @param  timeString: The time in string format.
 */
void parse_clock(char* dateString, char* timeString)
{
    // Allocate memory for parsedClock.
    parsedClock = (clock_sync_t*) malloc(sizeof(clock_sync_t));

    // Sets variables date and time
    parsedClock->date = dateString;
    parsedClock->time = timeString;
}

clock_sync_t* get_clock()
{
    return parsedClock;
}




