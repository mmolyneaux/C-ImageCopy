# Compiler
CC = gcc

# Compiler flags
#CFLAGS = -Wall

# Target executable
TARGET = imagecopy

# Source files
SRCS = main.c bmp_file_handler.c bmp_file_handler.h image_data_handler.c image_data_handler.h convolution.c convolution.h clamp.c clamp.h

# Object files
OBJS = main.o bmp_file_handler.o image_data_handler.o convolution.o clamp.o

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile main.o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

# Compile bmp_file_handler.o
bmp_file_handler.o: bmp_file_handler.c bmp_file_handler.h
	$(CC) $(CFLAGS) -c bmp_file_handler.c

# Compile image_data_handler.o
image_data_handler.o: image_data_handler.c image_data_handler.h
	$(CC) $(CFLAGS) -c image_data_handler.c

# Compile convolution.o
convolution.o: convolution.c convolution.h
	$(CC) $(CFLAGS) -c convolution.c

# Compile clamp.o
clamp.o: clamp.c clamp.h
	$(CC) $(CFLAGS) -c clamp.c

# Clean up the generated files
# WIN
clean:
	@del /F /Q $(OBJS) $(TARGET) || rm -f $(OBJS) $(TARGET)
