#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINE ===========================================
#define LINE_SIZE 512
#define COMMAND_LEN 17
#define HT_LOAD_FACTOR 0.75
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
void free_recipe_ht();
void recipe_ht_put(Recipe *);
Recipe *recipe_ht_get(char *);
inline void recipe_ht_delete(char *);
double recipe_ht_load_factor();
void recipe_ht_resize();
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
void free_stock_ht();
void stock_ht_put(Stock *);
Stock *stock_ht_get(char *);
double stock_ht_load_factor();
void stock_ht_resize();

inline Stock *stock_get_or_create(char *);
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
void order_queue_enqueue_by_arrival_time(OrderNode *); // Truck queue
void truck_order_queue_dequeue();
// END ORDER ===========================

// UTIL =================================
inline uint32_t hash_string(char *, int);
inline char *read_line(FILE *);

void add_recipe(char *);
void remove_recipe(char *);
void handle_stock(char *);
void handle_order(char *);
void handle_truck(char *);

bool try_send_order(Order *, bool);
bool check_missing_ingredients(Order *, Stock **);
void check_waiting_orders();
void send_order(Order *, bool, Stock **);
// END UTIL =============================

#define MAX_CAKES 5000

typedef struct {
  char cake_name[50];
  int max_orderable_quantity;
} CakeCache;

CakeCache cake_cache[MAX_CAKES];
int cake_cache_count = 0;

int get_cached_quantity(const char *cake_name) {
  for (int i = 0; i < cake_cache_count; ++i) {
    if (strcmp(cake_cache[i].cake_name, cake_name) == 0) {
      return cake_cache[i].max_orderable_quantity;
    }
  }
  return -1; // Non trovato nella cache
}

void update_cache(const char *cake_name, int quantity) {
  for (int i = 0; i < cake_cache_count; ++i) {
    if (strcmp(cake_cache[i].cake_name, cake_name) == 0) {
      cake_cache[i].max_orderable_quantity = quantity;
      return;
    }
  }
  // Aggiungi alla cache se non esiste
  strcpy(cake_cache[cake_cache_count].cake_name, cake_name);
  cake_cache[cake_cache_count].max_orderable_quantity = quantity;
  cake_cache_count++;
}

static RecipeHT *recipe_ht;
static StockHT *stock_ht;
static OrderQueue *waiting_queue;
static OrderQueue *truck_queue;

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

void free_recipe_ht() {
  for (int i = 0; i < recipe_ht->size; i++) {
    Recipe *recipe = recipe_ht->recipes[i];
    while (recipe != NULL) {
      Recipe *next = recipe->next;
      free_recipe(recipe);
      recipe = next;
    }
  }

  free(recipe_ht->recipes);
  free(recipe_ht);
}

inline void recipe_add_ingredient(Recipe *recipe,
                                  RecipeIngredient *ingredient) {
  recipe->weight += ingredient->quantity;
  recipe->n_ingredients++;
  ingredient->next = recipe->ingredients;
  recipe->ingredients = ingredient;
}

