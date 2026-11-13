CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDLIBS = -lreadline

TARGET = bin/myshell
SOURCES = src/main.c src/shell.c src/execute.c
OBJECTS = $(SOURCES:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDLIBS)

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o bin/myshell

.PHONY: all clean

