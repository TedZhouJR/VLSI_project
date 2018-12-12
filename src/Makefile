CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = 
CXXFLAGS = -std=c++14
TARGET = polish

.PHONY: all, clean
all: $(TARGET)

$(TARGET): main.cpp polish_node.cpp polish_tree.cpp yal/module.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)
