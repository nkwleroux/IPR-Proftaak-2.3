#ifndef clock_sync_H
#define clock_sync_H

//Init for the sntp
void initialize_sntp(void);

//Sets the time to the current time
void obtain_time(void);

//Clock task
void clock_task(void*pvParameter);

//Get time in string
char *clock_get_time();

//Get date in string
char* clock_get_date();

#endif
