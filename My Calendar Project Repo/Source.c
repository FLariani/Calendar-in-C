#define _CRT_SECURE_NO_WARNINGS // not required since we're using *_s functions, but harmless in VS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESC_LEN 256

// when 1, addTask won't print "Task added..." (we turn this on during file load)
static int g_silentAdd = 0;

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
};

// =====================
// DATE HELPERS
// =====================

int dayOfWeek(int year, int month, int day) {
    // offset values for each month (this is a standard trick to compute weekday fast)
    static int month_offsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    // Jan/Feb behave like months 13/14 of the previous year in this formula
    if (month < 3) year -= 1;

    int day_of_week = (year + year / 4 - year / 100 + year / 400
        + month_offsets[month - 1] + day) % 7;

    // just in case we ever get a negative (shouldn't happen often, but safe)
    if (day_of_week < 0) day_of_week += 7;

    return day_of_week; // 0=Sunday ... 6=Saturday
}

int isLeap(int year) {
    // leap year rules: divisible by 4, but not 100 unless also divisible by 400
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

int daysInMonth(int year, int month) {
    // returns the correct number of days for that month (handles leap Feb)
    switch (month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        return 31;
    case 4: case 6: case 9: case 11:
        return 30;
    case 2:
        return isLeap(year) ? 29 : 28;
    default:
        return 0; // invalid month
    }
}

// =====================
// YEAR / MONTH / DAY CREATION
// =====================

// finds a year in the list, or creates it if missing
struct years* findOrAddYear(struct years** calendar_head, int year_number) {

    // static arrays so we don't recreate strings every call
    static const char* monthNames[] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static const char* dayNames[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    // search existing years first
    struct years* current_year = *calendar_head;
    while (current_year != NULL) {
        if (current_year->year_number == year_number) return current_year;
        current_year = current_year->next;
    }

    // not found -> create a new year node
    struct years* new_year = (struct years*)malloc(sizeof(struct years));
    if (!new_year) {
        printf("Memory allocation failed for year.\n");
        return NULL;
    }

    new_year->year_number = year_number;
    new_year->next = NULL;

    // allocate 12 months for this year
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

        // allocate the day array for this month (28-31 days)
        new_year->months[m].days = (struct days*)malloc(new_year->months[m].num_days * sizeof(struct days));
        if (!new_year->months[m].days) {
            printf("Memory allocation failed for days in month %d.\n", month_num);

            // cleanup anything we already allocated so we don't leak memory
            for (int prev_m = 0; prev_m < m; prev_m++) {
                free(new_year->months[prev_m].days);
            }
            free(new_year->months);
            free(new_year);
            return NULL;
        }

        // initialize each day in that month
        for (int d = 0; d < new_year->months[m].num_days; d++) {
            int day_num = d + 1;
            new_year->months[m].days[d].day_number = day_num;
            new_year->months[m].days[d].day_name = dayNames[dayOfWeek(year_number, month_num, day_num)];
            new_year->months[m].days[d].tasks_head = NULL; // start with no tasks
        }
    }

    // insert new year at the head of the linked list (fast + simple)
    new_year->next = *calendar_head;
    *calendar_head = new_year;

    return new_year;
}

// =====================
// TASK OPERATIONS
// =====================

// adds a task to the chosen date (year/month/day)
void addTask(struct years** calendar_head, int year, int month, int day, const char* desc) {

    // make sure that year exists (create if needed)
    struct years* year_node = findOrAddYear(calendar_head, year);

    // month validity check
    if (!year_node || month < 1 || month > 12) {
        printf("Invalid month.\n");
        return;
    }

    // day validity check (depends on month + leap years)
    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) {
        printf("Invalid day for this month.\n");
        return;
    }

    struct days* day_node = &month_node->days[day - 1];

    // allocate a task node
    struct tasks* new_task = (struct tasks*)malloc(sizeof(struct tasks));
    if (!new_task) {
        printf("Memory allocation failed for task.\n");
        return;
    }

    // allocate description string exactly the size we need
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

    // add to end of the day's linked list, and assign sequential id
    if (day_node->tasks_head == NULL) {
        day_node->tasks_head = new_task;
        new_task->task_id = 1;
    }
    else {
        struct tasks* current_task = day_node->tasks_head;
        while (current_task->next != NULL) {
            current_task = current_task->next;
        }

        // next ID is last node's id + 1
        new_task->task_id = current_task->task_id + 1;
        current_task->next = new_task;
        new_task->prev = current_task;
    }

    // don't spam output during file load
    if (!g_silentAdd) {
        printf("Task added for %d-%02d-%02d.\n", year, month, day);
    }
}

