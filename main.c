#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include<locale.h>
#include <ncurses.h>


// macro for dynamic allocation
#define new(T) (T*)malloc(sizeof(T))

#define WORLD_WIDTH  20
#define WORLD_HEIGHT 10
#define WRONG_PASSWD_MSG "\033[30;103mSenha incorreta. Não desista!\033[m\n"

// --- ENGINE ---------------------------------------

// Enums && Structs
typedef struct {
	int x, y;
} Point;

typedef struct {
	Point pos;
	char ch;
	int fgcolor;
	int bgcolor;
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
	char title[32];
	char ch;
	int fgcolor;
	int bgcolor;
	bool walkable;
	bool seen;
} Tile;

enum Signal {
	BREAK     = 0b00000001,
	NEXT_TURN = 0b00000010,
	
	NONE      = 0b10000000
};

// Consts
const long double PI = 3.14159265359;

// Vars
Entity* player;
Tile** map;

// Map
char map_sketch[WORLD_HEIGHT][WORLD_WIDTH+1] = {
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

//                          id       tl                         ch    fg  bg  wlkb    seen
const Tile wall   = (Tile){ WALL   , "A wall."                , '%' , 1 , 0 , false , false };
const Tile ground = (Tile){ GROUND , "The ground."            , '.' , 1 , 0 , true  , false };
const Tile door   = (Tile){ DOOR   , "A door. Can be opened." , '+' , 1 , 0 , false , false };

const Tile blanktile = (Tile){BLANK, "", '!', 1, 0, true, false};

// Fns
void playermv(int, int);

Tile** worldgen() {
	Tile** map = (Tile**)malloc(10 * sizeof(Tile*));                          // Aloca espaço para 10 linhas
    for (int i = 0; i < WORLD_HEIGHT; i++) map[i] = (Tile*)malloc(21 * sizeof(Tile));   // Aloca espaço para 21 colunas
	
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			char ch = map_sketch[y][x];
			
			switch (ch) {
			case '#':
				map[y][x] = wall;
				break;
			case '.':
				map[y][x] = ground;
				break;
			case '+':
				map[y][x] = door;
				break;
			default:
				map[y][x] = blanktile;
			}
		}
		
	}
	
	return map;
}

enum Signal controls(int input) {
	switch (input) {
	case 'q':       return BREAK;
	
	case KEY_UP:    playermv(0, -1); break;
	case KEY_DOWN:  playermv(0, 1);  break;
	case KEY_LEFT:  playermv(-1, 0); break;
	case KEY_RIGHT: playermv(1, 0);  break;

	case 'C':
		int x, y;
		
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				x = player->pos.x + i;
				y = player->pos.y + j;
				
				if (map[y][x].id == DOOR || map[y][x].id == KEYLOCKED_DOOR) {
					map[y][x].walkable = false;
					map[y][x].ch = '+';
				}
			}
		}
		
		break;
	
	default: break;
	}
	
	return NONE;
}

void init() {
	// Locale
	setlocale(LC_CTYPE, ""); // Muda locale para UTF=8

	// Ncurses
	initscr();            // Inicia a tela do ncurses
	start_color();        // Inicia a compatibilidade com cores
	noecho();             // Impede o eco da tecla apertada
	curs_set(0);          // Esconde o cursor
	keypad(stdscr, TRUE); // habilita captura de mais teclas

	// Player
	player = new(Entity);
	player->pos = (Point){5, 5};
	player->ch = '@';
	player->fgcolor = COLOR_PAIR(2);

	// World
	map = worldgen();
}

void loop() {
	int input;
	
	do {
		/* Controls */ {
			if (controls(input) & BREAK) break;
		}
		
		/* Update */ {
			
		}

		/* Draw */ {
			erase();

			// draw world
			for (int y = 0; y < 10; y++) {
				for (int x = 0; x < 20; x++) {

					mvaddch(y, x, map[y][x].ch | map[y][x].fgcolor);
					
				}
			}

			mvaddch(player->pos.y, player->pos.x, player->ch);
			// init_pair()
			printw("<-você");
			
			refresh();
			napms(1);
		}

	} while ((input = getch()));
}

void close() {
	endwin();

	free(player);
	
	for (int i = 0; i < 10; i++) free(map[i]);
	free(map);	
}

// --- PLAYER ---------------------------------------

void playermv(int x, int y) {
	if (map[player->pos.y + y][player->pos.x + x].walkable) {
		player->pos.x += x;
		player->pos.y += y;
	} else if (map[player->pos.y + y][player->pos.x + x].id == DOOR) {
		map[player->pos.y + y][player->pos.x + x].walkable = true;
		map[player->pos.y + y][player->pos.x + x].ch = '\'';
	}
}


// --- MAIN -----------------------------------------

int main(int argc, char *argv[]) {
	// Calls
	if (argc > 1 && !strcmp(argv[1], "ghost")) {
		init();
		loop();
		close();
		
		return 0;
	}
	
	printf(WRONG_PASSWD_MSG);

	return 0;
}
