// TODO:
// 1. `stock_add_ingredient` should add the ingredient in the right position (by
// expiration date)
//
// 2. `send_order` should remove from stock all the expired ingredients of this
// stock. Increment the head until reach the CURR_TIME

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINE ===========================================
#define LINE_SIZE 512
#define NAME_LEN 256
#define COMMAND_LEN 17
#define HT_LOAD_FACTOR 0.75
#define HT_INIT_SIZE 16 // TODO: Evaluate to increase
// END DEFINE =======================================

// GLOBAL VARIABLES =================================
static int CURR_TIME = 0;
static int TRUCK_TIME = 0;
static int TRUCK_WEIGHT = 0;
// END GLOBAL VARIABLES =============================

// TRUCK ================================
typedef struct TruckNode TruckNode;
TruckNode *create_truck_node(char *);
void free_truck_node(TruckNode *);

typedef struct TruckQueue TruckQueue;
TruckQueue *create_truck_queue();
void free_truck_queue(TruckQueue *);

void set_truck_time(int); // TODO: Try to remove here all the expiring
                          // ingredients, and see the performance
void set_truck_weight(int);
void handle_truck(char *);
// END TRUCK ============================

// RECIPE ===============================
typedef struct RecipeIngredient RecipeIngredient;
typedef struct Recipe Recipe;
typedef struct RecipeHT RecipeHT;

RecipeIngredient *create_recipe_ingredient(char *, int);
void free_recipe_ingredient(RecipeIngredient *);

Recipe *create_recipe(char *);
void free_recipe(Recipe *);
void recipe_add_ingredient(Recipe *, RecipeIngredient *);

RecipeHT *create_recipe_ht(int);
void free_recipe_ht(RecipeHT *);
void recipe_ht_put(RecipeHT *, Recipe *);
Recipe *recipe_ht_get(RecipeHT *, char *);
void recipe_ht_delete(RecipeHT *, char *);
double recipe_ht_load_factor(RecipeHT *);
void recipe_ht_resize(RecipeHT *);
// END RECIPE ===========================

// STOCK ============================
typedef struct StockIngredient StockIngredient;
typedef struct Stock Stock; // of an ingredient
typedef struct StockHT StockHT;

StockIngredient *create_stock_ingredient(int, int);
void free_stock_ingredient(StockIngredient *);

Stock *create_stock(char *);
void free_stock(Stock *);
void stock_add_ingredient(Stock *, StockIngredient *);

StockHT *create_stock_ht(int);
void free_stock_ht(StockHT *);
void stock_ht_put(StockHT *, Stock *);
Stock *stock_ht_get(StockHT *, char *);
double stock_ht_load_factor(StockHT *);
void stock_ht_resize(StockHT *);
// void stock_ht_delete(StockHT *, char *);

Stock *stock_get_or_create(StockHT *, char *);
// END STOCK ============================

// ORDER ===============================
typedef struct Order Order;
Order *create_order(Recipe *, int, int);
void free_order(Order *);

typedef struct WaitingNode WaitingNode;
WaitingNode *create_waiting_node(Order *, Stock **, int);
void free_waiting_node(WaitingNode *);

typedef struct WaitingQueue WaitingQueue;
WaitingQueue *create_waiting_queue();
void free_waiting_queue(WaitingQueue *);
void waiting_queue_enqueue(WaitingQueue *, WaitingNode *);

void send_order(RecipeHT *, StockHT *, WaitingQueue *, TruckQueue *, Order *,
                bool);
// END ORDER ===========================

// UTIL =================================
uint32_t hash_string(char *, int);
void increase_curr_time();
char *read_line(FILE *);

void add_recipe(RecipeHT *, char *);
void remove_recipe(RecipeHT *, char *);
void handle_stock(StockHT *, char *);
void handle_order(RecipeHT *, StockHT *, WaitingQueue *, TruckQueue *, char *);
// END UTIL =============================

// RECIPE IMPLEMENTATION ============================
struct RecipeIngredient {
  char name[NAME_LEN];
  int quantity;
  RecipeIngredient *next;
};