// helper: find day node safely (used by delete + search UI + menu)
struct days* getDayNode(struct years* calendar_head, int year, int month, int day) {

    // find the year first
    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }
    if (!year_node) return NULL;

    // validate month
    if (month < 1 || month > 12) return NULL;

    // validate day based on month length
    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) return NULL;

    return &month_node->days[day - 1];
}

// prints tasks for a day node with IDs so user can pick one
int listTasksForDayNode(struct days* day_node) {

    // checks if day_node exists and has tasks
    if (!day_node || !day_node->tasks_head) {
        printf("No tasks for this day.\n");
        return 0;
    }

    // count is useful for testing + UI logic
    int count = 0;

    // cycle through tasks and print them
    struct tasks* viewTaskNode = day_node->tasks_head;
    while (viewTaskNode != NULL) {
        printf(" %d. %s\n", viewTaskNode->task_id, viewTaskNode->task_description);
        count++;
        viewTaskNode = viewTaskNode->next;
    }

    return count;
}

// helper function: renumber tasks after deletion so IDs stay clean (1..N)
void renumberTasks(struct days* day_node) {
    if (!day_node) return;

    int id = 1;
    struct tasks* numberTasks = day_node->tasks_head;

    while (numberTasks != NULL) {
        numberTasks->task_id = id++;
        numberTasks = numberTasks->next;
    }
}

// update a task's description by its task_id
// returns 0 on success, 1 on error (kept simple for menu logic)
int updateTask(struct years* calendar_head, int year, int month, int day, int task_id, const char* new_desc) {

    struct days* day_node = getDayNode(calendar_head, year, month, day);

    // date invalid or year not loaded
    if (!day_node) {
        printf("Invalid date / year not found.\n");
        return 1;
    }

    // find the task by task_id
    struct tasks* updateDay = day_node->tasks_head;
    while (updateDay != NULL && updateDay->task_id != task_id) {
        updateDay = updateDay->next;
    }

    // invalid task id
    if (!updateDay) {
        printf("Task %d not found on %d-%02d-%02d.\n", task_id, year, month, day);
        return 1;
    }

    // free the old description and replace it
    free(updateDay->task_description);

    size_t desc_len = strlen(new_desc) + 1;
    updateDay->task_description = (char*)malloc(desc_len);
    if (!updateDay->task_description) {

        // if malloc fails, we don't want a dangling pointer
        printf("Memory allocation failed for new task description.\n");
        updateDay->task_description = (char*)malloc(1);
        if (updateDay->task_description) {
            updateDay->task_description[0] = '\0';
        }
        return 1;
    }

    strcpy_s(updateDay->task_description, desc_len, new_desc);

    printf("Updated task %d on %d-%02d-%02d.\n", task_id, year, month, day);
    return 0;
}

// delete a task by task_id from a specific date
int deleteTask(struct years* calendar_head, int year, int month, int day, int task_id) {

    struct days* day_node = getDayNode(calendar_head, year, month, day);

    // date invalid or year not loaded
    if (!day_node) {
        printf("Invalid date / year not found.\n");
        return 0;
    }

    // no tasks to delete
    if (!day_node->tasks_head) {
        printf("No tasks to delete for %d-%02d-%02d.\n", year, month, day);
        return 0;
    }

    // find the node with the matching id
    struct tasks* deleteNode = day_node->tasks_head;
    while (deleteNode != NULL && deleteNode->task_id != task_id) {
        deleteNode = deleteNode->next;
    }

    // task id not found
    if (!deleteNode) {
        printf("Task %d not found on %d-%02d-%02d.\n", task_id, year, month, day);
        return 0;
    }

    // unlink from doubly linked list
    if (deleteNode->prev != NULL) {
        deleteNode->prev->next = deleteNode->next;
    }
    else {
        // deleting head
        day_node->tasks_head = deleteNode->next;
    }

    if (deleteNode->next != NULL) {
        deleteNode->next->prev = deleteNode->prev;
    }

    // free heap memory
    free(deleteNode->task_description);
    free(deleteNode);

    // keep IDs clean after deletes (avoids gaps like 1,2,4)
    renumberTasks(day_node);

    printf("Deleted task %d from %d-%02d-%02d.\n", task_id, year, month, day);
    return 1;
}

