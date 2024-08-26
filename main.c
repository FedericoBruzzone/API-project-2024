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

// RECIPE ===============================
typedef struct RecipeIngredient RecipeIngredient;
RecipeIngredient *create_recipe_ingredient(char *, int);
void free_recipe_ingredient(RecipeIngredient *);

typedef struct Recipe Recipe;
Recipe *create_recipe(char *);
void free_recipe(Recipe *);
void recipe_add_ingredient(Recipe *, RecipeIngredient *);

typedef struct RecipeHT RecipeHT;
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
void stock_remove_expired_ingredients(Stock *, int);
void stock_remove_ingredient(Stock *, int);

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
// END ORDER ===========================

// TRUCK ================================
typedef struct TruckNode TruckNode;
TruckNode *create_truck_node(Order *);
void free_truck_node(TruckNode *);
void order_node_enqueue_by_weight(TruckNode **, TruckNode *);

typedef struct TruckQueue TruckQueue;
TruckQueue *create_truck_queue();
void free_truck_queue(TruckQueue *);
void truck_queue_enqueue(TruckQueue *, TruckNode *);
void truck_queue_enqueue_by_arrival_time(TruckQueue *, TruckNode *);

void set_truck_time(int); // TODO: Try to remove here all the expiring
                          // ingredients, and see the performance
void set_truck_weight(int);

void truck_queue_dequeue(TruckQueue *);
// END TRUCK ============================

// UTIL =================================
uint32_t hash_string(char *, int);
void increase_curr_time();
char *read_line(FILE *);

void add_recipe(RecipeHT *, char *);
void remove_recipe(RecipeHT *, char *);
void handle_stock(StockHT *, char *, WaitingQueue *, TruckQueue *);
void handle_order(RecipeHT *, StockHT *, WaitingQueue *, TruckQueue *, char *);
void handle_truck(char *);

bool send_order(StockHT *, WaitingQueue *, TruckQueue *, Order *, bool);
void check_waiting_orders(WaitingQueue *, TruckQueue *, StockHT *);

void print_stock_ingredients(Stock *);
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
  int n_waiting_orders;
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
  recipe->n_waiting_orders = 0;

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
  uint32_t hash = hash_string(name, ht->size);
  Recipe *curr_recipe = ht->recipes[hash];

  // Check if there are waiting orders with this recipe
  if (curr_recipe->n_waiting_orders > 0) {
    printf("ordini in sospeso\n");
    return;
  }

  Recipe *prev_recipe = NULL;

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

  if (stock->ingredients == NULL) {
    stock->ingredients = ingredient;
    ingredient->next = NULL;
    stock->n_ingredients++;
    return;
  }

  StockIngredient *curr_ingredient = stock->ingredients;
  StockIngredient *prev_ingredient = NULL;
  while (curr_ingredient != NULL &&
         curr_ingredient->expiration_date < ingredient->expiration_date) {
    prev_ingredient = curr_ingredient;
    curr_ingredient = curr_ingredient->next;
  }

  if (prev_ingredient == NULL) {
    ingredient->next = stock->ingredients;
    stock->ingredients = ingredient;
    stock->n_ingredients++;
    return;
  }
  if (curr_ingredient != NULL &&
      curr_ingredient->expiration_date == ingredient->expiration_date) {
    curr_ingredient->quantity += ingredient->quantity;
    free(ingredient);
    return;
  }

  prev_ingredient->next = ingredient;
  ingredient->next = curr_ingredient;
  stock->n_ingredients++;
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

void stock_remove_expired_ingredients(Stock *stock, int curr_time) {
  StockIngredient *curr_ingredient = stock->ingredients;
  StockIngredient *prev_ingredient = NULL;
  while (curr_ingredient != NULL &&
         curr_ingredient->expiration_date <= curr_time) {
    StockIngredient *next = curr_ingredient->next;
    stock->total_quantity -= curr_ingredient->quantity;
    free(curr_ingredient);
    stock->n_ingredients--;
    curr_ingredient = next;
  }

  if (prev_ingredient == NULL) {
    stock->ingredients = curr_ingredient;
    return;
  }

  prev_ingredient->next = curr_ingredient;
}

