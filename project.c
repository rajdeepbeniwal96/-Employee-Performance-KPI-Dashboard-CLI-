
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MAXE 200
#define MAXL 5000
#define FILE_NAME "kpi_data.txt"

typedef struct {
    int id, employee, done, errors;
    char date[11];
    double hours, quality;
} Log;

typedef struct {
    int id;
    char name[40], dept[30];
} Employee;

typedef struct {
    int total, done, errors;
    double hours, doneHours, quality;
    double productivity, reliability, errorRate;
    double avgTime, score;
} KPI;

Employee e[MAXE];
Log logs[MAXL];
int ne = 0, nl = 0;

int input(const char *prompt, char *s, int size) {
    printf("%s", prompt);
    if (!fgets(s, size, stdin)) return 0;

    size_t n = strlen(s);
    if (n && s[n - 1] == '\n')
        s[n - 1] = 0;
    else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    return 1;
}

int integer(const char *p, int *v) {
    char s[80], x;
    return input(p, s, sizeof(s)) &&
           sscanf(s, " %d %c", v, &x) == 1;
}

int number(const char *p, double *v) {
    char s[80], x;
    return input(p, s, sizeof(s)) &&
           sscanf(s, " %lf %c", v, &x) == 1 &&
           isfinite(*v);
}

int dateOK(const char *s) {
    int y, m, d;
    int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-')
        return 0;

    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 &&
            !isdigit((unsigned char)s[i]))
            return 0;

    if (sscanf(s, "%4d-%2d-%2d", &y, &m, &d) != 3 ||
        y < 1900 || m < 1 || m > 12)
        return 0;

    if (m == 2 &&
        ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        days[2] = 29;

    return d >= 1 && d <= days[m];
}

int find(int id) {
    for (int i = 0; i < ne; i++)
        if (e[i].id == id) return i;
    return -1;
}

/* File handling */

void save(void) {
    FILE *f = fopen(FILE_NAME, "w");
    if (!f) {
        puts("Warning: data could not be saved!");
        return;
    }

    fprintf(f, "%d %d\n", ne, nl);

    for (int i = 0; i < ne; i++)
        fprintf(f, "%d|%s|%s\n",
                e[i].id, e[i].name, e[i].dept);

    for (int i = 0; i < nl; i++)
        fprintf(f, "%d %d %s %.10g %.10g %d %d\n",
                logs[i].id, logs[i].employee,
                logs[i].date, logs[i].hours,
                logs[i].quality, logs[i].errors,
                logs[i].done);

    fclose(f);
}

void load(void) {
    FILE *f = fopen(FILE_NAME, "r");
    if (!f) return;

    int a, b;
    if (fscanf(f, "%d %d\n", &a, &b) != 2 ||
        a < 0 || a > MAXE || b < 0 || b > MAXL) {
        fclose(f);
        return;
    }

    char s[160];

    for (int i = 0; i < a; i++) {
        if (!fgets(s, sizeof(s), f)) break;

        Employee t;
        char extra;

        if (sscanf(s, "%d|%39[^|]|%29[^\n]%c",
                   &t.id, t.name, t.dept, &extra) != 3)
            break;

        if (t.id <= 0 || strchr(t.name, '|') ||
            strchr(t.dept, '|') || find(t.id) >= 0)
            break;

        e[ne++] = t;
    }

    if (ne == a)
        for (int i = 0; i < b; i++) {
            Log t;

            if (fscanf(f, "%d %d %10s %lf %lf %d %d",
                       &t.id, &t.employee, t.date,
                       &t.hours, &t.quality,
                       &t.errors, &t.done) != 7)
                break;

            if (t.id <= 0 || find(t.employee) < 0 ||
                !dateOK(t.date) ||
                !isfinite(t.hours) || t.hours <= 0 ||
                !isfinite(t.quality) ||
                t.quality < 0 || t.quality > 100 ||
                t.errors < 0 ||
                (t.done != 0 && t.done != 1))
                break;

            int duplicate = 0;
            for (int j = 0; j < nl; j++)
                if (logs[j].employee == t.employee &&
                    logs[j].id == t.id)
                    duplicate = 1;

            if (duplicate) break;
            logs[nl++] = t;
        }

    fclose(f);
}

/* KPI calculations */