// =====================
// PRINT FUNCTIONS
// =====================

// prints all tasks for one specific date
void printTasksForDay(struct years* calendar_head, int year, int month, int day) {

    // find year
    struct years* year_node = calendar_head;
    while (year_node != NULL && year_node->year_number != year) {
        year_node = year_node->next;
    }

    // validate month
    if (!year_node || month < 1 || month > 12) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
        return;
    }

    // validate day
    struct months* month_node = &year_node->months[month - 1];
    if (day < 1 || day > month_node->num_days) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
        return;
    }

    // access day node and its tasks
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

// prints a month in an ASCII grid, and marks days with tasks using '*'
void printMonthCalendar(struct years* calendar_head, int year, int month) {

    if (month < 1 || month > 12) {
        printf("Invalid month.\n");
        return;
    }

    // use the same year node for title + task checks (creates year if missing)
    struct years* year_node = findOrAddYear(&calendar_head, year);
    if (!year_node) {
        printf("Error accessing year.\n");
        return;
    }

    int nDays = daysInMonth(year, month);
    int firstWeekday = dayOfWeek(year, month, 1);

    const char* month_title = year_node->months[month - 1].month_name;

    // this width is just used for centering the title
    const int calendar_width = 31;

    // calculate the number of digits in the year (for title length)
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

    // total title length: "Month" + space + "Year"
    int title_len = (int)strlen(month_title) + 1 + year_digits;

    // offset is how many spaces to print before the title to center it
    int offset = (calendar_width - title_len) / 2;

    // fix: % runs before -, so we need parentheses or centering is off
    if (((calendar_width - title_len) % 2) != 0) {
        offset--;
    }

    printf("\n");

    // left padding for centering
    for (int i = 0; i < offset; i++) {
        printf(" ");
    }

    printf("%s %d\n", month_title, year);

    printf("_____________________________\n");
    printf("|Su |Mo |Tu |We |Th |Fr |Sa |\n");
    printf("|___|___|___|___|___|___|___|\n|");

    // leading empty cells before day 1
    for (int i = 0; i < firstWeekday; i++) {
        printf("   |");
    }

    // print each day cell
    for (int d = 1; d <= nDays; d++) {

        // mark with * if that day has at least one task
        int has_task = (year_node->months[month - 1].days[d - 1].tasks_head != NULL);

        // each cell is 3 chars wide (plus the '|')
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

        // end of week (Saturday) -> newline + row line
        if ((firstWeekday + d - 1) % 7 == 6) {
            printf("|\n");
            if (d < nDays) {
                printf("|___|___|___|___|___|___|___|\n|");
            }
        }
        else {
            printf("|");
        }
    }

    // trailing empty cells to finish last row cleanly
    int lastDayWeekday = (firstWeekday + nDays - 1) % 7;
    if (lastDayWeekday != 6) {
        for (int i = lastDayWeekday; i < 6; i++) {
            printf("   |");
        }
        printf("\n");
    }

    printf("|___|___|___|___|___|___|___|\n");
    printf("\n* = day has one or more tasks.\n");
}

// compact month view: prints only days with tasks, and puts tasks on one line
void printTasksForMonthPretty(struct years* calendar_head, int year, int month) {

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

            printf("%2d (%s): ", day_node->day_number, day_node->day_name);

            // print all tasks comma-separated
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

// compact year view: groups by month, prints only days that have tasks
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
        int month_printed = 0;

        for (int d = 0; d < month_node->num_days; d++) {

            struct days* day_node = &month_node->days[d];
            struct tasks* t = day_node->tasks_head;

            if (t != NULL) {
                found_any = 1;

                // only print the month header once
                if (!month_printed) {
                    printf("\n-- %s --\n", month_node->month_name);
                    month_printed = 1;
                }

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

// simple case-insensitive "contains" check (no libraries needed)
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

            // manual lowercase conversion so we don't need extra headers
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');

            if (a != b) break;
            j++;
        }

        if (j == m) return 1;
    }

    return 0;
}