Recipe *recipe_ht_get(char *name) {
  uint32_t hash = hash_string(name, recipe_ht->size);
  Recipe *recipe = recipe_ht->recipes[hash];

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

void recipe_ht_put(Recipe *recipe) {
  uint32_t hash = hash_string(recipe->name, recipe_ht->size);
  Recipe *curr_recipe = recipe_ht->recipes[hash];

  if (curr_recipe == NULL) {
    recipe_ht->recipes[hash] = recipe;
    recipe_ht->n_elements++;
    return;
  }
  recipe->next = curr_recipe;
  recipe_ht->recipes[hash] = recipe;
  recipe_ht->n_elements++;

  if (recipe_ht_load_factor() >= HT_LOAD_FACTOR) {
    recipe_ht_resize();
  }
}

void recipe_ht_delete(char *name) {
  uint32_t hash = hash_string(name, recipe_ht->size);
  Recipe *curr_recipe = recipe_ht->recipes[hash];

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
    recipe_ht->recipes[hash] = curr_recipe->next;
  else
    prev_recipe->next = curr_recipe->next;

  curr_recipe->next = NULL;
  free_recipe(curr_recipe);
  recipe_ht->n_elements--;

  printf("rimossa\n");
}

double recipe_ht_load_factor() {
  return (double)recipe_ht->n_elements / recipe_ht->size;
}

void recipe_ht_resize() {
  RecipeHT *ht = recipe_ht;
  recipe_ht = create_recipe_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Recipe *recipe = ht->recipes[i];
    while (recipe != NULL) {
      Recipe *next = recipe->next;
      recipe->next = NULL;
      recipe_ht_put(recipe);
      recipe = next;
    }
  }

  free(ht->recipes);
  free(ht);
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

void free_stock_ht() {
  for (int i = 0; i < stock_ht->size; i++) {
    Stock *stock = stock_ht->stocks[i];
    while (stock != NULL) {
      Stock *next = stock->next;
      free_stock(stock);
      stock = next;
    }
  }

  free(stock_ht->stocks);
  free(stock_ht);
}

void stock_ht_put(Stock *stock) {
  uint32_t hash = hash_string(stock->name, stock_ht->size);
  Stock *curr_stock = stock_ht->stocks[hash];

  if (curr_stock == NULL) {
    stock_ht->stocks[hash] = stock;
    stock->next = NULL;
    stock_ht->n_elements++;
    return;
  }
  stock->next = curr_stock;
  stock_ht->stocks[hash] = stock;
  stock_ht->n_elements++;

  if (stock_ht_load_factor() >= HT_LOAD_FACTOR) {
    stock_ht_resize();
  }
}

inline Stock *stock_ht_get(char *name) {
  uint32_t hash = hash_string(name, stock_ht->size);
  Stock *stock = stock_ht->stocks[hash];

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

inline double stock_ht_load_factor() {
  return (double)stock_ht->n_elements / stock_ht->size;
}

inline void stock_ht_resize() {
  StockHT *ht = stock_ht;
  stock_ht = create_stock_ht(ht->size * 2);

  for (int i = 0; i < ht->size; i++) {
    Stock *stock = ht->stocks[i];
    while (stock != NULL) {
      Stock *next = stock->next;
      stock->next = NULL;
      stock_ht_put(stock);
      stock = next;
    }
  }

  free(ht->stocks);
  free(ht);
}

inline Stock *stock_get_or_create(char *name) {
  Stock *stock = stock_ht_get(name);
  if (stock == NULL) {
    stock = create_stock(name);
    stock_ht_put(stock);
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

void order_queue_enqueue_by_arrival_time(OrderNode *node) {
  if (truck_queue->tail == NULL) {
    truck_queue->head = node;
    truck_queue->tail = node;
    return;
  }

  // Find the correct position to insert the node (by arrival time).
  OrderNode *curr = truck_queue->head;
  OrderNode *prev = NULL;
  while (curr != NULL &&
         curr->order->arrival_time < node->order->arrival_time) {
    prev = curr;
    curr = curr->next;
  }

  if (prev == NULL) {
    node->next = truck_queue->head;
    truck_queue->head = node;
    return;
  }

  prev->next = node;
  node->next = curr;
  if (curr == NULL) {
    truck_queue->tail = node;
  }
}

void truck_order_queue_dequeue() {
  if (truck_queue->head == NULL) {
    printf("camioncino vuoto\n");
    return;
  }

  // Remove order from the list until the truck is full (TRUCK_WEIGHT)
  OrderNode *node = truck_queue->head;
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

  truck_queue->head = node;
  // If the truck_queue is now empty, update the tail to NULL
  if (truck_queue->head == NULL) {
    truck_queue->tail = NULL;
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

void add_recipe(char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }

  char *recipe_name = strtok(NULL, " ");
  if (recipe_name == NULL) {
    return;
  }

  if (recipe_ht_get(recipe_name) != NULL) {
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

  recipe_ht_put(recipe);
  printf("aggiunta\n");
}

void remove_recipe(char *line) {
  char *command = strtok(line, " ");
  if (command == NULL) {
    return;
  }
  char *name = strtok(NULL, " ");
  if (name == NULL) {
    return;
  }

  recipe_ht_delete(name);
}

void handle_stock(char *line) {
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

    Stock *stock = stock_get_or_create(stock_name);

    if (stock == NULL) {
      return;
    }
    stock_add_ingredient(stock, ingredient);
  }

  printf("rifornito\n");
  check_waiting_orders();
}

bool try_send_order(Order *order, bool is_waiting_order) {
  if (!is_waiting_order) {
    order->recipe->n_waiting_orders++;
  }

  Stock *stocks[order->recipe->n_ingredients];
  bool missing_ingredients_flag = check_missing_ingredients(order, stocks);

  if (!missing_ingredients_flag) {
    send_order(order, is_waiting_order, stocks);
    return true;
  } else {
    // If not from the waiting queue, we should enqueue the order in the
    // waiting
    if (!is_waiting_order) {
      order_queue_enqueue(waiting_queue,
                          create_order_node(order)); // TODO REMOVE
    }
    return false;
  }
  return false;
}

void send_order(Order *order, bool is_waiting_order, Stock **stocks) {
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
    order_queue_enqueue_by_arrival_time(node);
  } else {
    // Otherwise, we should enqueue the order in the truck queue at the end
    // (O(1))
    order_queue_enqueue(truck_queue, node);
  }
}

// Check if all ingredients are available
bool check_missing_ingredients(Order *order, Stock **stocks) {
  bool missing_ingredients_flag = false;
  int i = 0;
  RecipeIngredient *ingredient = order->recipe->ingredients;
  while (ingredient != NULL) {
    Stock *stock = stock_ht_get(ingredient->name);

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

// void check_waiting_orders() {
//   OrderNode *node = waiting_queue->head;
//   OrderNode *prev_node = NULL;
//
//   while (node != NULL) {
//     bool sent = try_send_order(node->order, true);
//
//     OrderNode *next = node->next;
//
//     if (sent) {
//       if (prev_node == NULL) {
//         // Node is the head
//         waiting_queue->head = next;
//       } else {
//         prev_node->next = next;
//       }
//
//       if (next == NULL) {
//         // Node is the tail
//         waiting_queue->tail = prev_node;
//       }
//
//       free(node);
//     } else {
//       prev_node = node;
//     }
//     node = next;
//   }
// }

void check_waiting_orders() {
  cake_cache_count = 0;

  OrderNode *node = waiting_queue->head;
  OrderNode *prev_node = NULL;

  while (node != NULL) {
    const char *cake_name = node->order->recipe->name;
    int order_quantity = node->order->amount;

    int cached_quantity = get_cached_quantity(cake_name);
    if (!(cached_quantity != -1 && order_quantity > cached_quantity)) {
      bool sent = try_send_order(node->order, true);

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
        update_cache(cake_name, order_quantity);
        prev_node = node;
      }
      node = next;
    } else {
      prev_node = node;
      node = node->next;
    }
  }
}

void handle_order(char *line) {
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

  Recipe *recipe = recipe_ht_get(recipe_name);
  if (recipe == NULL) {
    printf("rifiutato\n");
    return;
  }
  printf("accettato\n");

  Order *order = create_order(recipe, amount, CURR_TIME);
  try_send_order(order, false);
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
  recipe_ht = create_recipe_ht(HT_INIT_SIZE_RECIPE);
  stock_ht = create_stock_ht(HT_INIT_SIZE_INGREDIENT);
  waiting_queue = create_order_queue();
  truck_queue = create_order_queue();

  char command[4];
  char *line;

  while ((line = read_line(stdin)) != NULL) {
    if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
      truck_order_queue_dequeue();
    }

    if (sscanf(line, "%3s", command) == 1) {
      if (strcmp(command, "agg") == 0) {
        add_recipe(line);
        CURR_TIME++;
      } else if (strcmp(command, "rim") == 0) {
        remove_recipe(line);
        CURR_TIME++;
      } else if (strcmp(command, "rif") == 0) {
        handle_stock(line);
        CURR_TIME++;
      } else if (strcmp(command, "ord") == 0) {
        handle_order(line);
        CURR_TIME++;
      } else {
        handle_truck(line);
      }
    }

    free(line);
  }

  if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
    truck_order_queue_dequeue();
  }

  free_recipe_ht();
  free_stock_ht();
  free_order_queue(waiting_queue);
  free_order_queue(truck_queue);
}
