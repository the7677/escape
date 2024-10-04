#include <ncurses.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#define PI 3.141592653589

#define WORLD_WIDTH  50
#define WORLD_HEIGHT 30

#define GRP_SIZE 16

#define COLOR_DEFAULT    1
#define COLOR_DEFAULT2   2
#define COLOR_UNSEEN     3
#define COLOR_UNSEENWALL 4
#define COLOR_WALL       5
#define COLOR_GROUND     6
#define COLOR_DOOR       7
#define COLOR_DESC       8

#define SEED 01110111-01100001-01110010-01100100 
#define DELAY_MS 10

#define NO_ARGS_MSG      "\033[30;103margv?\033[m\n"
#define WRONG_PASSWD_MSG "\033[30;103mSenha incorreta. Não desista!\033[m\n"

#define rads(x) (x * (PI/180))
#define new(T)  (T*)malloc(sizeof(T))
#define append(g, n) for (int i = 0; i < GRP_SIZE; i++) if (g[i] == NULL) { g[i] = n; break; }

// protótipo
#define addflag |=
#define rmflag  &= ~

// --- ENGINE ---------------------------------------

// Enums && Structs
typedef struct {
	int x, y;
} Point;

typedef struct {
	Point pos;
	char ch;
	bool visible;
	unsigned color;
	unsigned style;
	char desc[64];
} Entity;
	
enum TileID {
	WALL,
	GROUND,
	HDOOR,
	VDOOR,
	LOCKDOOR,
	UPSTAIRS,
	DOWNSTAIRS,
	GENERIC,
	STATUE,
	ERROR   
};

enum TileFlags {
	WALKABLE   = 0b000000001,
	SEEN       = 0b000000010,
	VISIBLE    = 0b000000100,
	SEETHROUGH = 0b000001000,
};

typedef struct {
	enum TileID id;
	char ch;
	unsigned color;
	unsigned style;
	enum TileFlags flags;
	char desc[64];
} Tile;

enum ItemID {
	CHEST,
	BUTTON,
};

typedef struct {
	enum ItemID id;
	Point pos;
	char ch;
	unsigned color;
	unsigned style;
	bool interactable;
	bool pickable;
	bool seen;
	bool visible;
	char desc[64];
} Item;

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

// Vars
Entity* player;
Entity *entity_grp[GRP_SIZE];
Item *item_grp[GRP_SIZE];
Tile** map;
Point cursorpos;
Point popup_pos = {0, 0};
char buff[32] = "";
char name[32] = "você";
enum Menu menu = SHORTCUTS;
enum Gamemode mode = NORMAL;

// Mapa
// a string acaba ocupando o espaço do \0, mas não há problema, pois a const é apenas um gerador
const char map1_sketch[WORLD_HEIGHT][WORLD_WIDTH] = {
	"##################################################", // 
	"#.........................###......####.......####", // 
	"#.#i.#%##################..#.......#.......#...###", // 
	"#.####.=.#c.c..........i#..#.....................#", // 
	"###..#.#................#......................#.#", // 
	"###..#.#................#..#.....#.............#.#", // 
	"###..#n#................#..##....#......##....##.#", // 
	"###..#########........####.###x#####....##....##.#", // 
	"###.....=................#.#....#####........###.#", // 
	"#.#######...######-#######.#....#####........#n..#", // 
	"#.#.##......x..#.....#.....#....######.......###.#", // 
	"#.#.........#.n#..S..#.#####....##########..###.##", // 
	"#.#######...####.....#..#%%%%%%%#########........#", // 
	"#.#<.#..-...-..########=#%#####%#####............#", // 
	"#....=..#...#..-.m#%%%#%#%###%%%###..............#", // 
	"#.#######...####.n%%#%#%%%#####%###.............##", // 
	"#.###...#############%#%#%#####%#####.S.S.S.S.####", // 
	"#......##.......##%%%%#%#%####...#################", // 
	"#......#.##...##.##%###%#%###S.c.S#####%%%%%######", // 
	"#......#....n....#%%%%%%######...########%########", // 
	"#......##.......######%########S######%%%%%%%#####", // 
	"#.......##.#.#.####%%%%##%###############%########", // 
	"##......%###%###%%###%#%%%###############%########", // 
	"#####..#%%%%%%%%%%%%%%%%#%#############%%%########", // 
	"#########%########%#####%%###############%%%######", // 
	"#####...........##%%%%%%%################%########", // 
	"####.....#....#######%###################%########", // 
	"####...........#####%%%################%%%%%######", // 
	"#####.......#.........################%%###%%#####", // 
	"##################################################", // 
};

