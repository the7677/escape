#include <stdlib.h>
#include <stdbool.h>
#include <ncurses.h>

// macro for dynamic allocation
#define new(T)              (T*)malloc(sizeof(T))
#define new2(T, rows, cols) (T**)malloc(rows * sizeof(T*) + (sizeof(T) * rows * cols))

// --- ENGINE ---------------------------------------

// Enums && Structs
typedef struct {
	int x, y;
} Point;

typedef struct {
	Point pos;
	char ch;
} Entity;

enum TileID {
	WALL,
	GROUND,
	DOOR,
	KEYLOCKED_DOOR,
	BLANK
};

typedef struct {
	enum TileID id;
	char ch;
	bool walkable;
} Tile;

enum Signal {
	BREAK     = 0b00000001,
	NEXT_TURN = 0b00000010,
	
	NONE      = 0b00001000
};

// Consts
const long double PI = 3.14159265359;

// Vars
Entity* player;
Tile** world;

// World
char map[][21] = {
	"####################",
	"#..................#",
	"#..................#",
	"#..................#",
	"#..................#",
	"#............###+###",
	"#............#.....#",
	"#............#.....#",
	"#............#.....#",
	"####################"
};

const Tile wall   = (Tile){WALL, '#', false};
const Tile ground = (Tile){GROUND, '.', true};
const Tile door   = (Tile){DOOR, '+', false};
const Tile blanktile = (Tile){BLANK, ' ', true};

// Fns
void playermv(int, int);

Tile** worldgen() {
	Tile** world = (Tile**)malloc(10 * sizeof(Tile*)); // Aloca espaço para 10 linhas
    for (int i = 0; i < 10; i++) world[i] = (Tile*)malloc(21 * sizeof(Tile));   // Aloca espaço para 21 colunas
	
	for (int y = 0; y < 10; y++) {
		for (int x = 0; x < 21; x++) {
			switch (map[y][x]) {
			case '#':
				world[y][x] = wall;
				break;
			case '.':
				world[y][x] = ground;
				break;
			case '+':
				world[y][x] = door;
				break;
			default:
				world[y][x] = blanktile;
			}
		}
		
	}
	
	return world;
}

enum Signal controls(int input) {
	switch (input) {
	case 'q':       return BREAK;
	
	case KEY_UP:    playermv(0, -1); break;
	case KEY_DOWN:  playermv(0, 1);  break;
	case KEY_LEFT:  playermv(-1, 0); break;
	case KEY_RIGHT: playermv(1, 0);  break;
	
	default: break;
	}
	
	return NONE;
}

void init() {
	initscr();
	noecho();
	curs_set(0);
	nodelay(stdscr, TRUE); // evita pausa no getch
	keypad(stdscr, TRUE);  // habilita captura de mais teclas

	// Player
	player = new(Entity);
	player->pos = (Point){5, 5};
	player->ch = '@';

	// World
	world = worldgen();
}

void loop() {
	int input;
	
	while ((input = getch())) {
		
		/* Controls */ {
			if (controls(input) & BREAK) break;
		}
		
		/* Update */ {
			
		}

		/* Draw */ {
			clear();

			for (int y = 0; y < 10; y++) {
				for (int x = 0; x < 21; x++) {

					mvaddch(y, x, world[y][x].ch);
					
				}
			}

			mvaddch(player->pos.y, player->pos.x, player->ch);

			refresh();
			napms(100);
		}		
	}
}

void close() {
	endwin();

	free(player);
	
	for (int i = 0; i < 10; i++) free(world[i]);
	free(world);	
}

// --- PLAYER ---------------------------------------

void playermv(int x, int y) {
	if (world[player->pos.y + y][player->pos.x + x].walkable) {
		player->pos.x += x;
		player->pos.y += y;
	} else if (world[player->pos.y + y][player->pos.x + x].id == DOOR) {
		world[player->pos.y + y][player->pos.x + x].walkable = true;
		world[player->pos.y + y][player->pos.x + x].ch = '\'';
	}
}


// --- MAIN -----------------------------------------

int main(int, char**) {
	// Calls
	init();
	loop();
	close();

	return 0;	
}
