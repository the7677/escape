#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include<locale.h>
#include <ncurses.h>


// macro for dynamic allocation
#define new(T) (T*)malloc(sizeof(T))

#define WORLD_WIDTH  25
#define WORLD_HEIGHT 25

#define COLOR_DEFAULT  1
#define COLOR_DEFAULT2 2
#define COLOR_UNSEEN   3
#define COLOR_WALL     4
#define COLOR_GROUND   5
#define COLOR_DOOR     6
#define COLOR_DESC     7

#define SEED 01110111-01100001-01110010-01100100 
#define DELAY_MS 10

#define NO_ARGS_MSG      "\033[30;103margv?\033[m\n"
#define WRONG_PASSWD_MSG "\033[30;103mSenha incorreta. Não desista!\033[m\n"

// --- ENGINE ---------------------------------------

// Enums && Structs
typedef struct {
	int x, y;
} Point;

typedef struct {
	Point pos;
	char ch;
	int color;
	int style;
} Entity;
	
enum TileID {
	WALL,
	GROUND,
	HDOOR,
	VDOOR,
	LOCKDOOR,
	BLANK
};

typedef struct {
	enum TileID id;
	char ch;
	int color;
	int style;
	bool walkable;
	bool seen;
	bool visible;
	char desc[64];
} Tile;

enum Signal {
	BREAK     = 0b00000001,
	CONTINUE  = 0b00000010,
	NEXT_TURN = 0b00000100,
	NONE      = 0b10000000
};

enum Gamemode {
	NORMAL,
	PAUSED,
	LOOK,
	INTERACTION,
	ATTACK,
	SLEEP,
};

enum Menu {
	INVENTORY,
	SHORTCUTS,
	ABOUT,
	HELP,
};

// Consts
const long double PI = 3.14159265359;

// Vars
Entity* player;
Tile** map;
Point cursorpos;
Point popup_pos = {0, 0};
char buff[64] = "";
char name[32] = "você";
enum Menu menu = SHORTCUTS;
enum Gamemode mode = NORMAL;

// Map
const char map1_sketch[WORLD_HEIGHT][WORLD_WIDTH+1] = {
	"#########################",
	"###....................##",
	"###i.#%###############..#",
	"######.=.#o.o.......i#..#",
	"###..#.#.............#..#",
	"###..#.#.............#..#",
	"#.#..#n#.............#..#",
	"#.#..#########.....####.#",
	"#.#.....=.............#.#",
	"#.#######...######-####.#",
	"#.#.##......=..#.....#..#",
	"#.#.........#.n#..S..#%##",
	"#.#######...####.....#%%#",
	"#.#<.#..-...-..########=#",
	"#....=..#...#..-..#%%%#%#",
	"#.#######...####..%%#%#%#",
	"#.###...#############%#%#",
	"#......##.......##%%%%#%#",
	"#......#.##...##.##%###%#",
	"#......#....n....%%%%%%%#",
	"#......##.......######%##",
	"#.......##.#.#.####%%%%##",
	"##.......#######%%###%#%#",
	"#####....%%%%%%%%%%%%%%%#",
	"#########################",


};

const char cheatsheet[][70] = {
	"╭─╯ ╰─────────────────╮",
	"│tecla  info          │",
	"│                     │",
	"│q      voltar/sair   │",
	"│l      examinar      │",
	"│c      abrir porta   │",
	"│g      pegar item    │",
	"│space  interagir     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│<      subir escada  │",
	"│>      descer escada │",
	"╰─────────────────────╯",
};

// Tiles                      id         ch    color                     style      wlkb    seen    visib   desc
const Tile wall     = (Tile){ WALL     , ' ' , COLOR_PAIR(COLOR_WALL)  , A_NORMAL , false , false , false , "Parede" };
const Tile fakewall = (Tile){ WALL     , ' ' , COLOR_PAIR(COLOR_WALL)  , A_NORMAL , true  , false , false , "Parede?" };
const Tile ground   = (Tile){ GROUND   , '.' , COLOR_PAIR(COLOR_GROUND), A_NORMAL , true  , false , false , "O chão" };
const Tile hdoor    = (Tile){ HDOOR    , '-' , COLOR_PAIR(COLOR_DOOR)  , A_NORMAL , false , false , false , "Uma porta. Parece destrancada" };
const Tile vdoor    = (Tile){ VDOOR    , '|' , COLOR_PAIR(COLOR_DOOR)  , A_NORMAL , false , false , false , "Uma porta. Parece destrancada" };
const Tile lockdoor = (Tile){ LOCKDOOR , '#' , COLOR_PAIR(COLOR_WALL)  , A_NORMAL , false , false , false , "Uma porta. Há uma fechadura nela"}; 


const Tile blanktile = (Tile){BLANK, '!', COLOR_PAIR(COLOR_DEFAULT), A_BOLD, true, false, false, "BLANKTILE"};

// Fns
bool player_mv(int, int);
void player_closedoor();
void cursor_mv();
void menu_chsheet(int, int);
enum TileID door_axis(int ,int);

