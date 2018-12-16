CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = 
CXXFLAGS = -std=c++14 -g

SRC_DIR = ./src
POLISH_SRC_DIR = $(SRC_DIR)/polish
YAL_SRC_DIR = $(SRC_DIR)/yal
SEQPAIR_SRC_DIR = $(SRC_DIR)/seqpair
AURELIANO_SRC_DIR = $(SRC_DIR)/aureliano

BIN_DIR = ./bin
POLISH_BIN_DIR = $(BIN_DIR)/polish
YAL_BIN_DIR = $(BIN_DIR)/yal
SEQPAIR_BIN_DIR = $(BIN_DIR)/seqpair

POLISH_SRC_LIST = $(wildcard $(POLISH_SRC_DIR)/*.cpp)
POLISH_OBJ_LIST = $(addprefix $(POLISH_BIN_DIR)/, $(notdir $(POLISH_SRC_LIST:.cpp=.o)))
POLISH_TEST_OBJ = $(POLISH_BIN_DIR)/test.o

SEQPAIR_SRC_LIST = $(wildcard $(SEQPAIR_SRC_DIR)/*.cpp)
SEQPAIR_OBJ_LIST = $(addprefix $(SEQPAIR_BIN_DIR)/, $(notdir $(SEQPAIR_SRC_LIST:.cpp=.o)))

TARGET = $(BIN_DIR)/main
POLISH_TEST = $(BIN_DIR)/test_polish
SEQPAIR_TARGET = $(BIN_DIR)/seq_pair

TARGET_LIST = $(TARGET) $(POLISH_TEST) $(SEQPAIR_TARGET)

.PHONY: all, clean

all: $(TARGET_LIST)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(POLISH_SRC_DIR) -I $(YAL_SRC_DIR) $^ -o $@ 

$(POLISH_BIN_DIR)/%.o: $(POLISH_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(YAL_SRC_DIR) $^ -o $@ 

$(YAL_BIN_DIR)/module.o: $(YAL_SRC_DIR)/module.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

$(SEQPAIR_BIN_DIR)/%.o: $(SEQPAIR_SRC_DIR)/%.cpp
	$(CC) -DNDEBUG -std=c++14 -O2 -c $^ -I $(AURELIANO_SRC_DIR) -o $@

$(TARGET): $(BIN_DIR)/main.o $(filter-out $(POLISH_TEST_OBJ), $(POLISH_OBJ_LIST)) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(POLISH_TEST): $(POLISH_OBJ_LIST) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -lboost_unit_test_framework -o $@

$(SEQPAIR_TARGET): $(SEQPAIR_OBJ_LIST)
	$(CC) -DNDEBUG -std=c++14 -O2 $^ -o $@

clean:
	rm -f $(POLISH_BIN_DIR)/*.o
	rm -f $(YAL_BIN_DIR)/*.o
	rm -f $(SEQPAIR_BIN_DIR)/*.o
	rm -f $(BIN_DIR)/*.o
	rm -f $(TARGET_LIST)