struct Recipe {
  char name[NAME_LEN];
  int weight;
  int n_ingredients;
  RecipeIngredient *ingredients;
  Recipe *next;
};

struct RecipeHT {
  int n_elements;
  int size; // Number of buckets
  Recipe **recipes;
};

RecipeIngredient *create_recipe_ingredient(char *name, int quantity) {
  RecipeIngredient *ingredient =
      (RecipeIngredient *)malloc(sizeof(RecipeIngredient));
  if (ingredient == NULL) {
    printf("Error while allocating RecipeIngredient\n");
    return NULL;
  }

  strcpy(ingredient->name, name);
  ingredient->quantity = quantity;
  ingredient->next = NULL;

  return ingredient;
}

void free_recipe_ingredient(RecipeIngredient *ingredient) {
  while (ingredient != NULL) {
    RecipeIngredient *next = ingredient->next;
    free(ingredient);
    ingredient = next;
  }
}

Recipe *create_recipe(char *name) {
  Recipe *recipe = (Recipe *)malloc(sizeof(Recipe));
  if (recipe == NULL) {
    printf("Error while allocating Recipe\n");
    return NULL;
  }

  strcpy(recipe->name, name);
  recipe->weight = 0;
  recipe->n_ingredients = 0;
  recipe->ingredients = NULL;
  recipe->next = NULL;

  return recipe;
}

void free_recipe(Recipe *recipe) {
  free_recipe_ingredient(recipe->ingredients);
  free(recipe);
}

RecipeHT *create_recipe_ht(int size) {
  RecipeHT *ht = (RecipeHT *)malloc(sizeof(RecipeHT));
  if (ht == NULL) {
    printf("Error while allocating RecipeHT\n");
    return NULL;
  }

  ht->n_elements = 0;
  ht->size = size;
  ht->recipes = (Recipe **)malloc(size * sizeof(Recipe *));
  if (ht->recipes == NULL) {
    printf("Error while allocating RecipeHT recipes\n");
    return NULL;
  }
  for (int i = 0; i < size; i++) {
    ht->recipes[i] = NULL;
  }

  return ht;
}

void free_recipe_ht(RecipeHT *ht) {
  for (int i = 0; i < ht->size; i++) {
    Recipe *recipe = ht->recipes[i];
    while (recipe != NULL) {
      Recipe *next = recipe->next;
      free_recipe(recipe);
      recipe = next;
    }
  }

  free(ht->recipes);
  free(ht);
}

void recipe_add_ingredient(Recipe *recipe, RecipeIngredient *ingredient) {
  recipe->weight += ingredient->quantity;
  recipe->n_ingredients++;
  ingredient->next = recipe->ingredients;
  recipe->ingredients = ingredient;
}

Recipe *recipe_ht_get(RecipeHT *ht, char *name) {
  uint32_t hash = hash_string(name, ht->size);
  Recipe *recipe = ht->recipes[hash];

  if (recipe == NULL) {
    return NULL;
  }

  while (recipe != NULL) {
    if (strcmp(recipe->name, name) == 0) {
      return recipe;
    }
    recipe = recipe->next;
  }

  return NULL;
}

void recipe_ht_put(RecipeHT *ht, Recipe *recipe) {
  uint32_t hash = hash_string(recipe->name, ht->size);
  Recipe *curr_recipe = ht->recipes[hash];

  if (curr_recipe == NULL) {
    ht->recipes[hash] = recipe;
    ht->n_elements++;
    return;
  }
  recipe->next = curr_recipe;
  ht->recipes[hash] = recipe;
  ht->n_elements++;

  if (recipe_ht_load_factor(ht) >= HT_LOAD_FACTOR) {
    recipe_ht_resize(ht);
  }
}

void recipe_ht_delete(RecipeHT *ht, char *name) {
  // if (ordini in sospeso != NULL) {
  //   printf("ordini in sospeso\n");
  //   return;
  // }

  uint32_t hash = hash_string(name, ht->size);
  Recipe *prev_recipe = NULL;
  Recipe *curr_recipe = ht->recipes[hash];

  while (curr_recipe != NULL && strcmp(curr_recipe->name, name) != 0) {
    prev_recipe = curr_recipe;
    curr_recipe = curr_recipe->next;
  }
  if (curr_recipe == NULL) {
    printf("non presente\n");
    return;
  }
  if (prev_recipe == NULL)
    ht->recipes[hash] = curr_recipe->next;
  else
    prev_recipe->next = curr_recipe->next;

  free_recipe(curr_recipe);
  ht->n_elements--;

  printf("rimossa\n");
}

