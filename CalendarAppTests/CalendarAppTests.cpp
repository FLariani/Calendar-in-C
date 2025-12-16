#include "pch.h"
#include "CppUnitTest.h"

#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
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
    int updateTask(struct years* calendar_head, int year, int month, int day, int task_id, const char* new_desc);
    int deleteTask(struct years* calendar_head, int year, int month, int day, int task_id);

    // search helpers
    int containsIgnoreCase(const char* text, const char* key);
    void searchTasks(struct years* calendar_head, const char* keyword); // prints results

    // file I/O
    struct years* loadTasks(const char* filename);
    int saveTasks(const char* filename, struct years* calendar_head);

    // memory cleanup
    void freeCalendar(struct years* calendar_head);
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CalendarAppTests
{
    // Small helper to count tasks for a day (without relying on stdout printing)
    static int CountTasksForDay(struct years* cal, int y, int m, int d)
    {
        struct days* dayNode = getDayNode(cal, y, m, d);
        if (!dayNode || !dayNode->tasks_head) return 0;

        int count = 0;
        for (struct tasks* t = dayNode->tasks_head; t != NULL; t = t->next)
            count++;

        return count;
    }

    // Helper to get Nth task node in a day list (1-based by traversal order)
    static struct tasks* GetNthTaskNode(struct years* cal, int y, int m, int d, int n)
    {
        struct days* dayNode = getDayNode(cal, y, m, d);
        if (!dayNode) return NULL;

        int i = 1;
        for (struct tasks* t = dayNode->tasks_head; t != NULL; t = t->next, i++)
        {
            if (i == n) return t;
        }
        return NULL;
    }

    // Helper to count keyword matches across calendar by scanning nodes
    // (Instead of relying on searchTasks() printing)
    static int CountMatches(struct years* cal, const char* keyword)
    {
        if (!cal || !keyword || keyword[0] == '\0') return 0;

        int matches = 0;
        for (struct years* y = cal; y != NULL; y = y->next)
        {
            for (int mi = 0; mi < 12; mi++)
            {
                for (int di = 0; di < y->months[mi].num_days; di++)
                {
                    for (struct tasks* t = y->months[mi].days[di].tasks_head; t != NULL; t = t->next)
                    {
                        if (containsIgnoreCase(t->task_description, keyword))
                            matches++;
                    }
                }
            }
        }
        return matches;
    }

    TEST_CLASS(DateMathTests)
    {
    public:
        TEST_METHOD(IsLeap_KnownYears)
        {
            Assert::IsTrue(isLeap(2024) == 1);  // leap
            Assert::IsTrue(isLeap(1900) == 0);  // divisible by 100 but not 400
            Assert::IsTrue(isLeap(2000) == 1);  // divisible by 400
            Assert::IsTrue(isLeap(2025) == 0);  // normal
        }

        TEST_METHOD(DaysInMonth_Basics)
        {
            Assert::AreEqual(31, daysInMonth(2025, 1));
            Assert::AreEqual(30, daysInMonth(2025, 4));
            Assert::AreEqual(28, daysInMonth(2025, 2));
            Assert::AreEqual(29, daysInMonth(2024, 2)); // leap Feb
            Assert::AreEqual(0, daysInMonth(2025, 13)); // invalid month
        }

        TEST_METHOD(DayOfWeek_StableSanityChecks)
        {
            // This is a sanity check, not �calendar authority�.
            // Just making sure return range is always 0..6 and consistent.
            int w1 = dayOfWeek(2025, 12, 15);
            Assert::IsTrue(w1 >= 0 && w1 <= 6);

            int w2 = dayOfWeek(2025, 12, 16);
            Assert::IsTrue(w2 >= 0 && w2 <= 6);

            // next day should usually be (w1+1)%7 (it is with this algorithm)
            Assert::AreEqual((w1 + 1) % 7, w2);
        }
    };

    TEST_CLASS(CalendarStructureTests)
    {
    public:
        TEST_METHOD(FindOrAddYear_CreatesMonthsAndDays)
        {
            struct years* cal = NULL;

            struct years* y2025 = findOrAddYear(&cal, 2025);
            Assert::IsNotNull(y2025);
            Assert::AreEqual(2025, y2025->year_number);
            Assert::IsNotNull(y2025->months);

            // month array exists, and January has days
            Assert::AreEqual(1, y2025->months[0].month_number);
            Assert::IsNotNull(y2025->months[0].days);
            Assert::AreEqual(31, y2025->months[0].num_days);

            // calling again should return same node (not create a duplicate)
            struct years* again = findOrAddYear(&cal, 2025);
            Assert::IsTrue(again == y2025);

            freeCalendar(cal);
        }
    };

    TEST_CLASS(TaskAddTests)
    {
    public:
        TEST_METHOD(AddTask_FirstTaskGetsId1)
        {
            struct years* cal = NULL;

            addTask(&cal, 2025, 11, 29, "Finish assignment");
            struct days* dayNode = getDayNode(cal, 2025, 11, 29);
            Assert::IsNotNull(dayNode);
            Assert::IsNotNull(dayNode->tasks_head);

            Assert::AreEqual(1, dayNode->tasks_head->task_id);
            Assert::AreEqual(0, strcmp("Finish assignment", dayNode->tasks_head->task_description));

            freeCalendar(cal);
        }

        TEST_METHOD(AddTask_AppendsAndIdsIncrease)
        {
            struct years* cal = NULL;

            addTask(&cal, 2025, 11, 29, "Task A");
            addTask(&cal, 2025, 11, 29, "Task B");
            addTask(&cal, 2025, 11, 29, "Task C");

            Assert::AreEqual(3, CountTasksForDay(cal, 2025, 11, 29));

            // check ids in order (append behavior)
            struct tasks* t1 = GetNthTaskNode(cal, 2025, 11, 29, 1);
            struct tasks* t2 = GetNthTaskNode(cal, 2025, 11, 29, 2);
            struct tasks* t3 = GetNthTaskNode(cal, 2025, 11, 29, 3);

            Assert::IsNotNull(t1);
            Assert::IsNotNull(t2);
            Assert::IsNotNull(t3);

            Assert::AreEqual(1, t1->task_id);
            Assert::AreEqual(2, t2->task_id);
            Assert::AreEqual(3, t3->task_id);

            Assert::AreEqual(0, strcmp("Task A", t1->task_description));
            Assert::AreEqual(0, strcmp("Task B", t2->task_description));
            Assert::AreEqual(0, strcmp("Task C", t3->task_description));

            // prev pointers should be correct
            Assert::IsTrue(t2->prev == t1);
            Assert::IsTrue(t3->prev == t2);

            freeCalendar(cal);
        }

        TEST_METHOD(AddTask_InvalidDate_DoesNotCreateTasks)
        {
            struct years* cal = NULL;

            // invalid month: year may still get created, but it should NOT add any task anywhere
            addTask(&cal, 2025, 13, 10, "bad month");
            Assert::IsNotNull(cal); // year likely exists now

            // invalid day (Feb 30): should NOT insert
            addTask(&cal, 2025, 2, 30, "bad day");

            // confirm that a valid day still has zero tasks (we never added anything valid)
            Assert::AreEqual(0, CountTasksForDay(cal, 2025, 2, 28));
            Assert::AreEqual(0, CountTasksForDay(cal, 2025, 11, 1));

            freeCalendar(cal);
        }

       
    };

    TEST_CLASS(UpdateTaskTests)
    {

    private:
        // A pointer to the calendar head for use in all tests within this class.
        struct years* calendar = NULL;

    public:

        // TEST_METHOD_INITIALIZE runs before each test method.
        // New calender each time
        TEST_METHOD_INITIALIZE(Setup)
        {
            // Ensure calendar is clean before each test
            if (calendar != NULL) {
                freeCalendar(calendar);
                calendar = NULL;
            }
            // Add some initial tasks for a consistent starting point
            addTask(&calendar, 2025, 12, 25, "Initial Task 1");
            addTask(&calendar, 2025, 12, 25, "Initial Task 2");
        }

        // TEST_METHOD_CLEANUP runs after each test method.
        // This frees all memory allocated during a test to prevent leaks.
        TEST_METHOD_CLEANUP(Cleanup)
        {
            freeCalendar(calendar);
            calendar = NULL;
        }

        TEST_METHOD(SuccessfulUpdate)
        {
            int result = updateTask(calendar, 2025, 12, 25, 1, "Updated Task 1");

            // Function returns 0
            Assert::AreEqual(0, result);

            // Function updates task
            struct days* day_node = getDayNode(calendar, 2025, 12, 25);
            Assert::IsNotNull(day_node);
            Assert::AreEqual(0, strcmp(day_node->tasks_head->task_description, "Updated Task 1"));
        }

        TEST_METHOD(UpdateNonHeadTask)
        {
            int result = updateTask(calendar, 2025, 12, 25, 2, "Updated Task 2");

            Assert::AreEqual(0, result);

            struct days* day_node = getDayNode(calendar, 2025, 12, 25);
            Assert::IsNotNull(day_node);
            Assert::IsNotNull(day_node->tasks_head->next);
            Assert::AreEqual(0, strcmp(day_node->tasks_head->next->task_description, "Updated Task 2"));
        }

        TEST_METHOD(TaskIDNotFound)
        {
            int result = updateTask(calendar, 2025, 12, 25, 99, "This should fail");

            // function returns 1
            Assert::AreEqual(1, result);
        }

        TEST_METHOD(InvalidDate)
        {
            int result = updateTask(calendar, 2025, 13, 25, 1, "This should fail");
            Assert::AreEqual(1, result);
        }

        TEST_METHOD(YearNotFound)
        {
            // getDayNode should return NULL for a non-existent year, causing updateTask to fail
            int result = updateTask(calendar, 2026, 1, 1, 1, "This should fail");
            Assert::AreEqual(1, result);
        }
    };

    TEST_CLASS(TaskDeleteTests)
    {
    public:
        TEST_METHOD(DeleteTask_DeletesHead)
        {
            struct years* cal = NULL;

            addTask(&cal, 2025, 12, 25, "A");
            addTask(&cal, 2025, 12, 25, "B"); // id 2
            addTask(&cal, 2025, 12, 25, "C"); // id 3

            // delete id 1 (head)
            int ok = deleteTask(cal, 2025, 12, 25, 1);
            Assert::IsTrue(ok == 1);

            struct days* dayNode = getDayNode(cal, 2025, 12, 25);
            Assert::IsNotNull(dayNode);
            Assert::AreEqual(2, CountTasksForDay(cal, 2025, 12, 25));

            // new head should have prev == NULL
            Assert::IsNotNull(dayNode->tasks_head);
            Assert::IsNull(dayNode->tasks_head->prev);

            freeCalendar(cal);
        }

        TEST_METHOD(DeleteTask_DeletesMiddleFixesLinks)
        {
            struct years* cal = NULL;

            addTask(&cal, 2025, 11, 29, "A"); // id 1
            addTask(&cal, 2025, 11, 29, "B"); // id 2
            addTask(&cal, 2025, 11, 29, "C"); // id 3

            int ok = deleteTask(cal, 2025, 11, 29, 2);
            Assert::IsTrue(ok == 1);
            Assert::AreEqual(2, CountTasksForDay(cal, 2025, 11, 29));

            // list should now be A -> C
            struct tasks* first = GetNthTaskNode(cal, 2025, 11, 29, 1);
            struct tasks* second = GetNthTaskNode(cal, 2025, 11, 29, 2);

            Assert::AreEqual(0, strcmp("A", first->task_description));
            Assert::AreEqual(0, strcmp("C", second->task_description));

            // linkage sanity
            Assert::IsTrue(first->next == second);
            Assert::IsTrue(second->prev == first);

            freeCalendar(cal);
        }

        TEST_METHOD(DeleteTask_NotFound_Returns0)
        {
            struct years* cal = NULL;
            addTask(&cal, 2025, 11, 29, "A");

            int ok = deleteTask(cal, 2025, 11, 29, 99);
            Assert::IsTrue(ok == 0);

            freeCalendar(cal);
        }
    };

    TEST_CLASS(SearchHelperTests)
    {
    public:
        TEST_METHOD(ContainsIgnoreCase_BasicMatches)
        {
            Assert::IsTrue(containsIgnoreCase("Finish Assignment", "finish") == 1);
            Assert::IsTrue(containsIgnoreCase("Finish Assignment", "ASSIGN") == 1);
            Assert::IsTrue(containsIgnoreCase("Finish Assignment", "xyz") == 0);
            Assert::IsTrue(containsIgnoreCase("", "x") == 0);
            Assert::IsTrue(containsIgnoreCase("abc", "") == 1); // empty key matches
        }

        TEST_METHOD(Search_CountMatchesAcrossCalendar)
        {
            struct years* cal = NULL;

            addTask(&cal, 2025, 11, 29, "Buy groceries");
            addTask(&cal, 2025, 11, 30, "Buy milk");
            addTask(&cal, 2026, 1, 1, "New Year groceries");
            addTask(&cal, 2026, 1, 2, "Gym day");

            Assert::AreEqual(2, CountMatches(cal, "groceries")); 
            Assert::AreEqual(1, CountMatches(cal, "gym"));
            Assert::AreEqual(0, CountMatches(cal, "midterm"));

            freeCalendar(cal);
        }

       
    };

    TEST_CLASS(FileIOTests)
    {
    public:
        TEST_METHOD(SaveLoad_RoundTripPreservesTasks)
        {
            // write to a predictable test file name
            const char* fname = "tasks_test.txt";

            struct years* cal = NULL;

            addTask(&cal, 2025, 12, 25, "Christmas Day");
            addTask(&cal, 2025, 12, 25, "Dinner at 6");
            addTask(&cal, 2026, 1, 1, "New Year's Day");

            int saved = saveTasks(fname, cal);
            Assert::IsTrue(saved == 1);

            freeCalendar(cal);
            cal = NULL;

            // load into fresh structure
            cal = loadTasks(fname);
            Assert::IsNotNull(cal);

            Assert::AreEqual(2, CountTasksForDay(cal, 2025, 12, 25));
            Assert::AreEqual(1, CountTasksForDay(cal, 2026, 1, 1));

            // cleanup calendar
            freeCalendar(cal);

            // cleanup file (best effort)
            std::remove(fname);
        }
    };
}
