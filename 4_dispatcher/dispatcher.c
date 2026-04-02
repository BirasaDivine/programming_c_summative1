#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "numbers.dat"

// --- Global numerical dataset ---
double *data = NULL;
int dataCount = 0;
int dataCapacity = 0;

// --- Load / save helpers ---

void loadData() {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) {
        printf("Data file not found. Starting with empty dataset.\n");
        return;
    }
    double val;
    while (fscanf(file, "%lf", &val) == 1) {
        if (dataCount == dataCapacity) {
            dataCapacity = (dataCapacity == 0) ? 8 : dataCapacity * 2;
            double *tmp = realloc(data, dataCapacity * sizeof(double));
            if (!tmp) { fclose(file); printf("Realloc failed.\n"); return; }
            data = tmp;
        }
        data[dataCount++] = val;
    }
    fclose(file);
    printf("Loaded %d number(s) from %s.\n", dataCount, DATA_FILE);
}

void saveData() {
    FILE *file = fopen(DATA_FILE, "w");
    if (!file) { perror("Unable to open file"); return; }
    for (int i = 0; i < dataCount; i++)
        fprintf(file, "%.6g\n", data[i]);
    fclose(file);
}

void printData() {
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }
    printf("Dataset (%d values): ", dataCount);
    for (int i = 0; i < dataCount; i++) {
        printf("%.4g", data[i]);
        if (i < dataCount - 1) printf(", ");
    }
    printf("\n");
}

// --- Core operations (stored in dispatcher array) ---

