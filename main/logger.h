#ifndef LOGGER_H
#define LOGGER_H

#include "esp_log.h"
#include "led_vars.h"

const char* tag = "LOGGER";

typedef enum {
    NOT_SET,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
} weekday_enum_t;

typedef struct {
    weekday_enum_t dayOfWeek;
    bool todayIsActive;
    bool scheduleIsActive;
} schedule_status_t;

schedule_status_t scheduleStatus;

void logger_init_schedule_status() {
    scheduleStatus.dayOfWeek = NOT_SET;
    scheduleStatus.scheduleIsActive = false;
    scheduleStatus.todayIsActive = false;
}

void logger_set_day_of_week(const char* const dayOfWeek) {
    if (strcmp(dayOfWeek, "Monday") == 0) {
        scheduleStatus.dayOfWeek = MONDAY;
    } else if (strcmp(dayOfWeek, "Tuesday") == 0) {
        scheduleStatus.dayOfWeek = TUESDAY;
    } else if (strcmp(dayOfWeek, "Wednesday") == 0) {
        scheduleStatus.dayOfWeek = WEDNESDAY;
    } else if (strcmp(dayOfWeek, "Thursday") == 0) {
        scheduleStatus.dayOfWeek = THURSDAY;
    } else if (strcmp(dayOfWeek, "Friday") == 0) {
        scheduleStatus.dayOfWeek = FRIDAY;
    } else if (strcmp(dayOfWeek, "Saturday") == 0) {
        scheduleStatus.dayOfWeek = SATURDAY;
    } else if (strcmp(dayOfWeek, "Sunday") == 0) {
        scheduleStatus.dayOfWeek = SUNDAY;
    } else {
        scheduleStatus.dayOfWeek = NOT_SET;
    }
}

void logger_print_schedule_status(const led_data_t * const ledData) {
    const char *dayOfWeekStr;
    switch (scheduleStatus.dayOfWeek) {
        case MONDAY: dayOfWeekStr = "Monday"; break;
        case TUESDAY: dayOfWeekStr = "Tuesday"; break;
        case WEDNESDAY: dayOfWeekStr = "Wednesday"; break;
        case THURSDAY: dayOfWeekStr = "Thursday"; break;
        case FRIDAY: dayOfWeekStr = "Friday"; break;
        case SATURDAY: dayOfWeekStr = "Saturday"; break;
        case SUNDAY: dayOfWeekStr = "Sunday"; break;
        default: dayOfWeekStr = "Not Set"; break;
    }
    
    if (scheduleStatus.dayOfWeek != NOT_SET) {
        ESP_LOGI(tag, "The schedule is set for the following days: %s %s %s %s %s %s %s", 
            ledData->monday ? "Monday" : "",
            ledData->tuesday ? "Tuesday" : "",
            ledData->wednesday ? "Wednesday" : "",
            ledData->thursday ? "Thursday" : "",
            ledData->friday ? "Friday" : "",
            ledData->saturday ? "Saturday" : "",
            ledData->sunday ? "Sunday" : "");        
    }
    ESP_LOGI(tag, "Today is %s, there is %s schedule active", dayOfWeekStr, scheduleStatus.todayIsActive ? "a" : "no");
    ESP_LOGI(tag, "The current time is %s, the time zone is %d, the timestamp is %lu", ledData->timeNowString, ledData->timezone, ledData->timeNow);
    ESP_LOGI(tag, "The schedule is set between %s and %s, it is currently %s", ledData->lightStart, ledData->lightEnd, scheduleStatus.scheduleIsActive ? "active" : "inactive");
    ESP_LOGI(tag, "The target light intensity is %d, the current light intensity is %d \n", ledData->lightIntensity, ledData->currentLightIntensity);
}

#endif // LOGGER_H
