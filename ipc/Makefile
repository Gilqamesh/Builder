STATIC_LIB = libipc.a
CC         = gcc
CFLAGS     = -Wall -Wextra -Werror -g
AR         = ar
SRC        = mem.c msg.c proc.c seg.c sem.c
OBJ        = $(patsubst %.c,%.o,$(SRC))
DPS        = $(patsubst %.c,%.d,$(SRC))

$(STATIC_LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MP -MF $(<:.c=.d)

.phony: all
all: $(STATIC_LIB)

.phony: clean
clean:
	rm $(OBJ) $(STATIC_LIB)

.phony: re
re: clean
re: all

-include $(DPS)
