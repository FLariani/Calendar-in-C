#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESC_LEN 256

struct years;
struct months;
struct days;
struct tasks;

struct years {
    int year_number;
    struct months* months;   // array of 12 months
    struct years* next;      // linked list of years
};

struct months {
    int month_number;
    const char* month_name;
    struct days* days;       // array of days in this month
    int num_days;
};

struct days {
    int day_number;
    const char* day_name;
    struct tasks* tasks_head; // doubly linked list of tasks for this day
};

struct tasks {
    int task_id;
    char* task_description;
    struct tasks* next;
    struct tasks* prev;
    struct tasks* loop;
};

// =====================
// DATE HELPERS
// =====================

int dayOfWeek(int year, int month, int day) {
    // offset values for each month
    static int month_offsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    if (month < 3) year -= 1;

    int day_of_week = (year + year / 4 - year / 100 + year / 400
        + month_offsets[month - 1] + day) % 7;

    if (day_of_week < 0) day_of_week += 7;
    return day_of_week; // 0=Sunday ... 6=Saturday
}

int isLeap(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

int daysInMonth(int year, int month) {
    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:
        return isLeap(year) ? 29 : 28;
    default:
        return 0;
    }
}

// =====================
// YEAR / MONTH / DAY CREATION
// =====================

// finds a year in the list, or creates it if missing
struct years* findOrAddYear(struct years** calendar_head, int year_number) {

    // static name arrays so we don't recreate them every call
    static const char* monthNames[] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static const char* dayNames[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    // search existing years
    struct years* current_year = *calendar_head;
    while (current_year != NULL) {
        if (current_year->year_number == year_number) return current_year;
        current_year = current_year->next;
    }

    // not found -> create new year node
    struct years* new_year = (struct years*)malloc(sizeof(struct years));
    if (!new_year) {
        printf("Memory allocation failed for year.\n");
        return NULL;
    }

    new_year->year_number = year_number;
    new_year->next = NULL;

    // allocate 12 months
    new_year->months = (struct months*)malloc(12 * sizeof(struct months));
    if (!new_year->months) {
        printf("Memory allocation failed for months.\n");
        free(new_year);
        return NULL;
    }

    // set up months + allocate days arrays
    for (int m = 0; m < 12; m++) {
        int month_num = m + 1;

        new_year->months[m].month_number = month_num;
        new_year->months[m].month_name = monthNames[month_num];
        new_year->months[m].num_days = daysInMonth(year_number, month_num);

        new_year->months[m].days = (struct days*)malloc(new_year->months[m].num_days * sizeof(struct days));
        if (!new_year->months[m].days) {
            printf("Memory allocation failed for days in month %d.\n", month_num);

            // cleanup months already allocated
            for (int prev_m = 0; prev_m < m; prev_m++) {
                free(new_year->months[prev_m].days);
            }
            free(new_year->months);
            free(new_year);
            return NULL;
        }

        // init each day
        for (int d = 0; d < new_year->months[m].num_days; d++) {
            int day_num = d + 1;
            new_year->months[m].days[d].day_number = day_num;
            new_year->months[m].days[d].day_name = dayNames[dayOfWeek(year_number, month_num, day_num)];
            new_year->months[m].days[d].tasks_head = NULL;
        }
    }

    // insert new year at head of list (fast + simple)
    new_year->next = *calendar_head;
    *calendar_head = new_year;

    return new_year;
}

// =====================
// TASK OPERATIONS
// =====================

void addTask(struct years** calendar_head, int year, int month, int day, const char* desc) {

    struct years* year_node = findOrAddYear(calendar_head, year);
    if (!year_node || month < 1 || month > 12) {
        printf("Invalid month.\n");
        return;
    }

    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) {
        printf("Invalid day for this month.\n");
        return;
    }

    struct days* day_node = &month_node->days[day - 1];

