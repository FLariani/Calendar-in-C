#pragma once
#ifndef CALENDAR_H
#define CALENDAR_H

#include <stdio.h>

#define DESC_LEN 256

#ifdef __cplusplus
extern "C" {
#endif

    struct years;
    struct months;
    struct days;
    struct tasks;

    struct years {
        int year_number;
        struct months* months;
        struct years* next;
    };

    struct months {
        int month_number;
        const char* month_name;
        struct days* days;
        int num_days;
    };

    struct days {
        int day_number;
        const char* day_name;
        struct tasks* tasks_head;
    };

    struct tasks {
        int task_id;
        char* task_description;
        struct tasks* next;
        struct tasks* prev;
        struct tasks* loop;
    };

    // date helpers
    int dayOfWeek(int year, int month, int day);
    int isLeap(int year);
    int daysInMonth(int year, int month);

    // calendar creation
    struct years* findOrAddYear(struct years** calendar_head, int year_number);

    // task ops
    void addTask(struct years** calendar_head, int year, int month, int day, const char* desc);
    struct days* getDayNode(struct years* calendar_head, int year, int month, int day);
    int listTasksForDayNode(struct days* day_node);
    int deleteTask(struct years* calendar_head, int year, int month, int day, int task_id);

    // search helpers
    int containsIgnoreCase(const char* text, const char* key);
    void searchTasks(struct years* calendar_head, const char* keyword); // prints results

    // file I/O
    struct years* loadTasks(const char* filename);
    int saveTasks(const char* filename, struct years* calendar_head);

    // memory cleanup
    void freeCalendar(struct years* calendar_head);

#ifdef __cplusplus
}
#endif

#endif
