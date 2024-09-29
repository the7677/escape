INPUT_FILE  = *.c
OUTPUT_FILE = main
LIB_FLAGS   = -lncurses

all:
	gcc $(INPUT_FILE) $(LIB_FLAGS) -o $(OUTPUT_FILE)
	./$(OUTPUT_FILE)
	rm $(OUTPUT_FILE)