const char cheatsheet[][70] = {
	"╭─╯ ╰─────────────────╮",
	"│tecla  info          │",
	"│                     │",
	"│q      voltar/sair   │",
	"│                     │",
	"│l      examinar      │",
	"│                     │",
	"│c      fechar porta  │",
	"│                     │",
	"│g      pegar item    │",
	"│                     │",
	"│↑↓←→   andar/        │",
	"│         interagir   │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│                     │",
	"│<      subir escada  │",
	"│                     │",
	"│>      descer escada │",
	"╰─────────────────────╯",
};
 
// Tiles                      id         ch    color                       style      flags                   desc
const Tile wall     = (Tile){ WALL     , ' ' , COLOR_PAIR(COLOR_WALL)    , A_NORMAL , 0                     , "Parede" };
const Tile grate    = (Tile){ GROUND   , '#' , COLOR_PAIR(COLOR_WALL)    , A_NORMAL , SEETHROUGH            , "Grelha de ventilação. \"Consigo ver o outro lado\""};
const Tile fakewall = (Tile){ WALL     , ' ' , COLOR_PAIR(COLOR_WALL)    , A_NORMAL , WALKABLE              , "Parede?" };
const Tile ground   = (Tile){ GROUND   , ' ' , COLOR_PAIR(COLOR_GROUND)  , A_NORMAL , WALKABLE | SEETHROUGH , "Carpete" };
const Tile hdoor    = (Tile){ HDOOR    , '-' , COLOR_PAIR(COLOR_DOOR)    , A_NORMAL , 0                     , "Uma porta. Parece destrancada" };
const Tile vdoor    = (Tile){ VDOOR    , '|' , COLOR_PAIR(COLOR_DOOR)    , A_NORMAL , 0                     , "Uma porta. Parece destrancada" };
const Tile lockdoor = (Tile){ LOCKDOOR , ';' , COLOR_PAIR(COLOR_DOOR)    , A_NORMAL , 0                     , "Uma porta. Há uma fechadura nela"}; 
const Tile statue   = (Tile){ STATUE   , '&' , COLOR_PAIR(COLOR_DEFAULT) , A_NORMAL , 0                     , "Uma estátua sinistra" };

const Tile blanktile = (Tile){ERROR, '!', COLOR_PAIR(COLOR_DEFAULT), A_BOLD, 0, "BLANKTILE"};

// Itens                    id       point          ch    color                        style      inter   pickb   seen    visib   desc
const Item chest  = (Item){ CHEST  , (Point){0,0} , 'n' , COLOR_PAIR(COLOR_DEFAULT2) , A_NORMAL , true  , false , false , false , "Baú" };
const Item button = (Item){ BUTTON , (Point){0,0} , 'i' , COLOR_PAIR(COLOR_DEFAULT2) , A_BOLD   , true  , false , false , false , "Pedestal com um botão no topo" };

// Fns
Tile** worldgen();
void draw_map(int, int);
void make_fov(int);
bool player_mv(int, int);
void player_closedoor();
void cursor_mv();
void menu_chsheet(int, int);
enum TileID door_axis(int ,int);
void draw_entities();


enum Signal controls(int input) {
	while (getch() != ERR) {} // Limpa o getch()

	if (input == ERR) return NONE;

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
			break;
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
		