double recipe_ht_load_factor(RecipeHT *ht) {
  return (double)ht->n_elements / ht->size;
}
void recipe_ht_resize(RecipeHT *ht) {
  RecipeHT *new_ht = create_recipe_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Recipe *recipe = ht->recipes[i];
    while (recipe != NULL) {
      Recipe *next = recipe->next;
      recipe_ht_put(new_ht, recipe);
      recipe = next;
    }
  }

  free(ht->recipes);
  ht->recipes = new_ht->recipes;
  ht->size = new_ht->size;
  ht->n_elements = new_ht->n_elements;
  free(new_ht);
}

// END RECIPE IMPLEMENTATION =========================

// STOCK IMPLEMENTATION ============================
struct StockIngredient {
  int quantity;
  int expiration_date;
  StockIngredient *next;
};

struct Stock {
  char name[NAME_LEN];
  int n_ingredients; // TODO: Evaluate to remove
  int total_quantity;
  StockIngredient *ingredients;
  Stock *next;
};

struct StockHT {
  int n_elements;
  int size; // Number of buckets
  Stock **stocks;
};

StockIngredient *create_stock_ingredient(int quantity, int expiration_date) {
  StockIngredient *ingredient =
      (StockIngredient *)malloc(sizeof(StockIngredient));
  if (ingredient == NULL) {
    printf("Error while allocating StockIngredient\n");
    return NULL;
  }

  ingredient->quantity = quantity;
  ingredient->expiration_date = expiration_date;
  ingredient->next = NULL;

  return ingredient;
}

void free_stock_ingredient(StockIngredient *ingredient) {
  while (ingredient != NULL) {
    StockIngredient *next = ingredient->next;
    free(ingredient);
    ingredient = next;
  }
}

void stock_add_ingredient(Stock *stock, StockIngredient *ingredient) {
  stock->total_quantity += ingredient->quantity;
  stock->n_ingredients++;
  ingredient->next =
      stock->ingredients; // TODO: Consider to add in expiration date order
  stock->ingredients = ingredient;
}

Stock *create_stock(char *name) {
  Stock *stock = (Stock *)malloc(sizeof(Stock));
  if (stock == NULL) {
    printf("Error while allocating Stock\n");
    return NULL;
  }

  strcpy(stock->name, name);
  stock->n_ingredients = 0;
  stock->total_quantity = 0;
  stock->ingredients = NULL;
  stock->next = NULL;

  return stock;
}

void free_stock(Stock *stock) {
  free_stock_ingredient(stock->ingredients);
  free(stock);
}

StockHT *create_stock_ht(int size) {
  StockHT *ht = (StockHT *)malloc(sizeof(StockHT));
  if (ht == NULL) {
    printf("Error while allocating StockHT\n");
    return NULL;
  }

  ht->n_elements = 0;
  ht->size = size;
  ht->stocks = (Stock **)malloc(size * sizeof(Stock *));
  if (ht->stocks == NULL) {
    printf("Error while allocating StockHT stocks\n");
    return NULL;
  }
  for (int i = 0; i < size; i++) {
    ht->stocks[i] = NULL;
  }

  return ht;
}

void free_stock_ht(StockHT *ht) {
  for (int i = 0; i < ht->size; i++) {
    Stock *stock = ht->stocks[i];
    while (stock != NULL) {
      Stock *next = stock->next;
      free_stock(stock);
      stock = next;
    }
  }

  free(ht->stocks);
  free(ht);
}

void stock_ht_put(StockHT *ht, Stock *stock) {
  uint32_t hash = hash_string(stock->name, ht->size);
  Stock *curr_stock = ht->stocks[hash];

  if (curr_stock == NULL) {
    ht->stocks[hash] = stock;
    ht->n_elements++;
    return;
  }
  stock->next = curr_stock;
  ht->stocks[hash] = stock;
  ht->n_elements++;

  if (stock_ht_load_factor(ht) >= HT_LOAD_FACTOR) {
    stock_ht_resize(ht);
  }
}

