#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINE ===========================================
#define LINE_SIZE 512
#define COMMAND_LEN 17
#define HT_LOAD_FACTOR 0.70
#define HT_INIT_SIZE_RECIPE 1024
#define HT_INIT_SIZE_INGREDIENT 1024
// END DEFINE =======================================

// GLOBAL VARIABLES =================================
static int CURR_TIME = 0;
static int TRUCK_TIME = 0;
static int TRUCK_WEIGHT = 0;
// END GLOBAL VARIABLES =============================

// RECIPE ==================
typedef struct RecipeIngredient RecipeIngredient;
RecipeIngredient *create_recipe_ingredient(char *, int);
inline void free_recipe_ingredient(RecipeIngredient *);

typedef struct Recipe Recipe;
inline Recipe *create_recipe(char *);
inline void free_recipe(Recipe *);
inline void recipe_add_ingredient(Recipe *, RecipeIngredient *);

typedef struct RecipeHT RecipeHT;
RecipeHT *create_recipe_ht(int);
void free_recipe_ht(RecipeHT *);
inline void recipe_ht_put(RecipeHT *, Recipe *);
inline Recipe *recipe_ht_get(RecipeHT *, char *);
inline void recipe_ht_delete(RecipeHT *, char *);
inline double recipe_ht_load_factor(RecipeHT *);
void recipe_ht_resize(RecipeHT *);
// END RECIPE ===========================

// STOCK ============================
typedef struct StockIngredient StockIngredient;
typedef struct Stock Stock; // of an ingredient
typedef struct StockHT StockHT;

inline StockIngredient *create_stock_ingredient(int, int);
inline void free_stock_ingredient(StockIngredient *);

inline Stock *create_stock(char *);
inline void free_stock(Stock *);
inline void stock_add_ingredient(Stock *, StockIngredient *);
inline void stock_remove_expired_ingredients(Stock *, int);
inline void stock_remove_ingredient(Stock *, int);

StockHT *create_stock_ht(int);
void free_stock_ht(StockHT *);
void stock_ht_put(StockHT *, Stock *);
inline Stock *stock_ht_get(StockHT *, char *);
inline double stock_ht_load_factor(StockHT *);
inline void stock_ht_resize(StockHT *);

inline Stock *stock_get_or_create(StockHT *, char *);
// END STOCK ============================

// ORDER ===============================
typedef struct Order Order;
inline Order *create_order(Recipe *, int, int);
inline void free_order(Order *);

typedef struct OrderNode OrderNode;
inline OrderNode *create_order_node(Order *);
inline void free_order_node(OrderNode *);
inline void order_node_enqueue_by_weight(OrderNode **, OrderNode *);

typedef struct OrderQueue OrderQueue;
OrderQueue *create_order_queue();
void free_order_queue(OrderQueue *);
inline void order_queue_enqueue(OrderQueue *, OrderNode *);
inline void order_queue_enqueue_by_arrival_time(OrderQueue *, OrderNode *);
void order_queue_dequeue(OrderQueue *);
// END ORDER ===========================

// UTIL =================================
inline uint32_t hash_string(char *, int);
inline char *read_line(FILE *);

void add_recipe(RecipeHT *, char *);
void remove_recipe(RecipeHT *, char *);
void handle_stock(StockHT *, char *, OrderQueue *, OrderQueue *);
void handle_order(RecipeHT *, StockHT *, OrderQueue *, OrderQueue *, char *);
void handle_truck(char *);

inline bool try_send_order(StockHT *, OrderQueue *, OrderQueue *, Order *,
                           bool);
bool check_missing_ingredients(StockHT *, Order *, Stock **);
inline void check_waiting_orders(OrderQueue *, OrderQueue *, StockHT *);
inline void send_order(StockHT *, Order *, bool, OrderQueue *, Stock **);
// END UTIL =============================

// RECIPE IMPLEMENTATION ============================
struct RecipeIngredient {
  char *name;
  int quantity;
  RecipeIngredient *next;
};

