CFLAGS += -Wall -Werror -std=gnu11 -O2 -fsanitize=address -DDEBUG
LDFLAGS += -fsanitize=address -lm