// keyword search across every loaded year/month/day
// prints matches with the date so the user can actually find them again
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

    // silent mode so addTask doesn't print a line for every task in the file
    g_silentAdd = 1;

    struct years* calendar_head = NULL;
    char line[512];
    int current_year = 0;

    while (fgets(line, sizeof(line), fp)) {

        // year marker line: [YEAR] 2025
        if (sscanf_s(line, "[YEAR] %d", &current_year) == 1) {
            findOrAddYear(&calendar_head, current_year);
        }
        else if (current_year != 0) {

            int month, day;
            char desc[DESC_LEN] = "";

            // reads: month day description... (description can include spaces)
            if (sscanf_s(line, "%d %d %[^\n]", &month, &day, desc, (unsigned)DESC_LEN) >= 2) {
                addTask(&calendar_head, current_year, month, day, desc);
            }
        }
    }

    fclose(fp);

    // back to normal mode
    g_silentAdd = 0;

    return calendar_head;
}

int saveTasks(const char* filename, struct years* calendar_head) {

    FILE* fp;
    fopen_s(&fp, filename, "w");
    if (!fp) return 0;

    struct years* current_year = calendar_head;

    while (current_year != NULL) {

        // write a year header so loading is easy
        fprintf(fp, "[YEAR] %d\n", current_year->year_number);

        for (int m = 0; m < 12; m++) {
            for (int d = 0; d < current_year->months[m].num_days; d++) {

                struct tasks* current_task = current_year->months[m].days[d].tasks_head;

                while (current_task != NULL) {

                    // Save: month day description (no task_id needed)
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

// frees every task, then days arrays, then months array, then years list
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
        printf("9. Update task\n");
        printf("0. Save and exit\n");
        printf("Choice: ");

        // basic input validation
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

            // clear input buffer so fgets works correctly
            int ch; while ((ch = getchar()) != '\n' && ch != EOF);

            printf("Enter task description: ");
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                printf("Error reading description.\n");
                continue;
            }

            // strip newline from fgets
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

            // clear input buffer so fgets works
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

            // create/find the year so month calendars can print safely
            struct years* year_node = findOrAddYear(calendar_head, y);
            if (!year_node) {
                printf("Error: Could not create or find calendar for year %d.\n", y);
                continue;
            }

            // quick title centering (simple, not perfect, but good enough)
            int calendar_width = 31;

            // build the title string so we can measure it properly
            char title[64];
            sprintf_s(title, sizeof(title), "===Calendar of %d===", y);

            int title_len = (int)strlen(title);
            int offset = (calendar_width - title_len) / 2;

            // fix: same precedence issue as month title
            if (((calendar_width - title_len) % 2) != 0) {
                offset--;
            }

            printf("\n");
            for (int i = 0; i < offset; i++) {
                printf(" ");
            }
            printf("%s\n", title);

            // print all 12 months one after the other
            for (int m = 1; m <= 12; m++) {
                printMonthCalendar(*calendar_head, y, m);
            }
        }
        else if (choice == 9) {

            int y, m, d, id;
            char buffer[DESC_LEN];

            printf("Enter year month day to update a task (e.g. 2025 11 29): ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid date input.\n");
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

            printf("Enter the task number to update: ");
            if (scanf_s("%d", &id) != 1) {
                printf("Invalid input.\n");
                int ch; while ((ch = getchar()) != '\n' && ch != EOF);
                continue;
            }

            // clear buffer so fgets works
            int ch; while ((ch = getchar()) != '\n' && ch != EOF);

            printf("Enter the new task description: ");
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                printf("Error reading description.\n");
                continue;
            }

            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

            updateTask(*calendar_head, y, m, d, id, buffer);
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

    // If no calendar is loaded, ask the user what year to start with
    if (!calendar) {

        printf("No calendar file found or file is empty.\n");

        int start_year = 0;
        printf("What year would you like to start with? ");

        // keep asking until we get a valid integer year
        while (scanf_s("%d", &start_year) != 1 || start_year < 1) {

            printf("Invalid input. Please enter a valid year (e.g., 2025): ");

            // clear input buffer in case of non-numeric input
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        }

        // create the initial year structure
        findOrAddYear(&calendar, start_year);

        // save immediately so tasks.txt exists for the next run
        if (!saveTasks("tasks.txt", calendar)) {
            printf("Error: Could not save initial calendar file.\n");
        }
        else {
            printf("Calendar for %d created and saved.\n", start_year);
        }
    }

    // run menu UI
    menu(&calendar);

    // save back to disk on exit
    if (!saveTasks("tasks.txt", calendar)) {
        printf("Error saving tasks to file.\n");
    }

    // free everything before exiting
    freeCalendar(calendar);

    return 0;
}
