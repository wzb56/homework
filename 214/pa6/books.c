#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

struct book {
   char* title;
   double price;
   int id;
   char* category;
   struct book* next;
};

pthread_mutex_t bookLock;
pthread_mutex_t runLock;

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
char** allCats;
char** categories;

void* producer(void*);
void* consumer(void*);


void* producer(void* orderFileName) {
  /* Open orders file and load into memory */
  FILE *orderFile = fopen((char*)orderFileName, "r");
  if (orderFile != NULL) {
    char* line = malloc(2048);
    while(fgets (line, 2048, orderFile) != NULL) {
      struct book* newBook = malloc(sizeof(struct book) + 1);
      char* token;
      char* delims = "|\n";
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
      pthread_mutex_lock(&bookLock);
      newBook->next = orders;
      orders = newBook;
      pthread_mutex_unlock(&bookLock);
    }

    free(line);
    fclose(orderFile);
  }
  pthread_mutex_lock(&runLock);
  running = 0;
    pthread_mutex_unlock(&runLock);

  return NULL;
}


void* consumer(void* categoryI) {

  char* categoryName = (char*)categoryI;
  puts("### Before while ###\n");
    pthread_mutex_lock(&runLock);

  while ((running == 1) || (orders != NULL)) {
  pthread_mutex_unlock(&runLock);

    int found = 0;
    puts("### Inside while ###\n");
    //check Mutex
    pthread_mutex_lock(&bookLock);

    struct book* myOrders = orders;
    struct book* prev = orders;


    while(myOrders->next != NULL) {
      printf("Struct:_%s_\n", myOrders->category);
      printf("CategoryName:_%s_\n", categoryName);
      if(strcmp(myOrders->category, categoryName) == 0) {
        puts("### Found node ###\n");
        if (prev == myOrders) {
          myOrders = prev;
          orders = prev->next;
        }
        prev->next = myOrders->next;
        found = 1;
        break;
      }
      else{
        prev = myOrders;
        myOrders = myOrders->next;
      }
    }
    if (myOrders != NULL && myOrders->next == NULL) {
      if(strcmp(myOrders->category, categoryName) == 0) {
        orders = myOrders->next;
        found = 1;
      }
    }

    pthread_mutex_unlock(&bookLock);

    if(found == 1) {
       puts("### Found node and doing shit ###\n");
      int customerID = myOrders->id;

      struct customer* customerPtr = customerList;

      while (customerPtr != NULL)
      {
        if(customerPtr->id == customerID)
        {
          double currentBalance = customerPtr->balance;

          if(currentBalance >= myOrders->price)
          {
          //successfull
            customerPtr->balance = currentBalance - myOrders->price;
            struct hist* newHist = malloc(sizeof(struct hist));

            newHist->line = malloc(2048);
            char floatTmp[2048] = { '\0' };
            strcat (newHist->line, myOrders->title);
            strcat (newHist->line, "| ");
            sprintf(floatTmp, "%.2f", myOrders->price);
            strcat (newHist->line, floatTmp);
            strcat (newHist->line, "| ");
            sprintf(floatTmp, "%.2f", currentBalance);
            strcat (newHist->line, floatTmp);

            newHist->next = customerPtr->success;
            customerPtr->success = newHist;
          }
          else
          {
            struct hist* newHist = malloc(sizeof(struct hist));

            newHist->line = malloc(2048);
            char floatTmp[2048] = { '\0' };
            strcat (newHist->line, myOrders->title);
            strcat (newHist->line, "| ");
            sprintf(floatTmp, "%.2f", myOrders->price);
            strcat (newHist->line, floatTmp);

            newHist->next = customerPtr->fail;
            customerPtr->fail = newHist;
          }
        }
      }
      free(myOrders);
    }
  pthread_mutex_lock(&runLock);

    if(running == 0) {
      return NULL;
        pthread_mutex_unlock(&runLock);

    }

  }
  return NULL;
}

void output() {
  struct customer* ptr = customerList;

  //loop through customers
  while (ptr != NULL) {

    //print customer information
    puts("=== BEGIN CUSTOMER INFO ===\n");
    puts("### BALANCE ###\n");
    printf("Customer name: %s\n", ptr->name);
    printf("Customer ID number: %i\n", ptr->id);
    printf("Remaining credit balance after all purchases (a dollar amount): %f\n", ptr->balance);

    //print successes
    puts("### SUCCESSFUL ORDERS ###\n");

    struct hist* success = ptr->success;
    while (success != NULL) {
      printf("Customer name: %s\n", success->line);
      success = success->next;
    }

    //print fails
    puts("### REJECTED ORDERS ###\n");

    struct hist* fail = ptr->fail;

    while (fail != NULL) {
      printf("Customer name: %s\n", success->line);
      fail = fail->next;
    }

    puts("=== END CUSTOMER INFO ===\n\n");

    ptr = ptr->next;
  }
}

int main(int argc, char** argv) {
  if (argc != 4) {
    puts("Invalid number of arguments");
    return 1;
  }

  char* categoryList = argv[3];

  /* Build categories list */
  int categoryCount = 0;
  char* token;
  categories = malloc(sizeof(char*) * 50);
  for (token = strtok(categoryList, " "); token != NULL; token=strtok(NULL, " ")) {
    categories[categoryCount] = malloc( sizeof(char) * (strlen(token) + 1) );
    strcpy(categories[categoryCount], token);
    categoryCount++;
  }

  /* Load in database of customers */
  FILE *dbFile = fopen(argv[1], "r");
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

    /* Init mutexes and run threads */
    pthread_mutex_init(&bookLock, NULL);
    pthread_t prodT;

    pthread_create(&prodT, NULL, producer, argv[2]);
  /*if ( 1 != 0) {*/
    /*puts("yo");*/
    /*return 2;*/
  /*}*/
    pthread_t categoriesT[categoryCount];
    int i;
    for (i = 0; i < categoryCount; i++) {
      pthread_create(&categoriesT[i], NULL, consumer, categories[i]);
    }

    pthread_join(prodT, NULL);
    for (i = 0; i < categoryCount; i++) {
      pthread_join(categoriesT[i], NULL);
    }

    pthread_mutex_destroy(&bookLock);
    output();
  }
  else {
    puts("Database file failed to open");
    return 1;
  }

  //free the categories

  /*gundamFreedom();*/
  puts("done");

  return 0;
}

//customerlist (contains 2 linked lists of hist and a shit ton of strings), orders(strings)
void gundamFreedom() {
  return;
  /*struct book* tmp;*/
  /*while (orders != NULL) {*/
    /*tmp = orders;*/
    /*orders = orders->next;*/
    /*free(tmp->title);*/
    /*free(tmp->category);*/
    /*free(tmp);*/
  /*}*/

  /*struct customer* cusTmp;*/
  /*while (customerList != NULL) {*/
    /*tmp = customerList;*/
    /*customerList = customerList->next;*/
    /*while(tmp->success != NULL) {*/
      /*struct hisTmp = tmp->success;*/
      /*tmp->success = tmp->success->next;*/
      /*free(hisTmp->line);*/
      /*free(hisTmp);*/
    /*}*/
    /*while(tmp->fail != NULL) {*/
      /*struct hisTmp = tmp->fail;*/
      /*tmp->fail = tmp->fail->next;*/
      /*free(hisTmp->line);*/
      /*free(hisTmp);*/
    /*}*/
    /*free(tmp->name);*/
    /*free(tmp->address);*/
    /*free(tmp->state);*/
    /*free(tmp->zip);*/
    /*free(tmp);*/
  /*}*/
}
