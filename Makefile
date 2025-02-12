# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall

# Target executable
TARGET = imagecopy

# Source files
SRCS = main.c bitmap.c bitmap.h

# Object files
OBJS = main.o bitmap.o

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile main.o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

# Compile bitmap.o
bitmap.o: bitmap.c bitmap.h
	$(CC) $(CFLAGS) -c bitmap.c

# Clean up the generated files
# WIN
clean:
	del /F /Q $(OBJS) $(TARGET)

# UNIX
#clean:
#	rm -f $(OBJS) $(TARGET)