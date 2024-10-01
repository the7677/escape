INPUT_FILE  = *.c
OUTPUT_FILE = main
LIB_FLAGS   = -lm -lncurses
CC          = gcc
MINGW_CC    = i686-w64-mingw32-gcc
ARGV        = ward
OS          = $(shell uname)

ifeq ($(OS), Windows_NT)
	OUTPUT_FILE = main.exe
	LIB_FLAGS   = -lm -lpdcurses
endif

linux_comp:
	gcc $(INPUT_FILE) $(LIB_FLAGS) -o $(OUTPUT_FILE)
	./$(OUTPUT_FILE) $(ARGV)
	rm $(OUTPUT_FILE)

linux_win_comp:
	i686-w64-mingw32-gcc $(INPUT_FILE) -lm -lpdcurses -o $(OUTPUT_FILE).exe
	./$(OUTPUT_FILE).exe $(ARGV)
	rm $(OUTPUT_FILE).exe

win_comp:
	gcc $(INPUT_FILE) $(LIB_FLAGS) -o $(OUTPUT_FILE)
	./$(OUTPUT_FILE) $(ARGV)
	rm $(OUTPUT_FILE)

clean:
	rm -f $(OUTPUT_FILE) $(OUTPUT_FILE).exe
