#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESC_LEN 100   // max task description length


// DATA STRUCTURE


typedef struct Task {
    int year;
    int month;  // 1-12
    int day;    // 1-31
    char desc[DESC_LEN];
    struct Task* next;
} Task;


// FUNCTION PROTOTYPES


Task* loadTasks(const char* filename);
int   saveTasks(const char* filename, Task* head);
Task* addTask(Task** head, int year, int month, int day, const char* desc);
void  freeTasks(Task* head);

void  printMonthCalendar(int year, int month, Task* head);
void  printTasksForDay(Task* head, int year, int month, int day);
int   hasTask(Task* head, int year, int month, int day);
int   daysInMonth(int year, int month);
int   dayOfWeek(int year, int month, int day); // 0 = Sunday ... 6 = Saturday

void  menu(Task** head);


// MAIN


int main(void) {
    Task* head = loadTasks("tasks.txt");   // load existing tasks if file exists

    menu(&head);

    if (!saveTasks("tasks.txt", head)) {
        printf("Error saving tasks to file.\n");
    }

    freeTasks(head);
    return 0;
}


// DATE HELPERS


// Sakamoto's algorithm: 0 = Sunday, 1 = Monday, ... 6 = Saturday
int dayOfWeek(int year, int month, int day) {
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (month < 3) {
        year -= 1;
    }
    int w = (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
    if (w < 0) w += 7;
    return w;
}

int isLeap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
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
        return 30;
    }
}

// LINKED LIST / TASKS

Task* addTask(Task** head, int year, int month, int day, const char* desc) {

    Task* node = (Task*)malloc(sizeof(Task));
    if (!node) {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    node->year = year;
    node->month = month;
    node->day = day;
    strncpy_s(node->desc, DESC_LEN, desc, _TRUNCATE);
    node->desc[DESC_LEN - 1] = '\0';
    node->next = NULL;

    // simplest: insert at front of list
    node->next = *head;
    *head = node;
    return node;
}

void freeTasks(Task* head) {
    while (head) {
        Task* next = head->next;
        free(head);
        head = next;
    }
}

int hasTask(Task* head, int year, int month, int day) {
    for (Task* p = head; p != NULL; p = p->next) {
        if (p->year == year && p->month == month && p->day == day) {
            return 1;
        }
    }
    return 0;
}

void printTasksForDay(Task* head, int year, int month, int day) {
    int found = 0;
    for (Task* p = head; p != NULL; p = p->next) {
        if (p->year == year && p->month == month && p->day == day) {
            if (!found) {
                printf("Tasks for %d-%02d-%02d:\n", year, month, day);
            }
            found = 1;
            printf(" - %s\n", p->desc);
        }
    }
    if (!found) {
        printf("No tasks for %d-%02d-%02d.\n", year, month, day);
    }
}

// CALENDAR GRID


void printMonthCalendar(int year, int month, Task* head) {
    static const char* monthNames[] = {
        "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    int nDays = daysInMonth(year, month);
    int firstWeekday = dayOfWeek(year, month, 1); // 0 = Sunday

    printf("\n   %s %d\n", monthNames[month], year);
    printf("Su Mo Tu We Th Fr Sa\n");

    // leading spaces before the 1st
    for (int i = 0; i < firstWeekday; ++i) {
        printf("   ");
    }

    for (int d = 1; d <= nDays; ++d) {
        int weekday = (firstWeekday + d - 1) % 7;
        int mark = hasTask(head, year, month, d);

        // each cell is 3 characters wide:
        // "%2d*" or "%2d " for pretty alignment
        if (mark) {
            printf("%2d*", d);  // star means this day has at least one task
        }
        else {
            printf("%2d ", d);
        }

        if (weekday == 6) { // Saturday new line
            printf("\n");
        }
        else {
            printf(" ");
        }
    }
    printf("\n\n* = day has one or more tasks. Use 'View tasks for a day' to see details.\n\n");
}


// FILE I/O

//
// We use a VERY simple format:
//
//   year month day description...
//
// Example:
//   2025 11 29 Finish assignment
//
// Description can contain spaces.


Task* loadTasks(const char* filename) {


    FILE* fp = fopen(filename, "r");
    if (!fp) {
        // No file yet no tasks in memory
        return NULL;
    }

    Task* head = NULL;
    char  line[256];

    while (fgets(line, sizeof(line), fp)) {
        int year = 0;
        int month = 0;
        int day = 0;
        char desc[DESC_LEN] = "";

        int count = sscanf(line, "%d %d %d %[^\n]", &year, &month, &day, desc);

        if (count >= 3) {
            if (count == 3) {   // no description on that line
                desc[0] = '\0';
            }
            addTask(&head, year, month, day, desc);
        }
    }


    fclose(fp);
    return head;
}

int saveTasks(const char* filename, Task* head) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return 0;
    }

    for (Task* p = head; p != NULL; p = p->next) {
        if (p->desc[0] != '\0') {
            fprintf(fp, "%d %d %d %s\n", p->year, p->month, p->day, p->desc);
        }
        else {
            fprintf(fp, "%d %d %d\n", p->year, p->month, p->day);
        }
    }

    fclose(fp);
    return 1;
}


// MENU / UI


void menu(Task** head) {
    int choice;
    do {
        printf("\n=== Simple Calendar ===\n");
        printf("1. Add task\n");
        printf("2. Show calendar for a month\n");
        printf("3. View tasks for a specific day\n");
        printf("0. Save and exit\n");
        printf("Choice: ");

        if (scanf_s("%d", &choice) != 1) {
            // bad input clear stdin
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            choice = -1;
            continue;
        }

        if (choice == 1) {
            int y, m, d;
            char buffer[DESC_LEN];

            printf("Enter year month day (e.g. 2025 11 29): ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid date input.\n");
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }

            // clear leftover newline
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}

            printf("Enter task description: ");
            if (!fgets(buffer, sizeof(buffer), stdin)) {
                printf("Error reading description.\n");
                continue;
            }
            // remove trailing newline from fgets
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }

            addTask(head, y, m, d, buffer);
            printf("Task added.\n");

        }
        else if (choice == 2) {
            int y, m;
            printf("Enter year and month (e.g. 2025 11): ");
            if (scanf_s("%d %d", &y, &m) != 2) {
                printf("Invalid input.\n");
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }
            printMonthCalendar(y, m, *head);

        }
        else if (choice == 3) {
            int y, m, d;
            printf("Enter year month day: ");
            if (scanf_s("%d %d %d", &y, &m, &d) != 3) {
                printf("Invalid input.\n");
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                continue;
            }
            printTasksForDay(*head, y, m, d);

        }
        else if (choice == 0) {
            printf("Saving and exiting...\n");
        }
        else {
            printf("Invalid choice.\n");
        }

    } while (choice != 0);
}