Tile** worldgen() {
	Tile** map = (Tile**)malloc(WORLD_HEIGHT * sizeof(Tile*));                                     // Aloca 10 linhas
    for (int i = 0; i < WORLD_HEIGHT; i++) map[i] = (Tile*)malloc((WORLD_WIDTH+1) * sizeof(Tile)); // Aloca 21 colunas
	
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			char ch = map1_sketch[y][x];

			switch (ch) {
			case '#':
				map[y][x] = wall;
				break;
			case '%':
				map[y][x] = fakewall;
				break;
			case '.':
				map[y][x] = ground;
				map[y][x].ch = rand() % 5 == 0 ? '"' : ' ';
				map[y][x].style = rand() % 2 == 0 ? A_DIM : A_NORMAL;
				
				break;
			case '-':
				if (map[y][x-1].id == WALL){
					map[y][x] = hdoor;
					break;
				}

				map[y][x] = vdoor;
				break;
			case '=':
				map[y][x] = lockdoor;
				break;
			default:
				map[y][x] = blanktile;
			}
		}
		
	}
	
	return map;
}

void draw_map(int offset_x, int offset_y) {
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			mvaddch(offset_y + y, offset_x + x, map[y][x].ch | map[y][x].color | map[y][x].style);
		}
	}			
}

enum Signal controls(int input) {
	while (getch() != ERR) {} // Limpa o getch()
	if (input == ERR) {
		return NONE;
	}
	

	// Voltar/Sair
	if (input == 'q') {
		switch (mode) {
		case NORMAL:
			mode = PAUSED;
		case PAUSED:
			attron(COLOR_PAIR(COLOR_DESC));
			
			mvprintw(popup_pos.y+0, popup_pos.x, "╭────────────────────────────────╮");
			mvprintw(popup_pos.y+1, popup_pos.x, "│O jogo não é salvo. Sair? (s/n).│");
			mvprintw(popup_pos.y+2, popup_pos.x, "╰────────────────────────────────╯");
			
			attroff(COLOR_PAIR(COLOR_DESC));

			while ((input = getch())) {
				if (input >= 65 && input < 91) input += 32;
				if (input == 's' || input == 'y') return BREAK;
				else if (input != ERR) {
					mode = NORMAL;
					return NONE;	
				}
			}
			
			return NONE;
		case LOOK:
			mode = NORMAL;
			return NONE;			
		case SLEEP:

		default:
		
		}
	}	

	// Examinar
	if (mode == NORMAL && input == 'l') {
		mode = LOOK;

		cursorpos = player->pos;
		
		return NONE;
	}

	if (mode == LOOK) {
		switch (input) {
		
		case KEY_UP:    cursorpos.y--; break;
		case KEY_DOWN:  cursorpos.y++; break;
		case KEY_LEFT:  cursorpos.x--; break;
		case KEY_RIGHT: cursorpos.x++; break;
		
		default: break;
		}
		return NONE;
	}
	
	
	// Player Actions
	if (mode == NORMAL) {
		switch (input) {
		
		case KEY_UP:
			if (!player_mv(0, -1)) return NONE;
			break;
		case KEY_DOWN:
			if (!player_mv(0, 1)) return NONE;
			break;
		case KEY_LEFT:
			if (!player_mv(-1, 0)) return NONE;
			break;
		case KEY_RIGHT:
			if (!player_mv(1, 0)) return NONE;
			break;
			
		case 'c':
			player_closedoor();
			break;
		
		default:
		
		}
	}

	// Se não houver retorno precoce, a função emitirá um sinal para continuar o turno.
	return NEXT_TURN;
}