Stock *stock_ht_get(StockHT *ht, char *name) {
  uint32_t hash = hash_string(name, ht->size);
  Stock *stock = ht->stocks[hash];

  if (stock == NULL) {
    return NULL;
  }

  while (stock != NULL) {
    if (strcmp(stock->name, name) == 0) {
      return stock;
    }
    stock = stock->next;
  }

  return NULL;
}

double stock_ht_load_factor(StockHT *ht) {
  return (double)ht->n_elements / ht->size;
}

void stock_ht_resize(StockHT *ht) {
  StockHT *new_ht = create_stock_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Stock *stock = ht->stocks[i];
    while (stock != NULL) {
      Stock *next = stock->next;
      stock_ht_put(new_ht, stock);
      stock = next;
    }
  }

  free(ht->stocks);
  ht->stocks = new_ht->stocks;
  ht->size = new_ht->size;
  ht->n_elements = new_ht->n_elements;
  free(new_ht);
}

Stock *stock_get_or_create(StockHT *ht, char *name) {
  Stock *stock = stock_ht_get(ht, name);
  if (stock == NULL) {
    stock = create_stock(name);
    stock_ht_put(ht, stock);
  }
  return stock;
}

// END STOCK IMPLEMENTATION ========================

// TRUCK IMPLEMENTATION ==============================
void set_truck_time(int time) { TRUCK_TIME = time; }
void set_truck_weight(int weight) { TRUCK_WEIGHT = weight; }
void handle_truck(char *line) {
  char *time = strtok(line, " ");
  if (time == NULL) {
    printf("Error while parsing truck time\n");
    return;
  }
  char *weight = strtok(NULL, " ");
  if (weight == NULL) {
    printf("Error while parsing truck weight\n");
    return;
  }
  set_truck_time(atoi(time));
  set_truck_weight(atoi(weight));
}
// END TRUCK IMPLEMENTATION =========================

// Per ogni ordine in sospeso mi tengo solo gli ingredienti che hanno reso
// questo ordine non soddisfacibile

// ORDER IMPLEMENTATION ===============================
struct Order {
  Recipe *recipe;
  int amount;
  int arrival_time;
  int total_quantity;
};

Order *create_order(Recipe *recipe, int amount, int arrival_time) {
  Order *order = (Order *)malloc(sizeof(Order));
  if (order == NULL) {
    printf("Error while allocating Order\n");
    return NULL;
  }

  order->recipe = recipe;
  order->amount = amount;
  order->arrival_time = arrival_time;
  order->total_quantity = recipe->weight * amount;

  return order;
}

void free_order(Order *order) {
  // Trivially, we don't need to free the recipe because it's a reference
  free(order);
}

struct WaitingNode {
  Order *order;
  Stock **missing_ingredients;
  int n_missing_ingredients;
  WaitingNode *next;
};

WaitingNode *create_waiting_node(Order *order, Stock **missing_ingredients,
                                 int n_missing_ingredients) {
  WaitingNode *node = (WaitingNode *)malloc(sizeof(WaitingNode));
  if (node == NULL) {
    printf("Error while allocating WaitingNode\n");
    return NULL;
  }

  node->order = order;
  node->missing_ingredients = missing_ingredients;
  node->n_missing_ingredients = n_missing_ingredients;
  node->next = NULL;

  return node;
}
void free_waiting_node(WaitingNode *node) {
  free_order(node->order);
  free(node);
}

struct WaitingQueue {
  WaitingNode *head;
  WaitingNode *tail;
};

WaitingQueue *create_waiting_queue() {
  WaitingQueue *queue = (WaitingQueue *)malloc(sizeof(WaitingQueue));
  if (queue == NULL) {
    printf("Error while allocating WaitingQueue\n");
    return NULL;
  }

  queue->head = NULL;
  queue->tail = NULL;
  return queue;
}

