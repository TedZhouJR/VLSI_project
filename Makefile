CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = 
CXXFLAGS = -std=c++14 -g

SRC_DIR = ./src
TREE_SRC_DIR = $(SRC_DIR)/tree
YAL_SRC_DIR = $(SRC_DIR)/yal
#INCL_DIR_LIST = $(TREE_SRC_DIR) $(YAL_SRC_DIR)

BIN_DIR = ./bin
TREE_BIN_DIR = $(BIN_DIR)/tree
YAL_BIN_DIR = $(BIN_DIR)/yal

TREE_SRC_LIST = $(wildcard $(TREE_SRC_DIR)/*.cpp)
TREE_OBJ_LIST = $(addprefix $(TREE_BIN_DIR)/, $(notdir $(TREE_SRC_LIST:.cpp=.o)))
TREE_TEST_OBJ = $(TREE_BIN_DIR)/test.o

PROG = polish
TARGET = $(BIN_DIR)/$(PROG)
TREE_TEST = $(BIN_DIR)/test_tree

.PHONY: all, clean

all: $(TARGET) $(TREE_TEST)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(TREE_SRC_DIR) -I $(YAL_SRC_DIR) $^ -o $@ 

$(TREE_BIN_DIR)/%.o: $(TREE_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(YAL_SRC_DIR) $^ -o $@ 

$(YAL_BIN_DIR)/module.o: $(YAL_SRC_DIR)/module.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

$(TARGET): $(BIN_DIR)/main.o $(filter-out $(TREE_TEST_OBJ), $(TREE_OBJ_LIST)) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(TREE_TEST): $(TREE_OBJ_LIST) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TREE_BIN_DIR)/*.o
	rm -f $(YAL_BIN_DIR)/*.o
	rm -f $(BIN_DIR)/*.o
	rm -f $(TARGET)
