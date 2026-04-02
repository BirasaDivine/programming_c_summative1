#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAME     "students.dat"
#define INITIAL_CAP  4
#define MAX_SUBJECTS 6
#define MAX_COURSES  64

// ---- Data Model ----
// Each subject grade is on a 0.0–4.0 scale.
// GPA is the mean of all subject grades, stored for fast sorting.

typedef struct {
    int   id;
    char  name[50];
    int   age;
    char  course[50];        // degree / programme name
    int   numGrades;         // how many subjects (1..MAX_SUBJECTS)
    float grades[MAX_SUBJECTS];
    float gpa;               // always kept in sync via recalcGPA()
} Student;

// ---- Global dynamic array ----
Student *students = NULL;
int count    = 0;
int capacity = 0;

// ---- Forward declarations ----
void addStudent();
void viewStudents();
void searchMenu();
void updateStudent();
void deleteStudent();
void sortMenu();
void analysisMenu();
void saveLoadMenu();
void menu();

// ---- GPA helpers ----

void recalcGPA(Student *s) {
    if (s->numGrades == 0) { s->gpa = 0.0f; return; }
    float sum = 0;
    for (int i = 0; i < s->numGrades; i++) sum += s->grades[i];
    s->gpa = sum / s->numGrades;
}

// ---- File I/O ----

void loadStudents() {
    FILE *file = fopen(FILENAME, "rb");
    if (!file) return; // no file yet — start empty

    Student s;
    while (fread(&s, sizeof(Student), 1, file) == 1) {
        if (count == capacity) {
            capacity = (capacity == 0) ? INITIAL_CAP : capacity * 2;
            Student *tmp = realloc(students, capacity * sizeof(Student));
            if (!tmp) { fclose(file); printf("Realloc failed.\n"); return; }
            students = tmp;
        }
        students[count++] = s;
    }
    fclose(file);
}

void saveStudents() {
    FILE *file = fopen(FILENAME, "wb");
    if (!file) { perror("Cannot open file"); return; }
    fwrite(students, sizeof(Student), count, file);
    fclose(file);
}

// ---- Input helpers ----

// Read a non-empty trimmed string into buf (max len).
// Returns 1 on success, 0 if empty after trimming.
int readString(const char *prompt, char *buf, int len) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, len, stdin)) return 0;
    buf[strcspn(buf, "\n")] = 0;
    // ltrim
    char *p = buf;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == 0) return 0;
    if (p != buf) memmove(buf, p, strlen(p) + 1);
    // rtrim
    int l = (int)strlen(buf) - 1;
    while (l >= 0 && (buf[l] == ' ' || buf[l] == '\t')) buf[l--] = 0;
    return buf[0] != 0;
}

// Read an integer >= minVal.  Returns 1 on success.
int readInt(const char *prompt, int *out, int minVal) {
    char buf[64];
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        char *end;
        long v = strtol(buf, &end, 10);
        if (end != buf && (*end == '\n' || *end == 0) && v >= minVal) {
            *out = (int)v;
            return 1;
        }
        printf("  Invalid — enter a whole number >= %d.\n", minVal);
    }
}

// Read a float in [lo, hi].  Returns 1 on success.
int readGrade(const char *prompt, float *out, float lo, float hi) {
    char buf[64];
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        char *end;
        double v = strtod(buf, &end);
        if (end != buf && (*end == '\n' || *end == 0) && v >= lo && v <= hi) {
            *out = (float)v;
            return 1;
        }
        printf("  Invalid — enter a value between %.1f and %.1f.\n", lo, hi);
    }
}

// ---- Duplicate check ----

int isDuplicateID(int id) {
    for (int i = 0; i < count; i++)
        if (students[i].id == id) return 1;
    return 0;
}

// ---- Grow array by one slot ----

int ensureCapacity() {
    if (count == capacity) {
        int newcap = (capacity == 0) ? INITIAL_CAP : capacity * 2;
        Student *tmp = realloc(students, newcap * sizeof(Student));
        if (!tmp) { printf("Memory reallocation failed.\n"); return 0; }
        students = tmp;
        capacity = newcap;
    }
    return 1;
}

