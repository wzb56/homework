#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

struct book {
   char* title;
   double price;
   int id;
   char* category;
   struct book* next;
};

struct hist {
  char* line;
  struct hist* next;
};

struct customer {
  char* name;
  int id;
  double balance;
  char* address;
  char* state;
  char* zip;
  struct hist* success;
  struct hist* fail;
  struct customer* next;
};



struct book* orders = NULL;
struct customer* customerList = NULL;
int running = 1;

void producer(char*);
void consumer(char*);


void producer(char* orderFileName) {
  /* Open orders file and load into memory */
  FILE *orderFile = fopen(orderFileName, "r");
  if (orderFile != NULL) {
    char* line = malloc(2048);
    while(fgets (line, 2048, orderFile) != NULL) {
      struct book* newBook = malloc(sizeof(struct book) + 1);
      char* token;
      char* delims = "|";
      int i = 0;
      for (token = strtok(line, delims); token != NULL; token=strtok(NULL, delims)) {
        switch(i) {
          case 0: /*name*/
            newBook->title = malloc(strlen(token) + 1);
            strcpy(newBook->title, token);
            break;
          case 1: /*price*/
            newBook->price = atof(token); break;
          case 2: /*customerID*/
            newBook->id = atoi(token); break;
          case 3: /*category*/
            newBook->category = malloc(strlen(token) + 1);
            strcpy(newBook->category, token);
            break;
          default :
            break;
        }
        i++;
      }
      newBook->next = orders;
      orders = newBook;
    }

    free(line);
    fclose(orderFile);
  }
  return;
}


void consumer(char* category) {
  struct customer* customerList = NULL;
  return;
}


int main(int argc, char** argv) {
  if (argc != 4) {
    puts("Invalid number of arguments");
    return 1;
  }

  char* db = argv[1];
  char* orders = argv[2];
  char* categoryList = argv[3];

  /* Build categories list */
  int categoryCount = 0;
  char* token;
  for (token = strtok(categoryList, " "); token != NULL; token=strtok(NULL, " ")) {
    categoryCount++;
  }
  char** categories = malloc(sizeof(char*) * categoryCount + 1);
  int i = 0;
  for (token = strtok(categoryList, " "); token != NULL; token=strtok(NULL, " ")) {
    categories[i] = malloc( sizeof(char) * (strlen(token) + 1) );
    strcpy(categories[i], token);
  }

  /* Load in database of customers */
  FILE *dbFile = fopen(db, "r");
  if (dbFile != NULL) {
    char* line = malloc(2048);
    while (fgets (line, 2048, dbFile) != NULL) {
      struct customer* newCust = malloc(sizeof(struct customer) + 1);
      newCust->success = NULL;
      newCust->fail = NULL;
      char* token;
      char* delims = "|";
      int i = 0;
      for (token=strtok(line, delims); token != NULL; token=strtok(NULL, delims)) {
        switch(i) {
          case 0: /*name*/
            newCust->name = malloc(strlen(token) + 1);
            strcpy(newCust->name, token);
            break;
          case 1: /*id*/
            newCust->id = atoi(token);
            break;
          case 2: /*balance*/
            newCust->balance = atof(token);
            break;
          case 3: /*address*/
            newCust->address = malloc(strlen(token) + 1);
            strcpy(newCust->address, token);
            break;
          case 4: /*state*/
            newCust->state = malloc(strlen(token) + 1);
            strcpy(newCust->state, token);
            break;
          case 5: /*zip*/
            newCust->zip = malloc(strlen(token) + 1);
            strcpy(newCust->zip, token);
            break;
          default :
            break;
        }
        i++;
      }
      newCust->next = customerList;
      customerList = newCust;
    }
    free(line);
    fclose(dbFile);
  }
  else {
    puts("Database file failed to open");
    return 1;
  }

  return 0;
}


