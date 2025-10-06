# make: Default build

# make debug: Debug build with -g

# make release: Release build with -DNDEBUG

# make clean: Clean build artifacts

# Compiler
CC = gcc

# Default compiler flags
CFLAGS = -Wall

# Target executable
TARGET = imagecopy

# Source and object files
SRCS = main.c bmp_file_handler.c image_data_handler.c convolution.c clamp.c reduce_colors_24.c
OBJS = $(SRCS:.c=.o)

# Default build
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

# Release build with assertions disabled
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

# Debug build with symbols
debug: CFLAGS += -g
debug:
ifeq ($(OS),Windows_NT)
	@echo.
	@echo Starting debug build...
	@powershell -Command "$$start = Get-Date; $$null = (make clean); $$null = (make all); if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Path 'bin' } ; Move-Item -Force .\$(TARGET).exe bin\ ; $$end = Get-Date; $$elapsed = ($$end - $$start).TotalSeconds; Write-Host ''; Write-Host 'Debug build complete in' $$elapsed 'seconds.'; Write-Host 'Executable is at bin\\$(TARGET).exe'"
else
	@echo
	@echo Starting debug build...
	@START=$$(date +%s); \
	$(MAKE) --no-print-directory clean; \
	echo Building debug with symbols...; \
	$(MAKE) --no-print-directory all; \
	mkdir -p bin; \
	mv -f $(TARGET) bin/; \
	END=$$(date +%s); \
	echo; echo "Debug build complete in $$((END - START)) seconds."; \
	echo "Executable is at bin/$(TARGET)"
endif