void stock_remove_ingredient(Stock *stock, int quantity) {
  printf("Removing ingredient: %d\n", quantity);

  StockIngredient *curr_ingredient = stock->ingredients;
  StockIngredient *prev_ingredient = NULL;
  while (curr_ingredient != NULL && quantity > 0) {
    if (curr_ingredient->quantity <= quantity) {
      quantity -= curr_ingredient->quantity;
      stock->total_quantity -= curr_ingredient->quantity;
      StockIngredient *next = curr_ingredient->next;
      free(curr_ingredient);
      stock->n_ingredients--;
      curr_ingredient = next;
    } else {
      curr_ingredient->quantity -= quantity;
      stock->total_quantity -= quantity;
      quantity = 0;
    }
  }

  if (prev_ingredient == NULL) {
    stock->ingredients = curr_ingredient;
    return;
  }

  prev_ingredient->next = curr_ingredient;
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

// ORDER IMPLEMENTATION ===============================
struct Order {
  Recipe *recipe;
  int amount; // Number of orders
  int arrival_time;
  int total_weight;
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
  order->total_weight = recipe->weight * amount;

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
// END ORDER IMPLEMENTATION ===========================

// TRUCK IMPLEMENTATION ==============================
struct TruckNode {
  Order *order;
  TruckNode *next;
};

TruckNode *create_truck_node(Order *order) {
  TruckNode *node = (TruckNode *)malloc(sizeof(TruckNode));
  if (node == NULL) {
    printf("Error while allocating TruckNode\n");
    return NULL;
  }

  node->order = order;
  node->next = NULL;

  return node;
}
void free_truck_node(TruckNode *node) {
  free_order(node->order);
  free(node);
}

void order_node_enqueue_by_weight(TruckNode **list, TruckNode *node) {
  // Add node in decreasing order according to the weight in the list
  if (*list == NULL) {
    *list = node;
    return;
  }

  TruckNode *curr = *list;
  TruckNode *prev = NULL;
  while (curr != NULL &&
         curr->order->total_weight > node->order->total_weight) {
    prev = curr;
    curr = curr->next;
  }

  if (prev == NULL) {
    node->next = *list;
    *list = node;
    return;
  }

  prev->next = node;
  node->next = curr;
}

struct TruckQueue {
  TruckNode *head;
  TruckNode *tail;
};

TruckQueue *create_truck_queue() {
  TruckQueue *queue = (TruckQueue *)malloc(sizeof(TruckQueue));
  if (queue == NULL) {
    printf("Error while allocating TruckQueue\n");
    return NULL;
  }

  queue->head = NULL;
  queue->tail = NULL;
  return queue;
}

void free_truck_queue(TruckQueue *queue) {
  TruckNode *node = queue->head;
  while (node != NULL) {
    TruckNode *next = node->next;
    free_truck_node(node);
    node = next;
  }
  free(queue);
}

void truck_queue_enqueue(TruckQueue *queue, TruckNode *node) {
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
    return;
  }

  queue->tail->next = node;
  queue->tail = node;
}

void truck_queue_enqueue_by_arrival_time(TruckQueue *queue, TruckNode *node) {
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
    return;
  }

  // Find the correct position to insert the node (by arrival time).
  TruckNode *curr = queue->head;
  TruckNode *prev = NULL;
  while (curr != NULL &&
         curr->order->arrival_time < node->order->arrival_time) {
    prev = curr;
    curr = curr->next;
  }

  if (prev == NULL) {
    node->next = queue->head;
    queue->head = node;
    return;
  }

  prev->next = node;
  node->next = curr;

  // Print truck queue
  TruckNode *curr_node = queue->head;
  while (curr_node != NULL) {
    curr_node = curr_node->next;
  }
}

void truck_queue_dequeue(TruckQueue *queue) {
  if (queue->head == NULL) {
    printf("camioncino vuoto\n");
    return;
  }

  // Remove order from the list until the truck is full (TRUCK_WEIGHT)
  TruckNode *node = queue->head;
  int tmp_weight = 0;
  int n_orders = 0;
  TruckNode *orders = NULL;

  while (node != NULL &&
         tmp_weight + node->order->total_weight <= TRUCK_WEIGHT) {
    tmp_weight += node->order->total_weight;
    n_orders++;

    TruckNode *order_node = create_truck_node(node->order);
    order_node_enqueue_by_weight(&orders, order_node);
    node = node->next;
  }

  if (n_orders == 0) {
    printf("camioncino vuoto\n");
    return;
  }

  TruckNode *curr_order = orders;
  for (int i = 0; i < n_orders; i++) {
    printf("%d %s %d\n", curr_order->order->arrival_time,
           curr_order->order->recipe->name, curr_order->order->amount);
    curr_order->order->recipe->n_waiting_orders--;
    TruckNode *next = curr_order->next;
    free_truck_node(curr_order);
    curr_order = next;
  }

  queue->head = NULL;
  queue->tail = NULL;
}

void set_truck_time(int time) { TRUCK_TIME = time; }

void set_truck_weight(int weight) { TRUCK_WEIGHT = weight; }
// END TRUCK IMPLEMENTATION =========================

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

void handle_stock(StockHT *stock_ht, char *line, WaitingQueue *waiting_queue,
                  TruckQueue *truck_queue) {
  char *command = strtok(line, " ");
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
#ifdef DEBUG
    print_stock_ingredients(stock);
#endif /* ifdef DEBUG */
  }

  printf("rifornito\n");
  check_waiting_orders(waiting_queue, truck_queue, stock_ht);
}

