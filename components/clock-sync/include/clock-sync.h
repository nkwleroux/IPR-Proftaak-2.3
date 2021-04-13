#ifndef clock_sync_H
#define clock_sync_H

typedef struct {
    char* date;
    char* time;
} clock_sync_t;

/**
 * @brief  Clock Task which gets the time and date.
 * @param  pvParameter: Parameter used for the xCreateTask method.
 */
void clock_task(void*pvParameter);

/**
 * @brief  Getter for the clock variable.
 * @retval parsedClock
 */
clock_sync_t* get_clock();

#endif // clock-sync