    // allocate task node
    struct tasks* new_task = (struct tasks*)malloc(sizeof(struct tasks));
    if (!new_task) {
        printf("Memory allocation failed for task.\n");
        return;
    }

    // allocate description string
    size_t desc_len = strlen(desc) + 1;
    new_task->task_description = (char*)malloc(desc_len);
    if (!new_task->task_description) {
        printf("Memory allocation failed for task description.\n");
        free(new_task);
        return;
    }

    strcpy_s(new_task->task_description, desc_len, desc);
    new_task->next = NULL;
    new_task->prev = NULL;
    new_task->loop = NULL;

    // add to end of that day's list, assign sequential id
    if (day_node->tasks_head == NULL) {
        day_node->tasks_head = new_task;
        new_task->task_id = 1;
    }
    else {
        struct tasks* current_task = day_node->tasks_head;
        while (current_task->next != NULL) {
            current_task = current_task->next;
        }

        // simple + clear: next ID is the last node's id + 1
        new_task->task_id = current_task->task_id + 1;
        current_task->next = new_task;
        new_task->prev = current_task;
    }

    printf("Task added for %d-%02d-%02d.\n", year, month, day);
}

// helper: find day node safely (used by delete + search UI)
struct days* getDayNode(struct years* calendar_head, int year, int month, int day) {

    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }
    if (!year_node) return NULL;

    if (month < 1 || month > 12) return NULL;

    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) return NULL;

    return &month_node->days[day - 1];
}

// prints tasks for a day node with IDs so user can pick one
int listTasksForDayNode(struct days* day_node) {

    if (!day_node || !day_node->tasks_head) {
        printf("No tasks for this day.\n");
        return 0;
    }

    int count = 0;
    struct tasks* t = day_node->tasks_head;
    while (t != NULL) {
        printf(" %d. %s\n", t->task_id, t->task_description);
        count++;
        t = t->next;
    }
    return count;
}
void renumberTasks(struct days* day_node) {
    if (!day_node) return;

    int id = 1;
    struct tasks* t = day_node->tasks_head;
    while (t != NULL) {
        t->task_id = id++;
        t = t->next;
    }
}


// delete a task by task_id from a specific date
int deleteTask(struct years* calendar_head, int year, int month, int day, int task_id) {

    struct days* day_node = getDayNode(calendar_head, year, month, day);

    // date invalid or year not loaded
    if (!day_node) {
        printf("Invalid date / year not found.\n");
        return 0;
    }

    if (!day_node->tasks_head) {
        printf("No tasks to delete for %d-%02d-%02d.\n", year, month, day);
        return 0;
    }

    // find the node with the matching id
    struct tasks* curr = day_node->tasks_head;
    while (curr != NULL && curr->task_id != task_id) {
        curr = curr->next;
    }

    if (!curr) {
        printf("Task %d not found on %d-%02d-%02d.\n", task_id, year, month, day);
        return 0;
    }

    // unlink from doubly linked list
    if (curr->prev != NULL) {
        curr->prev->next = curr->next;
    }
    else {
        // deleting head
        day_node->tasks_head = curr->next;
    }

    if (curr->next != NULL) {
        curr->next->prev = curr->prev;
    }

    // free heap memory
    free(curr->task_description);
    free(curr);

    // keeps task ids clean (1..N) after deletes
    renumberTasks(day_node);

    printf("Deleted task %d from %d-%02d-%02d.\n", task_id, year, month, day);
    return 1;
}

// =====================
// PRINT FUNCTIONS
// =====================

void printTasksForDay(struct years* calendar_head, int year, int month, int day) {

    // find year
    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }

    if (!year_node || month < 1 || month > 12) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
        return;
    }

    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
        return;
    }

    struct days* day_node = &month_node->days[day - 1];
    struct tasks* task_node = day_node->tasks_head;

    if (task_node == NULL) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
        return;
    }

    printf("Tasks for %s, %s %d, %d:\n",
        day_node->day_name, month_node->month_name, day, year);

    while (task_node != NULL) {
        printf(" %d. %s\n", task_node->task_id, task_node->task_description);
        task_node = task_node->next;
    }
}