void op_sum(void) {
    printf("\n=======================================================\n");
    printf("                        Sum\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }
    double sum = 0;
    for (int i = 0; i < dataCount; i++) sum += data[i];
    printf("Sum: %.6g\n", sum);
}

void op_average(void) {
    printf("\n=======================================================\n");
    printf("                      Average\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }
    double sum = 0;
    for (int i = 0; i < dataCount; i++) sum += data[i];
    printf("Average: %.6g\n", sum / dataCount);
}

void op_min(void) {
    printf("\n=======================================================\n");
    printf("                      Minimum\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }
    double min = data[0];
    for (int i = 1; i < dataCount; i++)
        if (data[i] < min) min = data[i];
    printf("Minimum: %.6g\n", min);
}

void op_max(void) {
    printf("\n=======================================================\n");
    printf("                      Maximum\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }
    double max = data[0];
    for (int i = 1; i < dataCount; i++)
        if (data[i] > max) max = data[i];
    printf("Maximum: %.6g\n", max);
}

// --- Filter: callback receives each value and a threshold, returns 1 to keep ---
typedef int (*FilterCallback)(double value, double threshold);

int filter_above(double value, double threshold) { return value > threshold; }
int filter_below(double value, double threshold) { return value < threshold; }

void op_filter(void) {
    printf("\n=======================================================\n");
    printf("                Filter (callback)\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }

    int mode;
    double threshold;
    printf("Filter mode: 1=above threshold  2=below threshold\n");
    printf("Enter mode: "); fflush(stdout);
    scanf("%d", &mode);
    printf("Enter threshold: "); fflush(stdout);
    scanf("%lf", &threshold);
    getchar();

    FilterCallback cb = (mode == 2) ? filter_below : filter_above;
    const char *desc = (mode == 2) ? "below" : "above";

    printf("Values %s %.4g: ", desc, threshold);
    int found = 0;
    for (int i = 0; i < dataCount; i++) {
        if (cb(data[i], threshold)) {
            printf("%.4g ", data[i]);
            found++;
        }
    }
    if (!found) printf("(none)");
    printf("\n%d value(s) match.\n", found);
}

// --- Transform: callback receives a value and a factor, returns the new value ---
typedef double (*TransformCallback)(double value, double factor);

double transform_scale(double value, double factor) { return value * factor; }
double transform_offset(double value, double factor) { return value + factor; }

void op_transform(void) {
    printf("\n=======================================================\n");
    printf("               Transform (callback)\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }

    int mode;
    double factor;
    printf("Transform mode: 1=multiply by factor  2=add offset\n");
    printf("Enter mode: "); fflush(stdout);
    scanf("%d", &mode);
    printf("Enter factor: "); fflush(stdout);
    scanf("%lf", &factor);
    getchar();

    TransformCallback cb = (mode == 2) ? transform_offset : transform_scale;
    const char *desc = (mode == 2) ? "Adding" : "Multiplying by";

    printf("%s %.4g...\n", desc, factor);
    for (int i = 0; i < dataCount; i++)
        data[i] = cb(data[i], factor);
    saveData();
    printData();
}

// --- Sort with comparator function pointer (like qsort) ---
int compare_asc(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int compare_desc(const void *a, const void *b) {
    return compare_asc(b, a);
}

void op_sort(void) {
    printf("\n=======================================================\n");
    printf("               Sort (comparator)\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }

    int mode;
    printf("Sort order: 1=ascending  2=descending\n");
    printf("Enter mode: "); fflush(stdout);
    scanf("%d", &mode);
    getchar();

    int (*comparator)(const void *, const void *) =
        (mode == 2) ? compare_desc : compare_asc;

    qsort(data, dataCount, sizeof(double), comparator);
    saveData();
    printf("Sorted %s.\n", (mode == 2) ? "descending" : "ascending");
    printData();
}

// --- Search ---
void op_search(void) {
    printf("\n=======================================================\n");
    printf("                      Search\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is empty.\n"); return; }

    double target;
    printf("Enter value to search: "); fflush(stdout);
    scanf("%lf", &target);
    getchar();

    int found = 0;
    for (int i = 0; i < dataCount; i++) {
        if (data[i] == target) {
            printf("Found %.4g at index %d.\n", target, i);
            found++;
        }
    }
    if (!found) printf("Value %.4g not found in dataset.\n", target);
}

// --- Delete / reset dataset ---
void op_reset(void) {
    printf("\n=======================================================\n");
    printf("               Delete / Reset Dataset\n");
    printf("=======================================================\n");
    if (dataCount == 0) { printf("Dataset is already empty.\n"); return; }

    printf("This will delete all %d value(s). Continue? (y/N): ", dataCount);
    fflush(stdout);
    int c = getchar(); getchar(); // read answer + newline
    if (c != 'y' && c != 'Y') { printf("Cancelled.\n"); return; }

    int old = dataCount;
    dataCount = 0;
    saveData(); // writes an empty file
    printf("%d value(s) deleted. Dataset is now empty.\n", old);
}

// --- View/Add numbers ---
void op_load(void) {
    printf("\n=======================================================\n");
    printf("                  View / Add Data\n");
    printf("=======================================================\n");
    printData();
    printf("Enter numbers to add (type any non-number to stop):\n");

    double val;
    int added = 0;
    while (scanf("%lf", &val) == 1) {
        if (dataCount == dataCapacity) {
            dataCapacity = (dataCapacity == 0) ? 8 : dataCapacity * 2;
            double *tmp = realloc(data, dataCapacity * sizeof(double));
            if (!tmp) { printf("Realloc failed.\n"); break; }
            data = tmp;
        }
        data[dataCount++] = val;
        added++;
    }
    int c; while ((c = getchar()) != '\n' && c != EOF); // clear invalid token

    if (added > 0) {
        saveData();
        printf("Added %d number(s).\n", added);
        printData();
    }
}

// --- Dispatcher ---
typedef void (*Operation)(void);

int main() {
    dataCapacity = 8;
    data = malloc(dataCapacity * sizeof(double));
    if (!data) { printf("Memory allocation failed.\n"); return 1; }
    loadData();

    Operation handlers[] = {
        op_sum,       // 1
        op_average,   // 2
        op_min,       // 3
        op_max,       // 4
        op_filter,    // 5  — uses FilterCallback
        op_transform, // 6  — uses TransformCallback
        op_sort,      // 7  — uses comparator function pointer
        op_search,    // 8
        op_load,      // 9
        op_reset,     // 10 — delete / reset dataset
    };
    int numHandlers = (int)(sizeof(handlers) / sizeof(handlers[0]));

    int choice;
    do {
        printf("\n=======================================================\n");
        printf("          Dynamic Function Dispatcher Menu\n");
        printf("=======================================================\n");
        printf("Dataset: %d value(s) in %s\n", dataCount, DATA_FILE);
        printf("-------------------------------------------------------\n");
        printf("1.  Sum\n");
        printf("2.  Average\n");
        printf("3.  Minimum\n");
        printf("4.  Maximum\n");
        printf("5.  Filter          (callback)\n");
        printf("6.  Transform       (callback)\n");
        printf("7.  Sort            (comparator)\n");
        printf("8.  Search\n");
        printf("9.  View / Add Data\n");
        printf("10. Delete / Reset Dataset\n");
        printf("11. Exit\n");
        printf("Enter your choice: \n");
        fflush(stdout);

        // Validate numeric input
        char buf[32];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char *end; long v = strtol(buf, &end, 10);
        if (end == buf || (*end != '\n' && *end != '\0')) {
            printf("Invalid input — enter a number.\n");
            choice = 0; continue;
        }
        choice = (int)v;

        if (choice >= 1 && choice <= numHandlers) {
            Operation op = handlers[choice - 1];
            if (!op) { printf("Operation not available.\n"); continue; }
            op();
        } else if (choice != 11) {
            printf("Invalid choice. Enter 1-11.\n");
        }
    } while (choice != 11);

    printf("Exiting program. Freeing memory.\n");
    free(data);
    return 0;
}
