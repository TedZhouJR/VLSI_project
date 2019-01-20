CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = -DNDEBUG
CXXFLAGS = -std=c++14 -O2

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

YAL_SRC_LIST := $(wildcard $(YAL_SRC_DIR)/*.cpp)
YAL_SRC_LIST := $(filter-out $(YAL_SRC_DIR)/scanner.cpp, $(YAL_SRC_LIST))
YAL_SRC_LIST := $(filter-out $(YAL_SRC_DIR)/parser.cpp, $(YAL_SRC_LIST))
YAL_SRC_LIST += $(YAL_SRC_DIR)/scanner.cpp
YAL_SRC_LIST += $(YAL_SRC_DIR)/parser.cpp
YAL_OBJ_LIST = $(addprefix $(YAL_BIN_DIR)/, $(notdir $(YAL_SRC_LIST:.cpp=.o)))
YAL_MAIN_OBJ = $(YAL_BIN_DIR)/main.o

YAL_SRC_TMP_LIST := parser.cpp parser.hpp location.hh position.hh stack.hh scanner.cpp
YAL_SRC_TMP_LIST := $(addprefix $(YAL_SRC_DIR)/, $(YAL_SRC_TMP_LIST))

SEQPAIR_SRC_LIST = $(wildcard $(SEQPAIR_SRC_DIR)/*.cpp)
SEQPAIR_OBJ_LIST = $(addprefix $(SEQPAIR_BIN_DIR)/, $(notdir $(SEQPAIR_SRC_LIST:.cpp=.o)))
SEQPAIR_MAIN_OBJ = $(SEQPAIR_BIN_DIR)/run_packer.o

TARGET = $(BIN_DIR)/main
POLISH_TEST = $(BIN_DIR)/test_polish
YAL_TARGET = $(BIN_DIR)/interpreter
SEQPAIR_TARGET = $(BIN_DIR)/seq_pair

TARGET_LIST = $(TARGET) $(POLISH_TEST) $(YAL_TARGET) $(SEQPAIR_TARGET)

.PHONY: lexyacc, all, clean

all: lexyacc, $(TARGET_LIST)

lexyacc: $(YAL_SRC_DIR)/scanner.cpp $(YAL_SRC_DIR)/parser.cpp

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(AURELIANO_SRC_DIR) \
	-I $(POLISH_SRC_DIR) -I $(YAL_SRC_DIR) -I $(SEQPAIR_SRC_DIR) $^ -o $@ 

$(POLISH_BIN_DIR)/%.o: $(POLISH_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c -I $(YAL_SRC_DIR) -I $(AURELIANO_SRC_DIR) $^ -o $@ 

$(YAL_BIN_DIR)/%.o: $(YAL_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

$(SEQPAIR_BIN_DIR)/%.o: $(SEQPAIR_SRC_DIR)/%.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -I $(AURELIANO_SRC_DIR) -I $(YAL_SRC_DIR) -o $@

$(YAL_SRC_DIR)/scanner.cpp: $(YAL_SRC_DIR)/scanner.l
	flex -o $@ $<

$(YAL_SRC_DIR)/parser.cpp: $(YAL_SRC_DIR)/parser.y
	bison -o $@ -Wno-other $<

$(TARGET): lexyacc $(BIN_DIR)/main.o $(filter-out $(POLISH_TEST_OBJ), \
$(POLISH_OBJ_LIST)) $(filter-out $(YAL_MAIN_OBJ), $(YAL_OBJ_LIST)) \
$(filter-out $(SEQPAIR_MAIN_OBJ), $(SEQPAIR_OBJ_LIST))
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $(filter-out lexyacc, $^) -lboost_program_options -o $@

$(POLISH_TEST): $(POLISH_OBJ_LIST) $(YAL_BIN_DIR)/module.o
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -lboost_unit_test_framework -o $@

$(YAL_TARGET): lexyacc $(YAL_OBJ_LIST)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $(YAL_OBJ_LIST) -o $@

$(SEQPAIR_TARGET): $(SEQPAIR_OBJ_LIST) $(filter-out $(YAL_MAIN_OBJ), $(YAL_OBJ_LIST))
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -lboost_program_options -o $@

lexyacc: $(YAL_SRC_DIR)/scanner.cpp $(YAL_SRC_DIR)/parser.cpp

clean:
	rm -f $(POLISH_BIN_DIR)/*.o
	rm -f $(YAL_BIN_DIR)/*.o
	rm -f $(SEQPAIR_BIN_DIR)/*.o
	rm -f $(BIN_DIR)/*.o
	rm -f $(TARGET_LIST)
	rm -f $(YAL_SRC_TMP_LIST)