// FIXED printMonthCalendar: always use ONE year pointer (so stars are accurate)
void printMonthCalendar(struct years* calendar_head, int year, int month) {

    if (month < 1 || month > 12) {
        printf("Invalid month.\n");
        return;
    }

    // Important fix: use the same year node for title + task checks
    struct years* year_node = findOrAddYear(&calendar_head, year);
    if (!year_node) {
        printf("Error accessing year.\n");
        return;
    }

    int nDays = daysInMonth(year, month);
    int firstWeekday = dayOfWeek(year, month, 1);

    const char* month_title = year_node->months[month - 1].month_name;

    const int calendar_width = 31;

    // Calculate the number of digits in the year
    int year_digits = 0;
    int temp_year = year;
    if (temp_year == 0) {
        year_digits = 1;
    }
    else {
        while (temp_year != 0) {
            temp_year /= 10;
            year_digits++;
        }
    }

    // Calculate total title length: "Month" + " " + "Year"
    int title_len = (int)strlen(month_title) + 1 + year_digits;
    int offset = (calendar_width - title_len) / 2;
    
    if (calendar_width - title_len % 2 != 0) {
        offset--;
    }
    printf("\n");
    // Print the left padding
    for (int i = 0; i < offset; i++) {
        printf(" ");
    }
    // Print the title components separately
    printf("%s %d\n", month_title, year);
    printf("_____________________________\n");
    // Adjusted header spacing to match the 3-character cell width
    printf("|Su |Mo |Tu |We |Th |Fr |Sa |\n");
    printf("|___|___|___|___|___|___|___|\n|");

    // Print leading spaces for the first week
    for (int i = 0; i < firstWeekday; i++) {
        printf("   |"); // 3 spaces for each empty day
    }

    // Print each day of the month
    for (int d = 1; d <= nDays; d++) {
        int has_task = (year_node->months[month - 1].days[d - 1].tasks_head != NULL);

        // Each day cell is 3 characters wide
        if (has_task && d < 10) {
            printf("%1d* ", d);
        }
        else if (has_task) {
            printf("%2d*", d);
        }
        else if (!has_task && d < 10) {
            printf("%1d  ", d);
        }
        else {
            printf("%2d ", d);
        }

        // If it's the last day of the week (Saturday), print a newline
        if ((firstWeekday + d - 1) % 7 == 6) {
            printf("|\n");
            // Only print the next row if it's not the end of the calendar
            if (d < nDays) {
                printf("|___|___|___|___|___|___|___|\n|");
            }
        }
        // Otherwise, print a single space to separate the days
        else {
            printf("|");
        }
    }

    // Add trailing empty cells for the last week
    int lastDayWeekday = (firstWeekday + nDays - 1) % 7;
    if (lastDayWeekday != 6) {
        for (int i = lastDayWeekday; i < 6; i++) {
            printf("   |");
        }
        printf("\n");
    }

    printf("|___|___|___|___|___|___|___|\n");
    printf("\n* = day has one or more tasks.\n\n");
}
void printTasksForMonthPretty(struct years* calendar_head, int year, int month) {

    // basic validation
    if (month < 1 || month > 12) {
        printf("Invalid month.\n");
        return;
    }

    // find year node
    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }

    if (!year_node) {
        printf("No data for year %d.\n", year);
        return;
    }

    struct months* month_node = &year_node->months[month - 1];

    printf("\n=== %s %d ===\n", month_node->month_name, year);

    int found_any = 0;

    for (int d = 0; d < month_node->num_days; d++) {

        struct days* day_node = &month_node->days[d];
        struct tasks* t = day_node->tasks_head;

        if (t != NULL) {
            found_any = 1;

            // Day header: "29 (Saturday):"
            printf("%2d (%s): ", day_node->day_number, day_node->day_name);

            // Print all tasks on one line, comma-separated
            while (t != NULL) {
                printf("%s", t->task_description);
                if (t->next != NULL) {
                    printf(", ");
                }
                t = t->next;
            }

            printf("\n");
        }
    }

    if (!found_any) {
        printf("No tasks stored for %s %d.\n", month_node->month_name, year);
    }

    printf("\n");
}

