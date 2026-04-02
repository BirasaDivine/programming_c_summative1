#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_URLS 5

typedef struct {
  char *url;
  int index;
} ScrapeTask;

typedef struct {
  char *data;
  size_t size;
} MemoryBuffer;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t total_size = size * nmemb;
  MemoryBuffer *mem = (MemoryBuffer *)userp;

  char *ptr = realloc(mem->data, mem->size + total_size + 1);
  if (!ptr) {
    printf("Not enough memory (realloc failed)\n");
    return 0;
  }

  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, total_size);
  mem->size += total_size;
  mem->data[mem->size] = 0;

  return total_size;
}

void save_to_file(int index, const char *data) {
  char filename[64];
  snprintf(filename, sizeof(filename), "output_%d.html", index);
  FILE *f = fopen(filename, "w");
  if (f) {
    fprintf(f, "%s\n", data);
    fclose(f);
    printf("Saved to %s\n", filename);
  } else {
    printf("Failed to save to file %s\n", filename);
  }
}

void *scrape(void *arg) {
  ScrapeTask *task = (ScrapeTask *)arg;

  printf("\n=======================================================\n");
  printf("            Thread %d - Fetching: %s\n", task->index, task->url);
  printf("=======================================================\n");

  CURL *curl = curl_easy_init();
  if (curl) {
    MemoryBuffer buffer = {0};
    curl_easy_setopt(curl, CURLOPT_URL, task->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
      save_to_file(task->index, buffer.data);
    } else {
      fprintf(stderr, "Thread %d: Failed to fetch %s\n", task->index,
              task->url);
    }

    curl_easy_cleanup(curl);
    free(buffer.data);
  }

  free(task->url);
  free(task);
  pthread_exit(NULL);
}

int main() {
  const char *urls[MAX_URLS] = {
      "https://example.com", "https://example.org", "https://httpbin.org/html",
      "https://www.gnu.org", "https://www.kernel.org"};

  pthread_t threads[MAX_URLS];
  curl_global_init(CURL_GLOBAL_ALL);

  for (int i = 0; i < MAX_URLS; i++) {
    ScrapeTask *task = malloc(sizeof(ScrapeTask));
    task->url = strdup(urls[i]);
    task->index = i + 1;
    pthread_create(&threads[i], NULL, scrape, task);
  }

  for (int i = 0; i < MAX_URLS; i++) {
    pthread_join(threads[i], NULL);
  }

  curl_global_cleanup();
  printf("\nAll scraping tasks completed.\n");
  return 0;
}
