#ifndef clock_sync_H
#define clock_sync_H

typedef struct {
    char* date;
    char* time;
} clock_sync_t;

//Init for the sntp
void initialize_sntp(void);

//Sets the time to the current time
void obtain_time(void);

//Clock task
void clock_task(void*pvParameter);

clock_sync_t* get_clock();

#endif