struct Recipe {
  char *name;
  int weight;
  int n_ingredients;
  RecipeIngredient *ingredients; // TODO: Pointer to the stock ingredient
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
    return NULL;
  }

  ingredient->name = (char *)malloc(strlen(name) + 1);
  if (ingredient->name == NULL) {
    free(ingredient);
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
    free(ingredient->name);
    free(ingredient);
    ingredient = next;
  }
}

inline Recipe *create_recipe(char *name) {
  Recipe *recipe = (Recipe *)malloc(sizeof(Recipe));
  if (recipe == NULL) {
    return NULL;
  }

  recipe->name = (char *)malloc(strlen(name) + 1);
  if (recipe->name == NULL) {
    free(recipe);
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

inline void free_recipe(Recipe *recipe) {
  free_recipe_ingredient(recipe->ingredients);
  free(recipe->name);
  free(recipe);
}

RecipeHT *create_recipe_ht(int size) {
  RecipeHT *ht = (RecipeHT *)malloc(sizeof(RecipeHT));
  if (ht == NULL) {
    return NULL;
  }

  ht->n_elements = 0;
  ht->size = size;
  ht->recipes = (Recipe **)malloc(size * sizeof(Recipe *));
  if (ht->recipes == NULL) {
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

inline void recipe_add_ingredient(Recipe *recipe,
                                  RecipeIngredient *ingredient) {
  recipe->weight += ingredient->quantity;
  recipe->n_ingredients++;
  ingredient->next = recipe->ingredients;
  recipe->ingredients = ingredient;
}

inline Recipe *recipe_ht_get(RecipeHT *ht, char *name) {
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

inline void recipe_ht_put(RecipeHT *ht, Recipe *recipe) {
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

  if (curr_recipe == NULL) {
    printf("non presente\n");
    return;
  }
  // printf("curr_recipe->n_waiting_orders: %d\n",
  // curr_recipe->n_waiting_orders);

  Recipe *prev_recipe = NULL;

  while (curr_recipe != NULL && strcmp(curr_recipe->name, name) != 0) {
    prev_recipe = curr_recipe;
    curr_recipe = curr_recipe->next;
  }

  if (curr_recipe == NULL) {
    printf("non presente\n");
    return;
  }

  // Check if there are waiting orders with this recipe
  if (curr_recipe->n_waiting_orders > 0) {
    printf("ordini in sospeso\n");
    return;
  }

  if (prev_recipe == NULL)
    ht->recipes[hash] = curr_recipe->next;
  else
    prev_recipe->next = curr_recipe->next;

  curr_recipe->next = NULL;
  free_recipe(curr_recipe);
  ht->n_elements--;

  printf("rimossa\n");
}

inline double recipe_ht_load_factor(RecipeHT *ht) {
  return (double)ht->n_elements / ht->size;
}

void recipe_ht_resize(RecipeHT *ht) {
  RecipeHT *new_ht = create_recipe_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Recipe *recipe = ht->recipes[i];
    while (recipe != NULL) {
      Recipe *next = recipe->next;
      recipe->next = NULL;
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
  char *name;
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

inline StockIngredient *create_stock_ingredient(int quantity,
                                                int expiration_date) {
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

inline void free_stock_ingredient(StockIngredient *ingredient) {
  while (ingredient != NULL) {
    StockIngredient *next = ingredient->next;
    free(ingredient);
    ingredient = next;
  }
}

inline void stock_add_ingredient(Stock *stock, StockIngredient *ingredient) {
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

inline Stock *create_stock(char *name) {
  Stock *stock = (Stock *)malloc(sizeof(Stock));
  if (stock == NULL) {
    return NULL;
  }

  stock->name = (char *)malloc(strlen(name) + 1);
  if (stock->name == NULL) {
    free(stock);
    return NULL;
  }

  strcpy(stock->name, name);
  stock->n_ingredients = 0;
  stock->total_quantity = 0;
  stock->ingredients = NULL;
  stock->next = NULL;

  return stock;
}

inline void stock_remove_expired_ingredients(Stock *stock, int curr_time) {
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

inline void stock_remove_ingredient(Stock *stock, int quantity) {
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
  free(stock->name);
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
    stock->next = NULL;
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

inline Stock *stock_ht_get(StockHT *ht, char *name) {
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

inline double stock_ht_load_factor(StockHT *ht) {
  return (double)ht->n_elements / ht->size;
}

inline void stock_ht_resize(StockHT *ht) {
  StockHT *new_ht = create_stock_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Stock *stock = ht->stocks[i];
    while (stock != NULL) {
      Stock *next = stock->next;
      stock->next = NULL;
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

inline Stock *stock_get_or_create(StockHT *ht, char *name) {
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

inline Order *create_order(Recipe *recipe, int amount, int arrival_time) {
  Order *order = (Order *)malloc(sizeof(Order));
  if (order == NULL) {
    return NULL;
  }

  order->recipe = recipe;
  order->amount = amount;
  order->arrival_time = arrival_time;
  order->total_weight = recipe->weight * amount;

  return order;
}

inline void free_order(Order *order) { free(order); }

struct OrderNode {
  Order *order;
  OrderNode *next;
};

inline OrderNode *create_order_node(Order *order) {

  OrderNode *node = (OrderNode *)malloc(sizeof(OrderNode));
  if (node == NULL) {
    return NULL;
  }

  node->order = order;
  node->next = NULL;

  return node;
}

inline void free_order_node(OrderNode *node) {
  free_order(node->order);
  free(node);
}

struct OrderQueue {
  OrderNode *head;
  OrderNode *tail;
};

OrderQueue *create_order_queue() {
  OrderQueue *queue = (OrderQueue *)malloc(sizeof(OrderQueue));
  if (queue == NULL) {
    return NULL;
  }

  queue->head = NULL;
  queue->tail = NULL;
  return queue;
}

void free_order_queue(OrderQueue *queue) {
  OrderNode *node = queue->head;
  while (node != NULL) {
    OrderNode *next = node->next;
    free_order_node(node);
    node = next;
  }
  free(queue);
}

inline void order_queue_enqueue(OrderQueue *queue, OrderNode *node) {
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
    return;
  }

  queue->tail->next = node;
  queue->tail = node;
}

inline void order_node_enqueue_by_weight(OrderNode **list, OrderNode *node) {
  // Add node in decreasing order according to the weight in the list,
  // if the weight is the same, order by arrival time
  if (*list == NULL) {
    *list = node;
    return;
  }

  OrderNode *curr = *list;
  OrderNode *prev = NULL;
  while (curr != NULL &&
         curr->order->total_weight > node->order->total_weight) {
    prev = curr;
    curr = curr->next;
  }

  // Find the correct position to insert the node (by arrival time).
  while (curr != NULL &&
         curr->order->total_weight == node->order->total_weight &&
         curr->order->arrival_time < node->order->arrival_time) {
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
// END ORDER IMPLEMENTATION ===========================

// TRUCK IMPLEMENTATION ==============================

inline void order_queue_enqueue_by_arrival_time(OrderQueue *queue,
                                                OrderNode *node) {
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
    return;
  }

  // Find the correct position to insert the node (by arrival time).
  OrderNode *curr = queue->head;
  OrderNode *prev = NULL;
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
  if (curr == NULL) {
    queue->tail = node;
  }
}

void order_queue_dequeue(OrderQueue *queue) {
  if (queue->head == NULL) {
    printf("camioncino vuoto\n");
    return;
  }

  // Remove order from the list until the truck is full (TRUCK_WEIGHT)
  OrderNode *node = queue->head;
  int tmp_weight = 0;
  int n_orders = 0;
  OrderNode *orders = NULL;

  while (node != NULL &&
         tmp_weight + node->order->total_weight <= TRUCK_WEIGHT) {
    tmp_weight += node->order->total_weight;
    n_orders++;

    OrderNode *next = node->next;
    node->next = NULL;
    order_node_enqueue_by_weight(&orders, node);
    node = next;
  }

  if (n_orders == 0) {
    printf("camioncino vuoto\n");
    return;
  }

  OrderNode *curr_order = orders;
  for (int i = 0; i < n_orders; i++) {
    printf("%d %s %d\n", curr_order->order->arrival_time,
           curr_order->order->recipe->name, curr_order->order->amount);
    curr_order->order->recipe->n_waiting_orders--;
    OrderNode *next = curr_order->next;
    free_order_node(curr_order);
    curr_order = next;
  }

  queue->head = node;
  // If the queue is now empty, update the tail to NULL
  if (queue->head == NULL) {
    queue->tail = NULL;
  }
}

void set_truck_time(int time) { TRUCK_TIME = time; }

void set_truck_weight(int weight) { TRUCK_WEIGHT = weight; }
// END TRUCK IMPLEMENTATION =========================

// UTIL IMPLEMENTATION ==============================
inline uint32_t hash_string(char *str, int size) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash % size;
}

inline char *read_line(FILE *stream) {
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  nread = getline(&line, &len, stream);

  if (nread == -1) {
    free(line);
    return NULL;
  }

  if (nread > 0 && line[nread - 1] == '\n') {
    line[nread - 1] = '\0';
  }

  return line;
}

void add_recipe(RecipeHT *ht, char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }

  char *recipe_name = strtok(NULL, " ");
  if (recipe_name == NULL) {
    return;
  }

  if (recipe_ht_get(ht, recipe_name) != NULL) {
    printf("ignorato\n");
    return;
  }

  Recipe *recipe = create_recipe(recipe_name);
  char *ingredient_name;
  while ((ingredient_name = strtok(NULL, " ")) != NULL) {
    char *ingredient_quantity = strtok(NULL, " ");
    if (ingredient_quantity == NULL) {
      return;
    }

    // Add ingredient to the StockHT
    recipe_add_ingredient(
        recipe,
        create_recipe_ingredient(ingredient_name, atoi(ingredient_quantity)));
  }

  recipe_ht_put(ht, recipe);
  printf("aggiunta\n");
}

void remove_recipe(RecipeHT *ht, char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }
  char *name = strtok(NULL, " ");
  if (name == NULL) {
    return;
  }

  recipe_ht_delete(ht, name);
}

void handle_stock(StockHT *stock_ht, char *line, OrderQueue *waiting_queue,
                  OrderQueue *truck_queue) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }
  char *stock_name;
  while ((stock_name = strtok(NULL, " ")) != NULL) {
    char *quantity_str = strtok(NULL, " ");
    if (quantity_str == NULL) {
      return;
    }
    int quantity = atoi(quantity_str);

    char *expiration_date_str = strtok(NULL, " ");
    if (expiration_date_str == NULL) {
      return;
    }
    int expiration_date = atoi(expiration_date_str);

    StockIngredient *ingredient =
        create_stock_ingredient(quantity, expiration_date);

    Stock *stock = stock_get_or_create(stock_ht, stock_name);

    if (stock == NULL) {
      return;
    }
    stock_add_ingredient(stock, ingredient);
  }

  printf("rifornito\n");
  check_waiting_orders(waiting_queue, truck_queue, stock_ht);
}

inline bool try_send_order(StockHT *stock_ht, OrderQueue *waiting_queue,
                           OrderQueue *truck_queue, Order *order,
                           bool is_waiting_order) {
  if (!is_waiting_order) {
    order->recipe->n_waiting_orders++;
  }

  Stock *stocks[order->recipe->n_ingredients];
  bool missing_ingredients_flag =
      check_missing_ingredients(stock_ht, order, stocks);

  if (!missing_ingredients_flag) {
    send_order(stock_ht, order, is_waiting_order, truck_queue, stocks);
    return true;
  } else {
    // If not from the waiting queue, we should enqueue the order in the
    // waiting
    if (!is_waiting_order) {
      order_queue_enqueue(waiting_queue, create_order_node(order));
    }
    return false;
  }
  return false;
}

inline void send_order(StockHT *stock_ht, Order *order, bool is_waiting_order,
                       OrderQueue *truck_queue, Stock **stocks) {
  // Remove the ingredients from the stock
  RecipeIngredient *ingredient = order->recipe->ingredients;
  int i = 0;
  while (ingredient != NULL) {
    stock_remove_ingredient(stocks[i++], ingredient->quantity * order->amount);
    ingredient = ingredient->next;
  }

  OrderNode *node = create_order_node(order);
  if (node == NULL) {
    return;
  }

  // If the order was arrived from the waiting queue, we should enqueue
  // the order in the truck queue in the right position (by arrival time)
  // (O(n))
  if (is_waiting_order) {
    order_queue_enqueue_by_arrival_time(truck_queue, node);
  } else {
    // Otherwise, we should enqueue the order in the truck queue at the end
    // (O(1))
    order_queue_enqueue(truck_queue, node);
  }
}

// Check if all ingredients are available
bool check_missing_ingredients(StockHT *stock_ht, Order *order,
                               Stock **stocks) {
  bool missing_ingredients_flag = false;
  int i = 0;
  RecipeIngredient *ingredient = order->recipe->ingredients;
  while (ingredient != NULL) {
    Stock *stock = stock_ht_get(stock_ht, ingredient->name);

    if (stock != NULL) {
      stock_remove_expired_ingredients(stock, CURR_TIME);
    }

    if (stock == NULL ||
        ingredient->quantity * order->amount > stock->total_quantity) {
      missing_ingredients_flag = true;
      break;
    }

    stocks[i++] = stock;
    ingredient = ingredient->next;
  }
  return missing_ingredients_flag;
}

struct OrderCacheHT {
  OrderNode **buckets;
  int size;
  int n_elements;
};

double order_cache_ht_load_factor(OrderCacheHT *cache) {
  return (double)cache->n_elements / cache->size;
}

OrderCacheHT *create_order_cache_ht(int size) {
  OrderCacheHT *cache = (OrderCacheHT *)malloc(sizeof(OrderCacheHT));
  if (cache == NULL) {
    return NULL;
  }

  cache->size = size;
  cache->n_elements = 0;
  cache->buckets = (OrderNode **)malloc(cache->size * sizeof(OrderNode *));
  if (cache->buckets == NULL) {
    return NULL;
  }
  for (int i = 0; i < cache->size; i++) {
    cache->buckets[i] = NULL;
  }

  return cache;
}

void order_cache_ht_add(OrderCacheHT *cache, OrderNode *node) {
  uint32_t hash = hash_string(node->order->recipe->name, cache->size);
  OrderNode *curr_node = cache->buckets[hash];
  OrderNode *prev_node = NULL;

  while (curr_node != NULL) {
    // Check if a node with the same recipe name already exists
    if (strcmp(curr_node->order->recipe->name, node->order->recipe->name) ==
        0) {
      if (curr_node->order->amount > node->order->amount) {
        // Replace the existing node if the new one has a lesser amount
        if (prev_node == NULL) {
          cache->buckets[hash] = node;
        } else {
          prev_node->next = node;
        }
        node->next = curr_node->next;
        free(curr_node); // Free the old node
        cache->n_elements++;
      }
      return;
    }
    prev_node = curr_node;
    curr_node = curr_node->next;
  }

  // If no existing node with the same recipe name was found, add the new node
  if (prev_node == NULL) {
    // No nodes in the bucket, just add the new node
    cache->buckets[hash] = node;
  } else {
    // Add the new node at the end of the list
    prev_node->next = node;
  }
  node->next = NULL;
  cache->n_elements++;

  // Check if resizing the hash table is needed
  if (order_cache_ht_load_factor(cache) >= HT_LOAD_FACTOR) {
    order_cache_ht_resize(cache);
  }
}

bool order_cache_ht_contains(OrderCacheHT *cache, OrderNode *node) {
  uint32_t hash = hash_string(node->order->recipe->name, cache->size);
  OrderNode *curr_node = cache->buckets[hash];

  if (curr_node == NULL) {
    return false;
  }

  while (curr_node != NULL) {
    if (strcmp(curr_node->order->recipe->name, node->order->recipe->name) ==
            0 &&
        curr_node->order->amount >= node->order->amount) {
      return true;
    }
    curr_node = curr_node->next;
  }

  return false;
}

void order_cache_ht_resize(OrderCacheHT *cache) {
  OrderCacheHT *new_cache = create_order_cache_ht(cache->size * 2);

  for (int i = 0; i < cache->size; i++) {
    OrderNode *node = cache->buckets[i];
    while (node != NULL) {
      OrderNode *next = node->next;
      node->next = NULL;
      order_cache_ht_add(new_cache, node);
      node = next;
    }
  }

  free(cache->buckets);
  cache->buckets = new_cache->buckets;
  cache->size = new_cache->size;
  cache->n_elements = new_cache->n_elements;
  free(new_cache);
}

void free_order_cache_ht(OrderCacheHT *cache) {
  for (int i = 0; i < cache->size; i++) {
    OrderNode *node = cache->buckets[i];
    while (node != NULL) {
      OrderNode *next = node->next;
      free(node);
      node = next;
    }
  }

  free(cache->buckets);
  free(cache);
}

// TODO: Implement cache, we do not need to check ("pere", 2) if we already
// checked ("pere", 2) and it failed
inline void check_waiting_orders(OrderQueue *waiting_queue,
                                 OrderQueue *truck_queue, StockHT *stock_ht) {
  OrderNode *node = waiting_queue->head;
  OrderNode *prev_node = NULL;

  // Initialize the cache
  // OrderCacheHT *cache = create_order_cache_ht(HT_INIT_SIZE_RECIPE);

  // && node->order->arrival_time <= CURR_TIME) {
  while (node != NULL) {

    // if (order_cache_ht_contains(cache, create_order_node(node->order))) {
    //   prev_node = node;
    //   node = node->next;
    //   continue;
    // }

    bool sent =
        try_send_order(stock_ht, waiting_queue, truck_queue, node->order, true);

    OrderNode *next = node->next;

    if (sent) {
      if (prev_node == NULL) {
        // Node is the head
        waiting_queue->head = next;
      } else {
        prev_node->next = next;
      }

      if (next == NULL) {
        // Node is the tail
        waiting_queue->tail = prev_node;
      }

      free(node);
    } else {
      // order_cache_ht_add(cache, create_order_node(node->order));
      prev_node = node;
    }
    node = next;
  }

  // free_order_cache_ht(cache);
}

void handle_order(RecipeHT *recipe_ht, StockHT *stock_ht,
                  OrderQueue *waiting_queue, OrderQueue *truck_queue,
                  char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }

  char *recipe_name = strtok(NULL, " ");
  if (recipe_name == NULL) {
    return;
  }

  char *amount_str = strtok(NULL, " ");
  if (amount_str == NULL) {
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
  try_send_order(stock_ht, waiting_queue, truck_queue, order, false);
}

void handle_truck(char *line) {
  char *time = strtok(line, " ");
  if (time == NULL) {
    return;
  }
  char *weight = strtok(NULL, " ");
  if (weight == NULL) {
    return;
  }
  TRUCK_TIME = atoi(time);
  TRUCK_WEIGHT = atoi(weight);
}
// END UTIL IMPLEMENTATION ==========================

int main(void) {
  RecipeHT *recipe_ht = create_recipe_ht(HT_INIT_SIZE_RECIPE);
  StockHT *stock_ht = create_stock_ht(HT_INIT_SIZE_INGREDIENT);
  OrderQueue *waiting_queue = create_order_queue();
  OrderQueue *truck_queue = create_order_queue();

  char command[4];
  char *line;

  while ((line = read_line(stdin)) != NULL) {
    if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
      order_queue_dequeue(truck_queue);
    }

    if (sscanf(line, "%3s", command) == 1) {
      if (strcmp(command, "agg") == 0) {
        add_recipe(recipe_ht, line);
        CURR_TIME++;
      } else if (strcmp(command, "rim") == 0) {
        remove_recipe(recipe_ht, line);
        CURR_TIME++;
      } else if (strcmp(command, "rif") == 0) {
        handle_stock(stock_ht, line, waiting_queue, truck_queue);
        CURR_TIME++;
      } else if (strcmp(command, "ord") == 0) {
        handle_order(recipe_ht, stock_ht, waiting_queue, truck_queue, line);
        CURR_TIME++;
      } else {
        handle_truck(line);
      }
    }

    free(line);
  }

  if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
    order_queue_dequeue(truck_queue);
  }

  free_recipe_ht(recipe_ht);
  free_stock_ht(stock_ht);
  free_order_queue(waiting_queue);
  free_order_queue(truck_queue);
}