// ============================================================
//  CRUD
// ============================================================

void addStudent() {
    printf("\n=======================================================\n");
    printf("                   Add Student\n");
    printf("=======================================================\n");

    Student s;
    memset(&s, 0, sizeof(Student));

    // ID
    int id;
    if (!readInt("Student ID: ", &id, 1)) return;
    if (isDuplicateID(id)) {
        printf("Error: ID %d already exists.\n", id);
        return;
    }
    s.id = id;

    // Name
    if (!readString("Name: ", s.name, sizeof(s.name))) {
        printf("Error: Name cannot be empty.\n"); return;
    }

    // Age
    if (!readInt("Age: ", &s.age, 1)) return;
    if (s.age > 120) { printf("Error: Invalid age.\n"); return; }

    // Course / programme
    if (!readString("Course/Programme: ", s.course, sizeof(s.course))) {
        printf("Error: Course cannot be empty.\n"); return;
    }

    // Number of subjects
    int n;
    char prompt[64];
    snprintf(prompt, sizeof(prompt), "Number of subjects (1-%d): ", MAX_SUBJECTS);
    if (!readInt(prompt, &n, 1)) return;
    if (n > MAX_SUBJECTS) { printf("Error: Maximum %d subjects.\n", MAX_SUBJECTS); return; }
    s.numGrades = n;

    // Subject grades
    for (int i = 0; i < n; i++) {
        snprintf(prompt, sizeof(prompt), "  Grade for Subject %d (0.0-4.0): ", i + 1);
        if (!readGrade(prompt, &s.grades[i], 0.0f, 4.0f)) return;
    }

    recalcGPA(&s);

    if (!ensureCapacity()) return;
    students[count++] = s;
    saveStudents();
    printf("Student added. GPA: %.2f\n", s.gpa);
}

void viewStudents() {
    printf("\n=======================================================\n");
    printf("                  All Students\n");
    printf("=======================================================\n");

    if (count == 0) { printf("No students on record.\n"); return; }

    printf("%-5s %-20s %-4s %-20s %-7s %-6s\n",
           "ID", "Name", "Age", "Course", "Subj.", "GPA");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("%-5d %-20s %-4d %-20s %-7d %-6.2f\n",
               students[i].id, students[i].name, students[i].age,
               students[i].course, students[i].numGrades, students[i].gpa);
    }
}

// Print full detail for one student (including grade breakdown)
static void printStudentDetail(const Student *s) {
    printf("  ID     : %d\n", s->id);
    printf("  Name   : %s\n", s->name);
    printf("  Age    : %d\n", s->age);
    printf("  Course : %s\n", s->course);
    printf("  Grades : ");
    for (int i = 0; i < s->numGrades; i++) {
        printf("%.2f", s->grades[i]);
        if (i < s->numGrades - 1) printf(", ");
    }
    printf("\n  GPA    : %.2f\n", s->gpa);
}

// ---- Search ----

void searchByID() {
    int id;
    if (!readInt("Search by ID: ", &id, 1)) return;
    for (int i = 0; i < count; i++) {
        if (students[i].id == id) {
            printf("\nFound:\n");
            printStudentDetail(&students[i]);
            return;
        }
    }
    printf("No student with ID %d.\n", id);
}

void searchByName() {
    char query[50];
    if (!readString("Search by name (partial match): ", query, sizeof(query))) return;

    int found = 0;
    for (int i = 0; i < count; i++) {
        // Case-sensitive partial match
        if (strstr(students[i].name, query)) {
            if (!found) printf("\nMatches:\n");
            printf("---\n");
            printStudentDetail(&students[i]);
            found++;
        }
    }
    if (!found) printf("No students matching \"%s\".\n", query);
    else printf("(%d match(es))\n", found);
}