KPI calc(int id, const char *from, const char *to) {
    KPI k = {0};

    for (int i = 0; i < nl; i++) {
        Log *l = &logs[i];

        if (l->employee != id ||
            (from && strcmp(l->date, from) < 0) ||
            (to && strcmp(l->date, to) > 0))
            continue;

        k.total++;
        k.done += l->done;
        k.hours += l->hours;
        k.quality += l->quality;

        if (l->done) k.doneHours += l->hours;
        if (l->errors > 0) k.errors++;
    }

    if (!k.total) return k;

    k.quality /= k.total;
    k.productivity = k.done / k.hours;
    k.reliability = 100.0 * k.done / k.total;
    k.errorRate = 100.0 * k.errors / k.total;

    if (k.done)
        k.avgTime = k.doneHours / k.done;

    double p = fmin(100.0, k.productivity * 10.0);

    k.score = 0.3 * p +
              0.3 * k.reliability +
              0.3 * k.quality +
              0.1 * (100 - k.errorRate);

    return k;
}

void show(int i, const char *from, const char *to) {
    KPI k = calc(e[i].id, from, to);

    printf("\n%d | %s | %s\n",
           e[i].id, e[i].name, e[i].dept);

    if (!k.total) {
        puts("No records available.");
        return;
    }

    printf("Tasks: %d | Completed: %d\n",
           k.total, k.done);

    printf("Productivity: %.2f tasks/hour\n",
           k.productivity);

    printf("Reliability: %.2f%% | Error Rate: %.2f%%\n",
           k.reliability, k.errorRate);

    printf("Quality: %.2f | Avg Time: %.2f hours\n",
           k.quality, k.avgTime);

    printf("KPI: %.2f/100 (%s)\n", k.score,
           k.score < 50 ? "BELOW THRESHOLD" :
                          "SATISFACTORY");
}

/* Employee management */

void addEmployee(void) {
    if (ne == MAXE) {
        puts("Employee limit reached.");
        return;
    }

    Employee t;

    if (!integer("Employee ID: ", &t.id) ||
        t.id <= 0 || find(t.id) >= 0) {
        puts("Invalid/duplicate ID.");
        return;
    }

    if (!input("Name: ", t.name, sizeof(t.name)) ||
        !t.name[0] || strchr(t.name, '|') ||
        !input("Department: ", t.dept, sizeof(t.dept)) ||
        !t.dept[0] || strchr(t.dept, '|')) {
        puts("Invalid name/department.");
        return;
    }

    e[ne++] = t;
    save();
    puts("Employee added.");
}

void addLog(void) {
    if (nl == MAXL) {
        puts("Log limit reached.");
        return;
    }

    Log t;

    if (!integer("Employee ID: ", &t.employee) ||
        find(t.employee) < 0) {
        puts("Employee not found.");
        return;
    }

    if (!integer("Task ID: ", &t.id) || t.id <= 0) {
        puts("Invalid task ID.");
        return;
    }

    for (int i = 0; i < nl; i++)
        if (logs[i].employee == t.employee &&
            logs[i].id == t.id) {
            puts("Duplicate task ID.");
            return;
        }

    if (!input("Date (YYYY-MM-DD): ",
               t.date, sizeof(t.date)) ||
        !dateOK(t.date) ||
        !number("Hours: ", &t.hours) ||
        !number("Quality (0-100): ", &t.quality) ||
        !integer("Errors: ", &t.errors) ||
        !integer("Completed (1/0): ", &t.done) ||
        t.hours <= 0 ||
        t.quality < 0 || t.quality > 100 ||
        t.errors < 0 ||
        (t.done != 0 && t.done != 1)) {
        puts("Invalid task data.");
        return;
    }

    logs[nl++] = t;
    save();
    puts("Task log added.");
}

/* Ranking */

typedef struct {
    int index;
    double score;
} Rank;

int rankCmp(const void *a, const void *b) {
    const Rank *x = a, *y = b;

    if (x->score < y->score) return 1;
    if (x->score > y->score) return -1;

    return (e[x->index].id > e[y->index].id) -
           (e[x->index].id < e[y->index].id);
}

