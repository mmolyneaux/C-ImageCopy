# Compiler
CC = gcc

# Default compiler flags (debug mode)
CFLAGS = -Wall

# Target executable
TARGET = imagecopy

# Source and object files
SRCS = main.c bmp_file_handler.c image_data_handler.c convolution.c clamp.c reduce_colors_24.c
OBJS = $(SRCS:.c=.o)

# Default debug build
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Generic rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean intermediate and output files
clean:
	@echo Cleaning up...
	@del /F /Q $(OBJS) $(TARGET).exe *.gch *.bak *~ 2>nul || rm -f $(OBJS) $(TARGET) *.gch *.bak *~

# Cross-platform release build with timing and cleanup
release: CFLAGS += -DNDEBUG
release:
ifeq ($(OS),Windows_NT)
	@echo.
	@echo Starting clean release build...
	@powershell -Command "$$start = Get-Date; $$null = (make clean); $$null = (make all); if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Path 'bin' } ; Move-Item -Force .\$(TARGET).exe bin\ ; $$null = (make clean); $$end = Get-Date; $$elapsed = ($$end - $$start).TotalSeconds; Write-Host ''; Write-Host 'Release build complete in' $$elapsed 'seconds.'; Write-Host 'Executable is at bin\\$(TARGET).exe'"
else
	@echo
	@echo Starting clean release build...
	@START=$$(date +%s); \
	$(MAKE) --no-print-directory clean; \
	echo Building release with assertions disabled...; \
	$(MAKE) --no-print-directory all; \
	mkdir -p bin; \
	mv -f $(TARGET) bin/; \
	$(MAKE) --no-print-directory clean; \
	END=$$(date +%s); \
	echo; echo "Release build complete in $$((END - START)) seconds."; \
	echo "Executable is at bin/$(TARGET)"
endif