void searchMenu() {
    printf("\n=======================================================\n");
    printf("                  Search Student\n");
    printf("=======================================================\n");
    printf("1. Search by ID\n");
    printf("2. Search by Name\n");
    int choice;
    if (!readInt("Choice: ", &choice, 1)) return;
    switch (choice) {
    case 1: searchByID();   break;
    case 2: searchByName(); break;
    default: printf("Invalid choice.\n");
    }
}

void updateStudent() {
    printf("\n=======================================================\n");
    printf("                  Update Student\n");
    printf("=======================================================\n");

    int id;
    if (!readInt("ID of student to update: ", &id, 1)) return;

    for (int i = 0; i < count; i++) {
        if (students[i].id == id) {
            Student *s = &students[i];
            printf("Updating: %s (leave field blank to keep current value)\n", s->name);

            char tmp[50];
            snprintf(tmp, sizeof(tmp), "%s", s->name);
            if (readString("New name [enter to keep]: ", tmp, sizeof(tmp)))
                strcpy(s->name, tmp);

            printf("New age [%d, enter to keep]: ", s->age);
            fflush(stdout);
            char buf[32];
            fgets(buf, sizeof(buf), stdin);
            if (buf[0] != '\n') {
                char *end; long v = strtol(buf, &end, 10);
                if (v >= 1 && v <= 120) s->age = (int)v;
                else printf("  Invalid age — kept %d.\n", s->age);
            }

            snprintf(tmp, sizeof(tmp), "%s", s->course);
            if (readString("New course [enter to keep]: ", tmp, sizeof(tmp)))
                strcpy(s->course, tmp);

            // Re-enter grades
            printf("Re-enter grades? (y/n): "); fflush(stdout);
            char c; fgets(buf, sizeof(buf), stdin); c = buf[0];
            if (c == 'y' || c == 'Y') {
                int n;
                char pr[64];
                snprintf(pr, sizeof(pr), "Number of subjects (1-%d): ", MAX_SUBJECTS);
                if (readInt(pr, &n, 1) && n <= MAX_SUBJECTS) {
                    s->numGrades = n;
                    int ok = 1;
                    for (int j = 0; j < n && ok; j++) {
                        snprintf(pr, sizeof(pr), "  Grade %d (0.0-4.0): ", j + 1);
                        ok = readGrade(pr, &s->grades[j], 0.0f, 4.0f);
                    }
                    if (!ok) { printf("Update aborted.\n"); return; }
                }
            }

            recalcGPA(s);
            saveStudents();
            printf("Student updated. New GPA: %.2f\n", s->gpa);
            return;
        }
    }
    printf("No student with ID %d.\n", id);
}

void deleteStudent() {
    printf("\n=======================================================\n");
    printf("                  Delete Student\n");
    printf("=======================================================\n");

    int id;
    if (!readInt("ID to delete: ", &id, 1)) return;

    for (int i = 0; i < count; i++) {
        if (students[i].id == id) {
            printf("Delete %s? (y/n): ", students[i].name); fflush(stdout);
            char buf[8]; fgets(buf, sizeof(buf), stdin);
            if (buf[0] != 'y' && buf[0] != 'Y') { printf("Cancelled.\n"); return; }
            for (int j = i; j < count - 1; j++)
                students[j] = students[j + 1];
            count--;
            saveStudents();
            printf("Student deleted.\n");
            return;
        }
    }
    printf("No student with ID %d.\n", id);
}

// ============================================================
//  SORTING  (bubble sort — manual, no qsort)
// ============================================================

void sortByGPA() {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (students[j].gpa < students[j + 1].gpa) {
                Student t = students[j]; students[j] = students[j+1]; students[j+1] = t;
            }
    printf("Sorted by GPA (highest first).\n");
    viewStudents();
}

void sortByName() {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (strcmp(students[j].name, students[j+1].name) > 0) {
                Student t = students[j]; students[j] = students[j+1]; students[j+1] = t;
            }
    printf("Sorted by Name (A-Z).\n");
    viewStudents();
}