void printTasksForYearPretty(struct years* calendar_head, int year) {

    // find year node
    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }

    if (!year_node) {
        printf("No data for year %d.\n", year);
        return;
    }

    printf("\n=== Tasks for %d ===\n", year);

    int found_any = 0;

    for (int m = 0; m < 12; m++) {

        struct months* month_node = &year_node->months[m];
        int month_printed = 0;  // so we only print the month header if it has tasks

        for (int d = 0; d < month_node->num_days; d++) {

            struct days* day_node = &month_node->days[d];
            struct tasks* t = day_node->tasks_head;

            if (t != NULL) {
                found_any = 1;

                // print month header once, the first time we hit a day with tasks
                if (!month_printed) {
                    printf("\n-- %s --\n", month_node->month_name);
                    month_printed = 1;
                }

                // day line: "29 (Saturday): Task A, Task B"
                printf("%2d (%s): ", day_node->day_number, day_node->day_name);

                while (t != NULL) {
                    printf("%s", t->task_description);
                    if (t->next != NULL) {
                        printf(", ");
                    }
                    t = t->next;
                }
                printf("\n");
            }
        }
    }

    if (!found_any) {
        printf("No tasks stored for %d.\n", year);
    }

    printf("\n");
}



// =====================
// SEARCH FEATURE
// =====================

// simple case-insensitive "contains" check
int containsIgnoreCase(const char* text, const char* key) {
    if (!text || !key) return 0;

    size_t n = strlen(text);
    size_t m = strlen(key);
    if (m == 0) return 1;
    if (m > n) return 0;

    for (size_t i = 0; i <= n - m; i++) {
        size_t j = 0;
        while (j < m) {
            char a = text[i + j];
            char b = key[j];

            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');

            if (a != b) break;
            j++;
        }
        if (j == m) return 1;
    }
    return 0;
}

// searches all tasks in all loaded years for a keyword
void searchTasks(struct years* calendar_head, const char* keyword) {

    if (!keyword || keyword[0] == '\0') {
        printf("Search keyword can't be empty.\n");
        return;
    }

    int found = 0;
    struct years* y = calendar_head;

    while (y != NULL) {
        for (int m = 0; m < 12; m++) {
            for (int d = 0; d < y->months[m].num_days; d++) {

                struct tasks* t = y->months[m].days[d].tasks_head;
                while (t != NULL) {
                    if (containsIgnoreCase(t->task_description, keyword)) {
                        if (!found) {
                            printf("\nSearch results for \"%s\":\n", keyword);
                        }
                        found = 1;

                        printf(" - %d-%02d-%02d (Task %d): %s\n",
                            y->year_number,
                            y->months[m].month_number,
                            y->months[m].days[d].day_number,
                            t->task_id,
                            t->task_description);
                    }
                    t = t->next;
                }
            }
        }
        y = y->next;
    }

    if (!found) {
        printf("No tasks found containing \"%s\".\n", keyword);
    }
    else {
        printf("\n");
    }
}

// =====================
// FILE I/O
// =====================
//
// File format (simple and consistent):
// [YEAR] 2025
// 11 29 Finish assignment
// 12 25 Christmas Day
// [YEAR] 2026
// 1 1 New Year's Day
//
// We intentionally do NOT store task_id because addTask() rebuilds them.
//

struct years* loadTasks(const char* filename) {

    FILE* fp;
    fopen_s(&fp, filename, "r");
    if (!fp) {
        return NULL;
    }

    struct years* calendar_head = NULL;
    char line[512];
    int current_year = 0;

