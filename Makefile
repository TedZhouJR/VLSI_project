CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = 
CXXFLAGS = -std=c++14 -g

SRC_DIR = ./src
POLISH_SRC_DIR = $(SRC_DIR)/polish
YAL_SRC_DIR = $(SRC_DIR)/yal

BIN_DIR = ./bin
POLISH_BIN_DIR = $(BIN_DIR)/polish
YAL_BIN_DIR = $(BIN_DIR)/yal

POLISH_SRC_LIST = $(wildcard $(POLISH_SRC_DIR)/*.cpp)
POLISH_OBJ_LIST = $(addprefix $(POLISH_BIN_DIR)/, $(notdir $(POLISH_SRC_LIST:.cpp=.o)))
POLISH_TEST_OBJ = $(POLISH_BIN_DIR)/test.o

PROG = main
TARGET = $(BIN_DIR)/$(PROG)
POLISH_TEST = $(BIN_DIR)/test_polish

.PHONY: all, clean

all: $(TARGET) $(POLISH_TEST)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(POLISH_SRC_DIR) -I $(YAL_SRC_DIR) $^ -o $@ 

$(POLISH_BIN_DIR)/%.o: $(POLISH_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(YAL_SRC_DIR) $^ -o $@ 

$(YAL_BIN_DIR)/module.o: $(YAL_SRC_DIR)/module.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

$(TARGET): $(BIN_DIR)/main.o $(filter-out $(POLISH_TEST_OBJ), $(POLISH_OBJ_LIST)) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(POLISH_TEST): $(POLISH_OBJ_LIST) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(POLISH_BIN_DIR)/*.o
	rm -f $(YAL_BIN_DIR)/*.o
	rm -f $(BIN_DIR)/*.o
	rm -f $(TARGET)
	rm -f $(POLISH_TEST)
