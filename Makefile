CFLAGS=-Wall -Wextra
CC=gcc
CC_CFLAGS=$(CC) $(CFLAGS)
OUTPUT_DIR=build/lib
TEST_DIR=build/test
all:
	mkdir -p $(OUTPUT_DIR)
	$(CC_CFLAGS) -c gc.c -o $(OUTPUT_DIR)/gc.o
	ar rcs $(OUTPUT_DIR)/libgc.a $(OUTPUT_DIR)/gc.o

test:
	mkdir -p $(TEST_DIR)
	$(CC_CFLAGS) -c test.c -o $(TEST_DIR)/test.o
	$(CC) $(TEST_DIR)/test.o -L$(OUTPUT_DIR) -lgc -o $(TEST_DIR)/test
	./$(TEST_DIR)/test && valgrind ./$(TEST_DIR)/test

clean:
	rm {$(OUTPUT_DIR),$(TEST_DIR)}/*.o
	rm $(TEST_DIR)/test