void sortByID() {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (students[j].id > students[j+1].id) {
                Student t = students[j]; students[j] = students[j+1]; students[j+1] = t;
            }
    printf("Sorted by ID (ascending).\n");
    viewStudents();
}

void sortMenu() {
    printf("\n=======================================================\n");
    printf("                  Sort Students\n");
    printf("=======================================================\n");
    printf("1. Sort by GPA  (highest first)\n");
    printf("2. Sort by Name (A-Z)\n");
    printf("3. Sort by ID   (ascending)\n");
    int c; if (!readInt("Choice: ", &c, 1)) return;
    switch (c) {
    case 1: sortByGPA();  break;
    case 2: sortByName(); break;
    case 3: sortByID();   break;
    default: printf("Invalid choice.\n");
    }
}

// ============================================================
//  ANALYSIS
// ============================================================

static void collectUniqueCourses(char courses[][50], int *nc) {
    *nc = 0;
    for (int i = 0; i < count; i++) {
        int found = 0;
        for (int j = 0; j < *nc; j++)
            if (strcmp(students[i].course, courses[j]) == 0) { found = 1; break; }
        if (!found && *nc < MAX_COURSES)
            strcpy(courses[(*nc)++], students[i].course);
    }
}

void showAverage() {
    if (count == 0) { printf("No students.\n"); return; }
    float sum = 0;
    for (int i = 0; i < count; i++) sum += students[i].gpa;
    printf("Class Average GPA: %.2f\n", sum / count);
}

void showHighLow() {
    if (count == 0) { printf("No students.\n"); return; }
    int hi = 0, lo = 0;
    for (int i = 1; i < count; i++) {
        if (students[i].gpa > students[hi].gpa) hi = i;
        if (students[i].gpa < students[lo].gpa) lo = i;
    }
    printf("Highest GPA: %-20s (%.2f)\n", students[hi].name, students[hi].gpa);
    printf("Lowest  GPA: %-20s (%.2f)\n", students[lo].name, students[lo].gpa);
}

void showMedian() {
    if (count == 0) { printf("No students.\n"); return; }
    float *g = malloc(count * sizeof(float));
    if (!g) { printf("Allocation failed.\n"); return; }
    for (int i = 0; i < count; i++) g[i] = students[i].gpa;
    // bubble sort copy
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (g[j] > g[j+1]) { float t = g[j]; g[j] = g[j+1]; g[j+1] = t; }
    float median = (count % 2 == 0)
        ? (g[count/2 - 1] + g[count/2]) / 2.0f
        : g[count/2];
    printf("Median GPA: %.2f\n", median);
    free(g);
}

void showTopN() {
    if (count == 0) { printf("No students.\n"); return; }
    int n; if (!readInt("Top-N  (N = ): ", &n, 1)) return;
    if (n > count) { printf("Only %d student(s) available — showing all.\n", count); n = count; }
    Student *copy = malloc(count * sizeof(Student));
    if (!copy) { printf("Allocation failed.\n"); return; }
    memcpy(copy, students, count * sizeof(Student));
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (copy[j].gpa < copy[j+1].gpa) {
                Student t = copy[j]; copy[j] = copy[j+1]; copy[j+1] = t;
            }
    printf("\nTop %d Student(s):\n", n);
    printf("%-5s %-20s %-20s %-6s\n", "Rank", "Name", "Course", "GPA");
    printf("------------------------------------------------------\n");
    for (int i = 0; i < n; i++)
        printf("%-5d %-20s %-20s %-6.2f\n",
               i + 1, copy[i].name, copy[i].course, copy[i].gpa);
    free(copy);
}

void bestPerCourse() {
    if (count == 0) { printf("No students.\n"); return; }
    char courses[MAX_COURSES][50];
    int  nc = 0;
    collectUniqueCourses(courses, &nc);

    printf("\nBest Student Per Course:\n");
    printf("%-25s %-20s %-6s\n", "Course", "Best Student", "GPA");
    printf("-------------------------------------------------------\n");
    for (int c = 0; c < nc; c++) {
        int best = -1;
        for (int i = 0; i < count; i++) {
            if (strcmp(students[i].course, courses[c]) == 0) {
                if (best == -1 || students[i].gpa > students[best].gpa)
                    best = i;
            }
        }
        if (best != -1)
            printf("%-25s %-20s %-6.2f\n",
                   courses[c], students[best].name, students[best].gpa);
    }
}