bool send_order(
    StockHT *stock_ht, WaitingQueue *waiting_queue, TruckQueue *truck_queue,
    Order *order,
    bool is_waiting_order) { // TODO: Consider to return a boolean value

  if (!is_waiting_order) {
    order->recipe->n_waiting_orders++;
  }

  Stock **missing_ingredients =
      (Stock **)malloc(order->recipe->n_ingredients * sizeof(Stock *));
  if (missing_ingredients == NULL) {
    printf("Error while allocating missing ingredients\n");
    return false;
  }

  // Check if all ingredients are available
  RecipeIngredient *ingredient = order->recipe->ingredients;
  int n_missing_ingredients = 0;
  while (ingredient != NULL) {
    Stock *stock = stock_ht_get(stock_ht, ingredient->name);
    print_stock_ingredients(stock);

    if (stock != NULL) {
      stock_remove_expired_ingredients(stock, CURR_TIME);
    }

    printf("After removing expired ingredients\n");
    print_stock_ingredients(stock);

    if (ingredient->quantity * order->amount > stock->total_quantity) {
      missing_ingredients[n_missing_ingredients++] = stock;
    }

    // TODO: Check stock == NULL

    ingredient = ingredient->next;
  }

  if (n_missing_ingredients == 0) {
    free(missing_ingredients);

    // Remove the ingredients from the stock
    RecipeIngredient *ingredient = order->recipe->ingredients;
    while (ingredient != NULL) {
      Stock *stock = stock_ht_get(stock_ht, ingredient->name);

      printf("Before removing ingredients\n");
      print_stock_ingredients(stock);
      stock_remove_ingredient(stock, ingredient->quantity * order->amount);

      printf("After removing ingredients\n");
      print_stock_ingredients(stock);
#ifdef DEBUG
      print_stock_ingredients(stock);
#endif /* ifdef DEBUG */
      ingredient = ingredient->next;
    }

    // If the order was arrived from the waiting queue, we should enqueue
    // the order in the truck queue in the right position (by arrival time)
    // (O(n))
    if (is_waiting_order) {
      printf("Name: %s\n", order->recipe->name);
      truck_queue_enqueue_by_arrival_time(truck_queue,
                                          create_truck_node(order));
    } else {
      // Otherwise, we should enqueue the order in the truck queue at the end
      // (O(1))
      printf("Name 2: %s\n", order->recipe->name);
      truck_queue_enqueue(truck_queue, create_truck_node(order));
    }

    return true;
  } else {
    // If not from the waiting queue, we should enqueue the order in the
    // waiting
    if (!is_waiting_order) {
      waiting_queue_enqueue(waiting_queue,
                            create_waiting_node(order, missing_ingredients,
                                                n_missing_ingredients));
    }
    return false;
  }
  return false;
}

void check_waiting_orders(WaitingQueue *waiting_queue, TruckQueue *truck_queue,
                          StockHT *stock_ht) {
  WaitingNode *node = waiting_queue->head;
  WaitingNode *prev_node = NULL;
  while (node != NULL) {
    WaitingNode *next = node->next;
    printf("name: %s, n_missing_ingredients: %d\n", node->order->recipe->name,
           node->n_missing_ingredients);
    // TODO: Check only the missing ingredients
    bool sent =
        send_order(stock_ht, waiting_queue, truck_queue, node->order, true);
    if (sent) {
      if (prev_node == NULL) {
        waiting_queue->head = next;
      } else {
        prev_node->next = next;
      }
    } else {
      prev_node = node;
    }
    node = next;
  }
}

void print_stock_ingredients(Stock *stock) {
  printf("=====================================\n");
  printf("Stock: %s %d\n", stock->name, stock->n_ingredients);
  StockIngredient *ingredient = stock->ingredients;
  while (ingredient != NULL) {
    printf("Ingredient: %d %d\n", ingredient->quantity,
           ingredient->expiration_date);
    ingredient = ingredient->next;
  }
  printf("=====================================\n");
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
  send_order(stock_ht, waiting_queue, truck_queue, order, false);
}

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
      truck_queue_dequeue(truck_queue);
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
        handle_stock(stock_ht, line, waiting_queue, truck_queue);
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

    // Print truck queue
    TruckNode *curr_node = truck_queue->head;
    while (curr_node != NULL) {
      printf("Truck: %s %d\n", curr_node->order->recipe->name,
             curr_node->order->amount);
      curr_node = curr_node->next;
    }

    free(line);
  }

  if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
#ifdef DEBUG
    printf("DEBUG: HANDLE TRUCK\n");
#endif /* ifdef DEBUG */
    truck_queue_dequeue(truck_queue);
  }

  free_recipe_ht(recipe_ht);
  free_stock_ht(stock_ht);
  free_waiting_queue(waiting_queue);
  free_truck_queue(truck_queue);
}