		default:
			break;
		}
		
		return NONE;
	}
	
	
	// Controles do Player
	if (mode == NORMAL) {
		switch (input) {
		
		case KEY_UP:
			if (player_mv(0, -1)) return NEXT_TURN;
			break;
		case KEY_DOWN:
			if (player_mv(0, 1)) return NEXT_TURN;
			break;
		case KEY_LEFT:
			if (player_mv(-1, 0)) return NEXT_TURN;
			break;
		case KEY_RIGHT:
			if (player_mv(1, 0)) return NEXT_TURN;
			break;
			
		case 'c':
			player_closedoor();
			return NONE;
		default:
			break;
		}
	}

	// Se não houver retorno precoce, a função emitirá um sinal para incrementar o turno.
	return NONE;
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
	
	//        define             foreground     background
	init_pair( COLOR_DEFAULT    , COLOR_WHITE   , COLOR_BLACK );
	init_pair( COLOR_DEFAULT2   , COLOR_YELLOW  , COLOR_BLACK );
	init_pair( COLOR_UNSEEN     , COLOR_BLUE    , COLOR_BLACK );
	init_pair( COLOR_UNSEENWALL , COLOR_BLACK   , COLOR_BLUE  );
	init_pair( COLOR_WALL       , COLOR_BLACK   , COLOR_CYAN  );
	init_pair( COLOR_GROUND     , COLOR_MAGENTA , COLOR_BLACK );
	init_pair( COLOR_DOOR       , COLOR_YELLOW  , COLOR_BLACK );
	init_pair( COLOR_DESC       , COLOR_BLACK   , COLOR_GREEN );

	// Mapa
	map = worldgen();
	
	// Player
	player = new(Entity);
	append(entity_grp, player);	
	player->pos = (Point){8, 3};
	player->ch = '@';
	player->color = COLOR_PAIR(COLOR_DEFAULT);
	player->style = A_NORMAL;
	strcpy(player->desc, name);

	// Monstro
	Entity *sudis = new(Entity);
	append(entity_grp, sudis);
	sudis->pos = (Point){3, 10};
	sudis->ch = 's';
	sudis->color = COLOR_PAIR(COLOR_DEFAULT2);
	sudis->style = A_NORMAL;
	sudis->visible = false;
	strcpy(sudis->desc, "O Sudis");

	strcpy(map[3][7].desc, "Porta especial");
	
	append(entity_grp, player);	
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
			
			make_fov(12);

			if (player->pos.y > 9) popup_pos.y = 0;
			else                   popup_pos.y = 16;

			if (ticks % 400 == 0) player->style = player->style == A_NORMAL ? A_BOLD : A_NORMAL;
		}
		
		/* Draw */ {
			erase();

			// Menus
			mvprintw(0, WORLD_WIDTH+1, "╭I┬S┬!┬?╮");
			menu_chsheet(WORLD_WIDTH+1, 1);

			// Mapa
			draw_map(0, 0);

			// Itens

			// Entidades
			draw_entities();

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
			mvprintw(2, 75, buff);
			sprintf(buff, "%d", turn);
			mvprintw(3, 75, buff);

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

	if (cursorpos.x >= 0 && cursorpos.y >= 0 && cursorpos.x < WORLD_WIDTH && cursorpos.y < WORLD_HEIGHT && (map[cursorpos.y][cursorpos.x].flags & SEEN)) {
		for (int i = 0; i < GRP_SIZE; i++) {
			if (entity_grp[i] != NULL && entity_grp[i]->pos.x == cursorpos.x && entity_grp[i]->pos.y == cursorpos.y && entity_grp[i]->visible) {
				printw(entity_grp[i]->desc);
				addch(' ');

				attroff(COLOR_PAIR(COLOR_DESC));

				return;
			}	
		}
		
		printw(map[cursorpos.y][cursorpos.x].desc);
		addch(' ');
	}
	
	attroff(COLOR_PAIR(COLOR_DESC));
}

void menu_chsheet(int offset_x, int offset_y) {

	for (int y = 0; y < 23; y++) {
		mvprintw(y + offset_y, offset_x, cheatsheet[y]);	
	}
	
}

// --- MUNDO ----------------------------------------

Tile** worldgen() {
	Tile** map = (Tile**)malloc(WORLD_HEIGHT * sizeof(Tile*));                                     // Aloca 10 linhas
    for (int i = 0; i < WORLD_HEIGHT; i++) map[i] = (Tile*)malloc((WORLD_WIDTH+1) * sizeof(Tile)); // Aloca 21 colunas
	
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			char ch = map1_sketch[y][x];
			
			switch (ch) {
			// mapa
			case '#':
				map[y][x] = wall;
				break;
			case 'x':
				map[y][x] = grate;
				break;
			case '%':
				map[y][x] = fakewall;
				break;
			case '.':
				map[y][x] = ground;
				map[y][x].ch = rand() % 5 ? ' ' : '"';
				map[y][x].style = rand() % 2 ? A_NORMAL : A_DIM;
				break;
			case '-':
				if (map[y][x-1].id == WALL) map[y][x] = hdoor;
				else map[y][x] = vdoor;
				break;
			case '=':
				map[y][x] = lockdoor;
				break;
			case 'S':
				map[y][x] = statue;
				break;
				
			// itens
			case 'i':
				
			default:
				map[y][x] = ground;
				break;
			}
		}
	}
	
	return map;
}

