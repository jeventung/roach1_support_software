CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall
#CFLAGS += -DDEBUG
CFLAGS += -ggdb
AR = ar
RM = rm -f

INC = -I../include -I$(KATCP)
LIB = core.a
SRC = core.c misc.c pce.c input.c hwmon.c

OBJ = $(patsubst %.c,%.o,$(SRC))

all: $(LIB) 

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INC)

clean:
	$(RM) $(LIB) *.o core

