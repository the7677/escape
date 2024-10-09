INPUT_FILE  = *.c
OUTPUT_FILE = main
LIB_FLAGS   = -lm -lncurses
ARGV        = ghost

all:
	gcc $(INPUT_FILE) $(LIB_FLAGS) -o $(OUTPUT_FILE)
	./$(OUTPUT_FILE) $(ARGV)
	rm $(OUTPUT_FILE)
