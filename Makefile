CC := g++
FLAGS := -Wall -std=c++17
DEBUG_FLAGS = -p -g3 -Og
RELEASE_FLAGS = -O3 -flto -DNDEBUG
INCLUDE := -Isrc
LINK :=

CCFLAGS := $(FLAGS) $(INCLUDE)

BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src

TARGET_NAME := riscv-sim
TARGET := $(BIN_PATH)/$(TARGET_NAME)

SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

CLEAN_LIST := $(TARGET) $(OBJ)

default: makedir all

.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH)

.PHONY: all
all: 
	$(CC) -o $(TARGET) $(SRC) $(RELEASE_FLAGS) $(CCFLAGS) $(LINK)

.PHONY: debug
debug: 
	$(CC) -o $(TARGET) $(SRC) $(DEBUG_FLAGS) $(CCFLAGS) $(LINK)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -rf $(CLEAN_LIST)