    while (fgets(line, sizeof(line), fp)) {

        // year marker line
        if (sscanf_s(line, "[YEAR] %d", &current_year) == 1) {
            findOrAddYear(&calendar_head, current_year);
        }
        else if (current_year != 0) {
            int month, day;
            char desc[DESC_LEN] = "";

            // month day description...
            if (sscanf_s(line, "%d %d %[^\n]", &month, &day, desc, (unsigned)DESC_LEN) >= 2) {
                addTask(&calendar_head, current_year, month, day, desc);
            }
        }
    }

    fclose(fp);
    return calendar_head;
}

int saveTasks(const char* filename, struct years* calendar_head) {

    FILE* fp;
    fopen_s(&fp, filename, "w");
    if (!fp) return 0;

    struct years* current_year = calendar_head;
    while (current_year != NULL) {

        fprintf(fp, "[YEAR] %d\n", current_year->year_number);

        for (int m = 0; m < 12; m++) {
            for (int d = 0; d < current_year->months[m].num_days; d++) {

                struct tasks* current_task = current_year->months[m].days[d].tasks_head;
                while (current_task != NULL) {

                    // Save month day description (no task_id)
                    fprintf(fp, "%d %d %s\n",
                        current_year->months[m].month_number,
                        current_year->months[m].days[d].day_number,
                        current_task->task_description);

                    current_task = current_task->next;
                }
            }
        }

        current_year = current_year->next;
    }

    fclose(fp);
    return 1;
}

// =====================
// FREE ALL MEMORY
// =====================

void freeCalendar(struct years* calendar_head) {

    struct years* current_year = calendar_head;
    while (current_year != NULL) {

        for (int m = 0; m < 12; m++) {
            for (int d = 0; d < current_year->months[m].num_days; d++) {

                struct tasks* current_task = current_year->months[m].days[d].tasks_head;
                while (current_task != NULL) {
                    struct tasks* next_task = current_task->next;
                    free(current_task->task_description);
                    free(current_task);
                    current_task = next_task;
                }
            }
            free(current_year->months[m].days);
        }

        free(current_year->months);
        struct years* next_year = current_year->next;
        free(current_year);
        current_year = next_year;
    }
}

// =====================
// MENU / UI
// =====================

