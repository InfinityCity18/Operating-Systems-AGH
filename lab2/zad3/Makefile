CC=gcc

AR=ar

RM=rm

BUILD=build

CFLAGS=-c -Wall -O3

LDFLAGS=-lm

ARFLAGS=rcs

CHILD_SRC=child.c
CHILD_OBJ=$(BUILD)/$(CHILD_SRC:.c=.o)
CHILD_TARGET=$(CHILD_SRC:.c=)

MAIN_SRC=main.c
MAIN_OBJ=$(BUILD)/main.o
MAIN_TARGET=main

SIG_LIB_DIR=libsig
SIG_LIB_NAME=libsig
SIG_LIB_NAME_NOPREFIX=sig
SIG_LIB_SRC=$(wildcard $(SIG_LIB_DIR)/*.c)
SIG_LIB_OBJ=$(addprefix $(BUILD)/,$(notdir $(SIG_LIB_SRC:.c=.o)))
SIG_LIB_STATIC=libsig.a
SIG_LIB_SHARED=libsig.so

SOURCES=main.c child.c

EXECS=$(SOURCES:%.c=%)

.PHONY: all
all: static 

static: $(SIG_LIB_OBJ) $(MAIN_TARGET) $(CHILD_OBJ)
	$(AR) $(ARFLAGS) $(SIG_LIB_STATIC) $(SIG_LIB_OBJ)
	$(CC) $> $(CHILD_OBJ) $(SIG_LIB_STATIC) $(LDFLAGS) -o $(CHILD_TARGET)

shared: CFLAGS += -fPIC
shared: $(SIG_LIB_OBJ) $(MAIN_TARGET) $(CHILD_OBJ)
	$(CC) -shared $(LDFLAGS) -o $(SIG_LIB_SHARED) $(SIG_LIB_OBJ)
	$(CC) $(CHILD_OBJ) -L ./ -l$(SIG_LIB_NAME_NOPREFIX) -Wl,-rpath,. -o $(CHILD_TARGET)

dynamic: CFLAGS += -fPIC
dynamic: $(SIG_LIB_OBJ) $(MAIN_TARGET) $(CHILD_OBJ)
	$(CC) -shared $(LDFLAGS) -o $(SIG_LIB_SHARED) $(SIG_LIB_OBJ)
	$(CC) -D USEDLL $(CHILD_OBJ) -o $(CHILD_TARGET)

$(BUILD)/%.o: $(SIG_LIB_DIR)/%.c 
	$(CC) $(CFLAGS) $< -o $@

$(MAIN_OBJ): $(MAIN_SRC) $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(CHILD_OBJ): $(CHILD_SRC) $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(CHILD_TARGET): $(CHILD_OBJ) $(SIG_LIB_STATIC)

$(BUILD):
	mkdir -p build

.PHONY: clean
clean:
	-@ $(RM) $(BUILD)/*.o 
	-@ $(RM) $(EXECS)        
	-@ $(RM) $(SIG_LIB_STATIC) $(SIG_LIB_SHARED)