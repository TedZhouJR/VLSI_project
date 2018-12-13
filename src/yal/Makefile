CROSS_COMPILE = 
CC = $(CROSS_COMPILE)g++
CPPFLAGS = 
CXXFLAGS = -std=c++14
TARGET = interpreter

.PHONY: all, clean
$(TARGET): main.cpp interpreter.cpp module.cpp scanner.cpp parser.cpp
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

scanner.cpp: scanner.l
	flex -o $@ $<

parser.cpp: parser.y
	bison -o $@ -r all -Wno-other $<

clean:
	rm -f scanner.cpp
	rm -f parser.cpp parser.hpp location.hh position.hh stack.hh
	rm -f $(TARGET)
	rm -f *.output