void free_waiting_queue(WaitingQueue *queue) {
  WaitingNode *node = queue->head;
  while (node != NULL) {
    WaitingNode *next = node->next;
    free_waiting_node(node);
    node = next;
  }
  free(queue);
}

void waiting_queue_enqueue(WaitingQueue *queue, WaitingNode *node) {
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
    return;
  }

  queue->tail->next = node;
  queue->tail = node;
}

void send_order(
    RecipeHT *recipe_ht, StockHT *stock_ht, WaitingQueue *waiting_queue,
    TruckQueue *truck_queue, Order *order,
    bool is_waiting_order) { // TODO: Consider to return a boolean value
  Stock **missing_ingredients =
      (Stock **)malloc(order->recipe->n_ingredients * sizeof(Stock *));
  if (missing_ingredients == NULL) {
    printf("Error while allocating missing ingredients\n");
    return;
  }

  // Check if all ingredients are available
  RecipeIngredient *ingredient = order->recipe->ingredients;
  int n_missing_ingredients = 0;
  while (ingredient != NULL) {
    Stock *stock = stock_ht_get(stock_ht, ingredient->name);
    // if (stock != NULL) {
    // TODO: Remove all expired ingredients of this stock
    // }
    if (stock == NULL || order->total_quantity > stock->total_quantity) {
      missing_ingredients[n_missing_ingredients++] = stock;
    }
    ingredient = ingredient->next;
  }

  if (n_missing_ingredients == 0) {
    // TODO: Remove from stock all the ingredients that are needed

    // If the order was arrived from the waiting queue, we should enqueue the
    // order in the truck queue in the right position (by arrival time) (O(n))

    // Otherwise, we should enqueue the order in the truck queue at the end
    // (O(1))
    return;
  }

  // If not from the waiting queue, we should enqueue the order in the waiting
}
// END ORDER IMPLEMENTATION ===========================

// UTIL IMPLEMENTATION ==============================
uint32_t hash_string(char *str, int size) {
  unsigned long hash = 2166136261UL;
  int c;

  while ((c = *str++)) {
    hash ^= c;
    hash *= 16777619;
  }

  return hash % size;
}

void increase_curr_time() { CURR_TIME++; }

char *read_line(FILE *stream) {
  char *buffer = (char *)malloc(LINE_SIZE * sizeof(char));
  if (buffer == NULL) {
    printf("Error while allocating buffer size\n");
    return NULL;
  }

  size_t size = LINE_SIZE;
  int c;
  size_t i = 0;

  while ((c = getchar_unlocked()) != EOF && c != '\n') {
    if (i >= size - 1) {
      size_t new_size = (size * 2);
      char *new_buffer = (char *)realloc(buffer, new_size * sizeof(char));
      if (buffer == NULL) {
        printf("Error while reallocating buffer size\n");
        return NULL;
      }
      buffer = new_buffer;
      size = new_size;
    }
    buffer[i++] = c;
  }

  if (ferror(stream)) {
    printf("Error while reading from stream\n");
    free(buffer);
    return NULL;
  }

  if (i == 0 && c == EOF) {
    free(buffer);
    return NULL;
  }

  buffer[i] = '\0';

  return buffer;
}

void add_recipe(RecipeHT *ht, char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    printf("Error while parsing command\n");
    return;
  }

  char *recipe_name = strtok(NULL, " ");
  if (recipe_name == NULL) {
    printf("Error while parsing recipe name\n");
    return;
  }

  if (recipe_ht_get(ht, recipe_name) != NULL) {
    printf("ingnorato\n");
    return;
  }

  Recipe *recipe = create_recipe(recipe_name);
  char *ingredient_name;
  while ((ingredient_name = strtok(NULL, " ")) != NULL) {
    char *ingredient_quantity = strtok(NULL, " ");
    if (ingredient_quantity == NULL) {
      printf("Error while parsing ingredient quantity\n");
      return;
    }
    RecipeIngredient *ingredient =
        create_recipe_ingredient(ingredient_name, atoi(ingredient_quantity));
    recipe_add_ingredient(recipe, ingredient);
  }

  recipe_ht_put(ht, recipe);
  printf("aggiunta\n");
}