void menu(struct years** calendar_head) {

    int choice;
    do {
        printf("\n=== Simple Calendar ===\n");
        printf("1. Add task\n");
        printf("2. Show calendar for a month\n");
        printf("3. View tasks for a specific day\n");
        printf("4. Delete a task\n");
        printf("5. Search tasks\n");
        printf("6. View all tasks for a month\n");
        printf("7. View all tasks for a year\n");
        printf("8. Show Calendar for a year\n");
        printf("0. Save and exit\n");
        printf("Choice: ");

        if (scanf_s("%d", &choice) != 1) {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF);
            choice = -1;
            continue;
        }

        if (choice == 1) {
            int y, m, d;
            char buffer[DESC_LEN];

            printf("Enter year month day (e.g. 2025 11 29): ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid date input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            int ch; while ((ch = getchar()) != '\n' && ch != EOF);
            printf("Enter task description: ");
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                printf("Error reading description.\n");
                continue;
            }

            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

            addTask(calendar_head, y, m, d, buffer);
        }
        else if (choice == 2) {
            int y, m;
            printf("Enter year and month (e.g. 2025 11): ");
            if (scanf_s("%d %d", &y, &m) != 2) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            printMonthCalendar(*calendar_head, y, m);
        }
        else if (choice == 3) {
            int y, m, d;
            printf("Enter year month day: ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            printTasksForDay(*calendar_head, y, m, d);
        }
        else if (choice == 4) {
            int y, m, d;
            printf("Enter year month day to delete from (e.g. 2025 11 29): ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            struct days* day_node = getDayNode(*calendar_head, y, m, d);
            if (!day_node || !day_node->tasks_head) {
                printf("No tasks for %d-%02d-%02d.\n", y, m, d);
                continue;
            }

            printf("Tasks for %d-%02d-%02d:\n", y, m, d);
            listTasksForDayNode(day_node);

            int id;
            printf("Enter the task number to delete: ");
            if (scanf_s("%d", &id) != 1) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            deleteTask(*calendar_head, y, m, d, id);
        }
        else if (choice == 5) {
            char keyword[DESC_LEN];

            int ch; while ((ch = getchar()) != '\n' && ch != EOF);
            printf("Enter keyword to search: ");
            if (!fgets(keyword, sizeof(keyword), stdin)) {
                printf("Error reading keyword.\n");
                continue;
            }

            size_t len = strlen(keyword);
            if (len > 0 && keyword[len - 1] == '\n') keyword[len - 1] = '\0';

            searchTasks(*calendar_head, keyword);
        }
        else if (choice == 6) {

            int y, m;
            printf("Enter year and month (e.g. 2025 11): ");
            if (scanf_s("%d %d", &y, &m) != 2) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            printTasksForMonthPretty(*calendar_head, y, m);
        }

        else if (choice == 7) {

            int y;
            printf("Enter year (e.g. 2025): ");
            if (scanf_s("%d", &y) != 1) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            printTasksForYearPretty(*calendar_head, y);
        }

        else if (choice == 8) {
            int y;
            printf("Enter year (e.g. 2025): ");
            if (scanf_s("%d", &y) != 1) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }
            // Calculate the number of digits in the year
            int calendar_width = 31;
            int year_digits = 0;
            int temp_year = y;
            if (temp_year == 0) {
                year_digits = 1;
            }
            else {
                while (temp_year != 0) {
                    temp_year /= 10;
                    year_digits++;
                }
            }

            // This will create the year if it's not already loaded.
            struct years* year_node = findOrAddYear(calendar_head, y);

            // Check if the year was created successfully (it could fail on memory allocation).
            if (!year_node) {
                printf("Error: Could not create or find calendar for year %d.\n", y);
                continue; // Go back to the menu
            }

            // Calculate total title length
            int title_len = year_digits;
            int offset = (calendar_width - title_len - 18) / 2;
            printf("\n");
            if (calendar_width - title_len % 2 != 0) {
                offset--;
            }

            // Print the left padding
            for (int i = 0; i < offset; i++) {
                printf(" ");
            }
            printf("===Calendar of %d===\n", y);
            for (int m = 1; m < 13; m++) {
                printMonthCalendar(*calendar_head, y, m);
            }
        }

        else if (choice == 0) {
            printf("Saving and exiting...\n");
        }
        else {
            printf("Invalid choice.\n");
        }

    } while (choice != 0);
}

// =====================
// MAIN
// =====================

int main(void) {

    // Load existing calendar from disk if it exists
    struct years* calendar = loadTasks("tasks.txt");

    // If no calendar is loaded, prompt for the initial year to create
    if (!calendar) {
        printf("No calendar file found or file is empty.\n");
        int start_year = 0;
        printf("What year would you like to start with? ");

        // Input validation loop to ensure a valid year is entered
        while (scanf_s("%d", &start_year) != 1 || start_year < 1) {
            printf("Invalid input. Please enter a valid year (e.g., 2025): ");
            // Clear the input buffer in case of non-numeric input
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        }

        // Create the initial year structure based on user input
        findOrAddYear(&calendar, start_year);

        // Save the newly created year immediately to ensure it persists
        if (!saveTasks("tasks.txt", calendar)) {
            printf("Error: Could not save initial calendar file.\n");
        }
        else {
            printf("Calendar for %d created and saved.\n", start_year);
        }
    }

    // run UI
    menu(&calendar);

    // save back to disk on exit
    if (!saveTasks("tasks.txt", calendar)) {
        printf("Error saving tasks to file.\n");
    }

    // free everything
    freeCalendar(calendar);
    return 0;
}