void init() {
	// Locale
	setlocale(LC_CTYPE, ""); // Muda locale para UTF-8

	// Seed
	srand(SEED);             // Define a seed de aleatoriedade

	// Ncurses
	initscr();               // Inicia a tela do ncurses
	noecho();                // Impede o eco da tecla apertada
	curs_set(0);             // Esconde o cursor
	keypad(stdscr, TRUE);    // habilita captura de mais teclas
	nodelay(stdscr, TRUE);   // Desabilita a pausa do getch()

	// Ncurses - Cor
	start_color();           // Inicia a compatibilidade com cores
	init_pair(COLOR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
	init_pair(COLOR_DEFAULT2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_UNSEEN, COLOR_BLUE, COLOR_BLACK);
	init_pair(COLOR_WALL, COLOR_BLACK, COLOR_CYAN);
	init_pair(COLOR_GROUND, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(COLOR_DOOR, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_DESC, COLOR_BLACK, COLOR_GREEN);

	// Mapa
	
	// Player
	player = new(Entity);
	player->pos = (Point){7, 6};
	player->ch = '@';
	player->color = COLOR_PAIR(COLOR_DEFAULT);
	player->style = A_NORMAL;
	
	// Mapa
	map = worldgen();
}

void loop() {
	int input = ' ', turn = 0, ticks = 0;
	enum Signal signal;
	bool l_pressed = false, g_pressed = false;

	do {
		if (input >= 65 && input < 91) input += 32; // converte input para minúsculo

		/* Controls */ {
			if (input == 'g') g_pressed = true;	
			signal = controls(input);
						
		}
		
		/* Update */ {
			if (signal & BREAK) break;
			if (signal & CONTINUE) continue;
			if (signal & NEXT_TURN) {
				turn++;
			}

			if (player->pos.y > 9) popup_pos.y = 0;
			else                   popup_pos.y = 16;

			if (ticks % 400 == 0) {
				player->style = player->style == A_NORMAL ? A_BOLD : A_NORMAL;
			}
		}
		
		/* Draw */ {
			erase();

			// Menus
			mvprintw(0, 26, "╭I┬S┬!┬?╮");
			menu_chsheet(26, 1);

			// Mapa
			draw_map(0, 0);

			mvaddch(player->pos.y, player->pos.x, player->ch | player->color | player->style);

			// Tutorial
			if (turn <= 10 && mode == NORMAL) {
				if (input == 'l') l_pressed = true;	

				attron(COLOR_PAIR(COLOR_DESC));

				mvaddch(player->pos.y, player->pos.x+1, '<');
				printw(name);
				addch(' ');
				
				mvprintw(popup_pos.y+0, popup_pos.x, "╭─────────────────────╮");
				mvprintw(popup_pos.y+1, popup_pos.x, "│Mova-se com as setas.│");
				mvprintw(popup_pos.y+2, popup_pos.x, "╰─────────────────────╯");

				attroff(COLOR_PAIR(COLOR_DESC));
			} else if (!l_pressed) {
				if (input == 'l') l_pressed = true;	

				attron(COLOR_PAIR(COLOR_DESC));
				
				mvprintw(popup_pos.y+0, popup_pos.x, "╭─────────────────────────────╮");
				mvprintw(popup_pos.y+1, popup_pos.x, "│Examine os arredores com [l].│");
				mvprintw(popup_pos.y+2, popup_pos.x, "╰─────────────────────────────╯");
				
				attroff(COLOR_PAIR(COLOR_DESC));
			} else if (!g_pressed && mode == NORMAL) {
					if (input == 'g') g_pressed = true;	

				attron(COLOR_PAIR(COLOR_DESC));
								
				mvprintw(popup_pos.y+0, popup_pos.x, "╭────────────────────────────╮");
				mvprintw(popup_pos.y+1, popup_pos.x, "│Pegue itens no chão com [g].│");
				mvprintw(popup_pos.y+2, popup_pos.x, "╰────────────────────────────╯");
				
				attroff(COLOR_PAIR(COLOR_DESC));
			}

			// Modos Especiais
			if (mode == LOOK) {
				cursor_mv();
			}

			sprintf(buff, "%d", ticks / 1000);
			mvprintw(3, 50, buff);
			sprintf(buff, "%d", turn);
			mvprintw(4, 50, buff);

			refresh();
			napms(DELAY_MS);
			ticks += DELAY_MS;
		}

	} while ((input = getch()));
}

void close() {
	endwin();

	free(player);
	
	for (int i = 0; i < 10; i++) free(map[i]);
	free(map);	
}

// --- MENUS ETC ------------------------------------

void cursor_mv() {
	attron(COLOR_PAIR(COLOR_DESC));
	
	mvaddch(cursorpos.y, cursorpos.x+1, '<');
	if (cursorpos.x >= 0 && cursorpos.y >= 0 && cursorpos.x < WORLD_WIDTH && cursorpos.y < WORLD_HEIGHT) printw(map[cursorpos.y][cursorpos.x].desc);
	addch(' ');
	
	attroff(COLOR_PAIR(COLOR_DESC));
}

void menu_chsheet(int offset_x, int offset_y) {

	for (int y = 0; y < 23; y++) {
		mvprintw(y + offset_y, offset_x, cheatsheet[y]);	
	}
	
}

// --- PLAYER ---------------------------------------

bool player_mv(int x, int y) {
	Tile tile = map[player->pos.y + y][player->pos.x + x];

	if (tile.walkable) {
		player->pos.x += x;
		player->pos.y += y;

		return true;
	} else if (tile.id == HDOOR || tile.id == VDOOR) {
		map[player->pos.y + y][player->pos.x + x].walkable = true;
		map[player->pos.y + y][player->pos.x + x].ch = '\'';
	}
	
	return false;
}

void player_closedoor() {
	int x, y;
				
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if( i == 0 && j == 0) continue;

			x = player->pos.x + i;
			y = player->pos.y + j;

			switch (map[y][x].id) {
				
			case HDOOR:
				map[y][x].walkable = false;
				map[y][x].ch = '-';
				break;
			case VDOOR:
				map[y][x].walkable = false;
				map[y][x].ch = '|';
				break;
			default:
			}
		}
	}
}

// --- MAIN -----------------------------------------

int main(int argc, char *argv[]) {
	// Calls
	if (argc > 2 && strlen(argv[2]) < 32) {
		strcpy(name, argv[2]);	
	}
	if (argc > 1 && !strcmp(argv[1], "ward")) {
		init();
		loop();
		close();
		
		return 0;
	}
	if (argc > 2 && strlen(argv[2])) {
		sprintf(name, "%s ", argv[2]);
	}
	
	printf(WRONG_PASSWD_MSG);

	return 0;
}