void remove_recipe(RecipeHT *ht, char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    printf("Error while parsing command\n");
    return;
  }
  char *name = strtok(NULL, " ");
  if (name == NULL) {
    printf("Error while parsing recipe name\n");
    return;
  }

  recipe_ht_delete(ht, name);
}

void handle_stock(StockHT *stock_ht, char *line) {
  char *command = strtok(line, " ");
  printf("%s\n", command);
  if (command == NULL) {
    printf("Error while parsing command\n");
    return;
  }

  char *stock_name;
  while ((stock_name = strtok(NULL, " ")) != NULL) {
    char *quantity_str = strtok(NULL, " ");
    if (quantity_str == NULL) {
      printf("Error while parsing stock quantity\n");
      return;
    }
    int quantity = atoi(quantity_str);

    char *expiration_date_str = strtok(NULL, " ");
    if (expiration_date_str == NULL) {
      printf("Error while parsing stock expiration date\n");
      return;
    }
    int expiration_date = atoi(expiration_date_str);

    StockIngredient *ingredient =
        create_stock_ingredient(quantity, expiration_date);
    Stock *stock = stock_get_or_create(stock_ht, stock_name);
    if (stock == NULL) {
      printf("Error while getting or creating stock\n");
      return;
    }
    stock_add_ingredient(stock, ingredient);
  }
}

void handle_order(RecipeHT *recipe_ht, StockHT *stock_ht,
                  WaitingQueue *waiting_queue, TruckQueue *truck_queue,
                  char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    printf("Error while parsing command\n");
    return;
  }

  char *recipe_name = strtok(NULL, " ");
  if (recipe_name == NULL) {
    printf("Error while parsing recipe name\n");
    return;
  }

  char *amount_str = strtok(NULL, " ");
  if (amount_str == NULL) {
    printf("Error while parsing order amount\n");
    return;
  }
  int amount = atoi(amount_str);

  Recipe *recipe = recipe_ht_get(recipe_ht, recipe_name);
  if (recipe == NULL) {
    printf("rifiutato\n");
    return;
  }
  printf("accettato\n");

  Order *order = create_order(recipe, amount, CURR_TIME);
  send_order(recipe_ht, stock_ht, waiting_queue, truck_queue, order, false);
}
// END UTIL IMPLEMENTATION ==========================

int main(void) {
  RecipeHT *recipe_ht = create_recipe_ht(HT_INIT_SIZE);
  StockHT *stock_ht = create_stock_ht(HT_INIT_SIZE);
  WaitingQueue *waiting_queue = create_waiting_queue();
  TruckQueue *truck_queue = create_truck_queue();

  char command[COMMAND_LEN];
  char *line;

  while ((line = read_line(stdin)) != NULL) {
    if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
#ifdef DEBUG
      printf("DEBUG: HANDLE TRUCK\n");
#endif /* ifdef DEBUG */
    }

    if (sscanf(line, "%s", command) == 1) {
      if (strcmp(command, "aggiungi_ricetta") == 0) {
#ifdef DEBUG
        printf("DEBUG: %s\n", line);
#endif /* ifdef DEBUG */
        add_recipe(recipe_ht, line);
        increase_curr_time();
      } else if (strcmp(command, "rimuovi_ricetta") == 0) {
#ifdef DEBUG
        printf("DEBUG: %s\n", line);
#endif /* ifdef DEBUG */
        remove_recipe(recipe_ht, line);
        increase_curr_time();
      } else if (strcmp(command, "rifornimento") == 0) {
#ifdef DEBUG
        printf("DEBUG: %s\n", line);
#endif /* ifdef DEBUG */
        handle_stock(stock_ht, line);
        increase_curr_time();
      } else if (strcmp(command, "ordine") == 0) {
#ifdef DEBUG
        printf("DEBUG: %s\n", line);
#endif /* ifdef DEBUG */
        handle_order(recipe_ht, stock_ht, waiting_queue, truck_queue, line);
        increase_curr_time();
      } else {
        handle_truck(line);
      }
    }

    free(line);
  }

  free_recipe_ht(recipe_ht);
  free_stock_ht(stock_ht);
  free_waiting_queue(waiting_queue);
}
