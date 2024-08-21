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
// ==================================================

// GLOBAL VARIABLES =================================
static int CURR_TIME = 0;
static int TRUCK_TIME = 0;
static int TRUCK_WEIGHT = 0;
// ==================================================

// UTIL =================================
uint32_t hash_string(char *, int);
void increase_curr_time();
char *read_line(FILE *);
// ==================================================

// TRUCK ================================
void set_truck_time(int);
void set_truck_weight(int);
void handle_truck(char *);
// ==================================================

// RECIPE ===============================
typedef struct RecipeIngredient RecipeIngredient;
typedef struct Recipe Recipe;
typedef struct RecipeHT RecipeHT;

RecipeIngredient *create_recipe_ingredient(char *, int);
Recipe *create_recipe(char *);
RecipeHT *create_recipe_ht(int);

void recipe_add_ingredient(Recipe *, RecipeIngredient *);
void add_recipe(RecipeHT *, char *);
Recipe *recipe_ht_get(RecipeHT *, char *);
void recipe_ht_put(RecipeHT *, Recipe *);
double recipe_ht_load_factor(RecipeHT *);
void recipe_ht_resize(RecipeHT *);
// ==================================================

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
// ==================================================

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

RecipeHT *create_recipe_ht(int size) {
  RecipeHT *ht = (RecipeHT *)malloc(sizeof(RecipeHT));
  if (ht == NULL) {
    printf("Error while allocating RecipeHT\n");
    return NULL;
  }

  ht->n_elements = 0;
  ht->size = size;
  ht->recipes = (Recipe **)malloc(size * sizeof(Recipe *));
  for (int i = 0; i < size; i++) {
      ht->recipes[i] = NULL;
  }
  if (ht->recipes == NULL) {
    printf("Error while allocating RecipeHT recipes\n");
    return NULL;
  }

  return ht;
}

void add_recipe(RecipeHT *ht, char *line) {
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

  if (recipe_ht_get(ht, name) != NULL) {
    printf("ingnorato\n");
    return;
  }

  Recipe *recipe = create_recipe(name);
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

// ==================================================

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

int main(void) {
  RecipeHT *ht = create_recipe_ht(HT_INIT_SIZE);

  char command[COMMAND_LEN];
  char *line;

  while ((line = read_line(stdin)) != NULL) {
    if (CURR_TIME != 0 && TRUCK_TIME != 0 && CURR_TIME % TRUCK_TIME == 0) {
      printf("HANDLE TRUCK\n");
    }

    if (sscanf(line, "%s", command) == 1) {
      if (strcmp(command, "aggiungi_ricetta") == 0) {
        add_recipe(ht, line);
        printf("%s\n", line);
        increase_curr_time();
      } else if (strcmp(command, "rimuovi_ricetta") == 0) {
        printf("%s\n", line);
        increase_curr_time();
      } else if (strcmp(command, "rifornimento") == 0) {
        printf("%s\n", line);
        increase_curr_time();
      } else if (strcmp(command, "ordine") == 0) {
        printf("%s\n", line);
        increase_curr_time();
      } else {
        handle_truck(line);
      }
    }

    free(line);
  }
}
