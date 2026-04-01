CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -lm
SRC     = binary_buddy.c main.c
TARGET  = main

# Default: build and run minimal test (MAX=4, MIN=1)
all: $(TARGET)

$(TARGET): $(SRC) binary_buddy.h buddy_size.h
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET)

# Run the minimal test (MAX_ALLOC_SIZE_BITS=4, MIN_ALLOC_SIZE_BITS=1)
test-minimal: $(TARGET)
	./$(TARGET)

# Run the advanced test (MAX_ALLOC_SIZE_BITS=32, MIN_ALLOC_SIZE_BITS=20)
test-advanced: buddy_size.h
	sed -i 's/^#define MAX_ALLOC_SIZE_BITS.*/#define MAX_ALLOC_SIZE_BITS 32/' buddy_size.h
	sed -i 's/^#define MIN_ALLOC_SIZE_BITS.*/#define MIN_ALLOC_SIZE_BITS 20/' buddy_size.h
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET)
	./$(TARGET)
	sed -i 's/^#define MAX_ALLOC_SIZE_BITS.*/#define MAX_ALLOC_SIZE_BITS 4/' buddy_size.h
	sed -i 's/^#define MIN_ALLOC_SIZE_BITS.*/#define MIN_ALLOC_SIZE_BITS 1/' buddy_size.h

# Build with address sanitizer to catch memory bugs
debug: $(SRC) binary_buddy.h buddy_size.h
	$(CC) $(SRC) $(CFLAGS) -g -fsanitize=address,undefined -o $(TARGET)_debug
	./$(TARGET)_debug

clean:
	rm -f $(TARGET) $(TARGET)_debug

.PHONY: all test-minimal test-advanced debug clean