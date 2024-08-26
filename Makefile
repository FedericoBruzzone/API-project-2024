# -DDEBUG
CFLAGS += -Wall -Werror -std=gnu11 -O2 -fsanitize=address
LDFLAGS += -fsanitize=address -lm