void draw_map(int offset_x, int offset_y) {
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			if(!(map[y][x].flags & SEEN)) continue;
			
			if (map[y][x].id == WALL) {
				mvaddch(
					offset_y + y,
					offset_x + x,
					map[y][x].ch                                                                   | // Char
					((map[y][x].flags & VISIBLE) ? map[y][x].color : COLOR_PAIR(COLOR_UNSEENWALL)) | // Cor
				 	((map[y][x].flags & VISIBLE) ? map[y][x].style : A_DIM)                          // Estilo
			 	);
			 	continue;
			 }

			mvaddch(
				offset_y + y,
				offset_x + x,
				map[y][x].ch                                                               | // Char
				((map[y][x].flags & VISIBLE) ? map[y][x].color : COLOR_PAIR(COLOR_UNSEEN)) | // Cor
			 	((map[y][x].flags & VISIBLE) ? map[y][x].style : A_DIM)                      // Estilo
		 	);
		}
	}			
}

void make_fov(int vision_radius) {
	for (int y = 0; y < WORLD_HEIGHT; y++) {
		for (int x = 0; x < WORLD_WIDTH; x++) {
			map[y][x].flags rmflag VISIBLE;
		}
	}

	for (int i = 0; i < GRP_SIZE; i++) {
		if (entity_grp[i] == NULL) continue;

		entity_grp[i]->visible = false;
	}

	for (int deg = 0; deg < 360; deg++) {
		for (int r = 0; r <= vision_radius; r++) {
			int x = (int)(cos(rads(deg))*r) + player->pos.x,
				y = (int)(sin(rads(deg))*r) + player->pos.y;

			for (int i = 0; i < GRP_SIZE; i++) {
				if (entity_grp[i] == NULL) continue;

				if (entity_grp[i]->pos.x == x && entity_grp[i]->pos.y == y) {
					entity_grp[i]->visible = true;
				}
				
			}


			if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) continue;
		
			map[y][x].flags addflag VISIBLE;
			map[y][x].flags addflag SEEN;

			if (!(map[y][x].flags & SEETHROUGH)) break;
		} 
	}

}

// --- ITENS ----------------------------------------

void append_item(Item *item) {
	for (int i = 0; i < GRP_SIZE; i++) {
		if (item_grp[i] == NULL) {
			item_grp[i] = item;
			break;
		}
	}
}




// --- PLAYER ---------------------------------------

bool player_mv(int x, int y) {
	Tile tile = map[player->pos.y + y][player->pos.x + x];

	// fakewall
	if (tile.id == WALL && (tile.flags & WALKABLE)) map[player->pos.y + y][player->pos.x + x].ch = '.';

	if (tile.flags & WALKABLE) {
		player->pos.x += x;
		player->pos.y += y;

		return true;
	// abrir porta
	} else if (tile.id == HDOOR || tile.id == VDOOR) {
		map[player->pos.y + y][player->pos.x + x].flags addflag WALKABLE;
		map[player->pos.y + y][player->pos.x + x].flags addflag SEETHROUGH;
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
				map[y][x].flags rmflag WALKABLE;
				map[y][x].flags rmflag SEETHROUGH;
				map[y][x].ch = '-';
				break;
			case VDOOR:
				map[y][x].flags rmflag WALKABLE;
				map[y][x].flags rmflag SEETHROUGH;
				map[y][x].ch = '|';
				break;
			default:
				break;
			}
		}
	}
}

// --- OUTRAS ENTIDADES -----------------------------

void draw_entities() {
	for (int i = 0; i < GRP_SIZE; i++) {
		if (entity_grp[i] == NULL || !entity_grp[i]->visible) continue;

		Entity *ett = entity_grp[i];

		mvaddch(ett->pos.y, ett->pos.x, ett->ch | ett->color | ett->style);
	}
}

// --- ITENS ----------------------------------------

void draw_items() {
	for (int i = 0; i < GRP_SIZE; i++) {
		if (item_grp[i] == NULL || !item_grp[i]->visible) continue;

		Item *item = item_grp[i];

		mvaddch(item->pos.y, item->pos.x, item->ch | item->color | item->style);
	}
}

// --- MAIN -----------------------------------------

int main(int argc, char *argv[]) {
	// Calls
	if (argc > 2 && strlen(argv[2]) < 32) {
		strcpy(name, argv[2]);	
	}
	if (argc > 1 && !strcmp(argv[1], "ghost")) {
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