int doubleCmp(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

void ranking(void) {
    Rank r[MAXE];
    int n = 0;

    for (int i = 0; i < ne; i++) {
        KPI k = calc(e[i].id, NULL, NULL);

        if (k.total)
            r[n++] = (Rank){i, k.score};
    }

    if (!n) {
        puts("No performance records.");
        return;
    }

    qsort(r, n, sizeof(r[0]), rankCmp);

    for (int i = 0; i < n; i++)
        printf("%d. %s (ID %d): %.2f\n",
               i + 1, e[r[i].index].name,
               e[r[i].index].id, r[i].score);
}

/* Summary */

void summary(void) {
    double a[MAXE], sum = 0;
    int n = 0, low = 0;

    for (int i = 0; i < ne; i++) {
        KPI k = calc(e[i].id, NULL, NULL);
        if (!k.total) continue;

        a[n++] = k.score;
        sum += k.score;

        if (k.score < 50) low++;
    }

    if (!n) {
        puts("No performance records.");
        return;
    }

    qsort(a, n, sizeof(a[0]), doubleCmp);

    double median = n % 2 ? a[n / 2] :
                    (a[n / 2 - 1] + a[n / 2]) / 2;

    printf("Employees: %d | With logs: %d\n", ne, n);
    printf("Average: %.2f | Median: %.2f\n",
           sum / n, median);
    printf("Highest: %.2f | Lowest: %.2f\n",
           a[n - 1], a[0]);
    printf("Below threshold: %d\n", low);
}

/* Performance trend */

int logCmp(const void *a, const void *b) {
    const Log *x = a, *y = b;
    int c = strcmp(x->date, y->date);

    return c ? c : (x->id > y->id) - (x->id < y->id);
}

void trend(void) {
    int id, n;

    if (!integer("Employee ID: ", &id) ||
        find(id) < 0 ||
        !integer("Records per group: ", &n) ||
        n <= 0) {
        puts("Invalid input.");
        return;
    }

    Log a[MAXL];
    int total = 0;

    for (int i = 0; i < nl; i++)
        if (logs[i].employee == id)
            a[total++] = logs[i];

    if ((long long)n * 2 > total) {
        puts("Not enough records.");
        return;
    }

    qsort(a, total, sizeof(a[0]), logCmp);

    double old = 0, recent = 0;

    for (int i = total - 2 * n; i < total - n; i++)
        old += a[i].quality;

    for (int i = total - n; i < total; i++)
        recent += a[i].quality;

    old /= n;
    recent /= n;

    printf("Quality Trend:\n");
    printf("Previous: %.2f | Recent: %.2f\n", old, recent);

    printf("Status: %s\n",
           recent > old ? "IMPROVING" :
           recent < old ? "WORSENING" : "STABLE");
}

/* View task logs */

void viewLogs(void) {
    int id, n = 0;

    if (!integer("Employee ID: ", &id) ||
        find(id) < 0) {
        puts("Employee not found.");
        return;
    }

    for (int i = 0; i < nl; i++) {
        if (logs[i].employee != id) continue;

        Log *l = &logs[i];

        printf("Task %d | %s | %.2f hours | "
               "Quality %.2f | Errors %d | Done %d\n",
               l->id, l->date, l->hours,
               l->quality, l->errors, l->done);
        n++;
    }

    if (!n) puts("No logs available.");
}

/* Main menu */

int main(void) {
    load();

    for (;;) {
        int ch, id;
        char dept[30], from[11], to[11];

        puts("\n=== EMPLOYEE KPI DASHBOARD ===");
        puts("1. Add Employee");
        puts("2. Add Task Log");
        puts("3. View All KPIs");
        puts("4. Search Employee");
        puts("5. Rank Employees");
        puts("6. Filter Department");
        puts("7. Filter Date Range");
        puts("8. Summary Report");
        puts("9. Performance Trend");
        puts("10. View Task Logs");
        puts("0. Exit");

        if (!integer("Choice: ", &ch)) {
            puts("Invalid choice.");
            continue;
        }

        switch (ch) {
            case 1:
                addEmployee();
                break;

            case 2:
                addLog();
                break;

            case 3:
                if (!ne) puts("No employees.");
                for (int i = 0; i < ne; i++)
                    show(i, NULL, NULL);
                break;

            case 4:
                if (integer("Employee ID: ", &id) &&
                    find(id) >= 0)
                    show(find(id), NULL, NULL);
                else
                    puts("Not found.");
                break;

            case 5:
                ranking();
                break;

            case 6: {
                if (!input("Department: ",
                           dept, sizeof(dept)))
                    break;

                int n = 0;

                for (int i = 0; i < ne; i++)
                    if (strcmp(e[i].dept, dept) == 0) {
                        show(i, NULL, NULL);
                        n++;
                    }

                if (!n) puts("No matching employees.");
                break;
            }

            case 7: {
                if (!input("Start date: ",
                           from, sizeof(from)) ||
                    !input("End date: ",
                           to, sizeof(to)) ||
                    !dateOK(from) || !dateOK(to) ||
                    strcmp(from, to) > 0) {
                    puts("Invalid date range.");
                    break;
                }

                int n = 0;

                for (int i = 0; i < ne; i++)
                    if (calc(e[i].id, from, to).total) {
                        show(i, from, to);
                        n++;
                    }

                if (!n) puts("No matching logs.");
                break;
            }

            case 8:
                summary();
                break;

            case 9:
                trend();
                break;

            case 10:
                viewLogs();
                break;

            case 0:
                save();
                puts("Goodbye!");
                return 0;

            default:
                puts("Invalid choice.");
        }
    }
}