void courseAverage() {
    if (count == 0) { printf("No students.\n"); return; }
    char courses[MAX_COURSES][50];
    int  nc = 0;
    collectUniqueCourses(courses, &nc);

    printf("\nCourse-Based Average GPA:\n");
    printf("%-25s %-10s %-6s\n", "Course", "Students", "Avg GPA");
    printf("-----------------------------------------------\n");
    for (int c = 0; c < nc; c++) {
        float sum = 0; int cnt = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(students[i].course, courses[c]) == 0) {
                sum += students[i].gpa; cnt++;
            }
        }
        if (cnt > 0)
            printf("%-25s %-10d %-6.2f\n", courses[c], cnt, sum / cnt);
    }
}

void analysisMenu() {
    printf("\n=======================================================\n");
    printf("                Analysis & Reports\n");
    printf("=======================================================\n");
    printf("1. Class Average GPA\n");
    printf("2. Highest and Lowest GPA\n");
    printf("3. Median GPA\n");
    printf("4. Top-N Students\n");
    printf("5. Best Student Per Course\n");
    printf("6. Course-Based Average GPA\n");
    int c; if (!readInt("Choice: ", &c, 1)) return;
    switch (c) {
    case 1: showAverage();   break;
    case 2: showHighLow();   break;
    case 3: showMedian();    break;
    case 4: showTopN();      break;
    case 5: bestPerCourse(); break;
    case 6: courseAverage(); break;
    default: printf("Invalid choice.\n");
    }
}

// ============================================================
//  Save / Load explicit menu
// ============================================================

void saveLoadMenu() {
    printf("\n=======================================================\n");
    printf("                  Save / Load\n");
    printf("=======================================================\n");
    printf("1. Save records to file now\n");
    printf("2. Reload records from file\n");
    int c; if (!readInt("Choice: ", &c, 1)) return;
    switch (c) {
    case 1:
        saveStudents();
        printf("Saved %d record(s) to %s.\n", count, FILENAME);
        break;
    case 2: {
        // Discard in-memory array and reload
        count = 0;
        loadStudents();
        printf("Loaded %d record(s) from %s.\n", count, FILENAME);
        break;
    }
    default:
        printf("Invalid choice.\n");
    }
}

// ============================================================
//  Main menu
// ============================================================

void menu() {
    int choice;
    do {
        printf("\n=======================================================\n");
        printf("         Course Performance & Records Analyzer\n");
        printf("=======================================================\n");
        printf("Students loaded: %d\n", count);
        printf("-------------------------------------------------------\n");
        printf("1. Add Student\n");
        printf("2. View All Students\n");
        printf("3. Search Student\n");
        printf("4. Update Student\n");
        printf("5. Delete Student\n");
        printf("6. Sort Students\n");
        printf("7. Analysis & Reports\n");
        printf("8. Save / Load Records\n");
        printf("9. Exit\n");
        printf("-------------------------------------------------------\n");
        if (!readInt("Choice: ", &choice, 1)) break;
        switch (choice) {
        case 1: addStudent();   break;
        case 2: viewStudents(); break;
        case 3: searchMenu();   break;
        case 4: updateStudent(); break;
        case 5: deleteStudent(); break;
        case 6: sortMenu();      break;
        case 7: analysisMenu();  break;
        case 8: saveLoadMenu();  break;
        case 9: printf("Goodbye.\n"); break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 9);
}

int main() {
    capacity = INITIAL_CAP;
    students = malloc(capacity * sizeof(Student));
    if (!students) { printf("malloc failed.\n"); return 1; }

    loadStudents();
    printf("Student Management System — %d record(s) loaded.\n", count);
    menu();

    free(students);
    return 0;
}
