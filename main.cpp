#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
//#include "main.h"
}

#define SCREEN_WIDTH 640 //px
#define SCREEN_HEIGHT 480 //px
#define BACKGROUND_HEIGHT 480 //px
#define BACKGROUND_WIDTH 1280 //px
#define UNICORN_WIDTH 80 //px
#define UNICORN_HEIGHT 40 //px
#define OBSTACLE_WIDTH 70 //px
#define OBSTACLE_HEIGHT 40 //px
#define STAR_WIDTH 70 //px
#define STAR_HEIGHT 70 //px
#define SHELF_WIDTH 250 //px
#define SHELF_HEIGHT 35 //px
#define DASH_COOLDOWN 1000 //seconds
#define DASH_HASTE 1.2 //dash speed multiplier
#define DASH_HASTE_REDUCTION 0.005 //reduction of dash haste
#define JUMP_HASTE 2.5 //value added to vertical speed during jump
#define JUMP_HEIGHT 30 //max height of jump, px
#define BASE_VELOCITY 0.3 //base speed
#define MINIMAL_Y -400 //minimal value of y_position, px
#define EPSILON 5 //epsilon, used in colision, px
#define SHELVES_NUMBER 9 //number of platforms during one pass of the map
#define STALAGTITE_WIDTH 80 //px
#define STALAGTITE_HEIGHT 170 //px
#define OBSTACLES_NUMBER 2 //number of obstacles on the map
#define FAIRY_WIDTH 25 //px
#define FAIRY_HEIGHT 25 //px
#define AREA_SIZE 50 //range of fairy movement, px
#define LIVES_NUMBER 3 //number of lives
#define LIFE_HEIGHT 20 //px
#define LIFE_WIDTH 20 //px
#define MAX_STRING_SIZE 128 //max length of the caption 
#define STALAGTITE_SHIFT 100 //shift of the stalagtite, px
#define OBSTACLE_SHIFT 200 //shift of the obstacle, px
#define STAR_SHIFT 150 //shift of the star, px
#define FAIRY_SHIFT 400 //shift of the fairy, px
#define HASTE 0.004 //growth of the speed due to traveled distance
#define STAR_NUMBER 3 //number of stars
#define FAIRY_NUMBER 3 //number of places where fairies can appear
#define INFO_HEIGHT 35 //px
#define INFO_WIDTH 160 //px
#define FAIRY_HASTE 3 //haste of the fairy
#define MOVE_FREQUENCY 20 //frequency of the fairy's movement
#define GRAVITY_VALUE 300 //value of the gravity
#define MAX_Y 250 //maximum value of y_movement, px
#define LIFE_SHIFT 30 //shift of the lives, px
#define STAR_POINTS 100 //number of points for a single star
#define FAIRY_POINTS 10 //number of points for a single fairy


// drawing txt caption on the screen, starting form point (x,y)
// charset is a bitmap 128x128
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

//drawing sprite on the screen, starting form point (x,y)
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

//drawing single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};

// drawing the vertical line of length l (when dx=0, dy=1)
// or horizontal (when dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// drawing rectangle of sides l and k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

//collision detector
int CheckCollision(SDL_Rect unicorn, SDL_Rect object, char type)
{
	//declaring the rectangle sides
	int unicornLeft, unicornRight, unicornTop, unicornBottom;
	int objectLeft, objectRight, objectTop, objectBottom;

	//calculating the sides of the first hitbox
	unicornLeft = unicorn.x;
	unicornRight = unicorn.x + unicorn.w;
	unicornTop = unicorn.y;
	unicornBottom = unicorn.y + unicorn.h;

	//calculating the sides of the second hitbox
	objectLeft = object.x;
	objectRight = object.x + object.w;
	objectTop = object.y;
	objectBottom = object.y + object.h;

	//if any side of first hitbox is outside the second then there's no collision
	if (unicornBottom < objectTop) return 0;
	if (unicornTop > objectBottom) return 0;
	if (unicornRight < objectLeft) return 0;
	if (unicornLeft > objectRight) return 0;
	//if it's collision with the platform i check cases of jumping on it, or hitting it from below
	//abs because program can skip the moment of collision with the platform
	if (type == 'p') {
		if (abs(unicornBottom - objectTop) < EPSILON) return 2;
		if (abs(unicornTop - objectBottom) < EPSILON) return 3;
	}

	//if not, then there's a collision
	return 1;
}

//reset the game
void ClearGame(double* backgroundDistance, double* obstacleDistance,
				double* worldTime, double* worldRealTime,
				double* baseYPosition, double* yPosition, int* dashCd, 
				int* dash, double* screenMovement, double* yMovement, 
				int* jumping, int* peak, double shelfDistance[], 
				double* stalagtiteDistance, double* velocity, double* velocityCap,
				double* yVelocity, int* lives, int* play, int* starStreak, 
				int* starDestroyed, int* tempStar, int* fairyStreak, int* fairyCaught,
				int* tempFairy) {
	
	*backgroundDistance = 0;
	*obstacleDistance = 0;
	*worldTime = 0;
	*worldRealTime = 0;
	*baseYPosition = MINIMAL_Y;
	*yPosition = 0;
	*dashCd = 0;
	*dash = 0;
	*screenMovement = 0;
	*yMovement = 0;
	*jumping = 1;
	*peak = 1;
	*stalagtiteDistance = 0;
	*velocity = BASE_VELOCITY;
	*yVelocity = 0;
	if(*lives)(*lives)--;
	*velocityCap = BASE_VELOCITY;
	for (int i = 0; i < SHELVES_NUMBER; i++) shelfDistance[i] = 0;
	*starStreak = 0;
	*fairyStreak = 0;
	*fairyCaught = 0;
	*starDestroyed = 0;
	*tempStar = 0;
	*tempFairy = 0;
	*play = 0;
}

//loading the file from path
int LoadFile(SDL_Surface** load, char* path, SDL_Surface** screen, SDL_Surface** charset, 
			SDL_Surface** unicorn, SDL_Surface** platform, SDL_Surface** background,
			SDL_Surface* obstacle[], SDL_Surface* shelf[], SDL_Surface** stalagtite, 
			SDL_Surface* life[], SDL_Surface** menu, SDL_Surface** mainMenu,
			SDL_Surface** star, SDL_Surface** fairy, SDL_Texture** scrtex,
			SDL_Window** window, SDL_Renderer** renderer) {

	*load = SDL_LoadBMP(path);
	if (*load == NULL) {
		if(strcmp(path, "./endgame.bmp")) printf("SDL_LoadBMP(%s) error: %s\n", path, SDL_GetError());
		SDL_FreeSurface(*screen);
		SDL_FreeSurface(*charset);
		SDL_FreeSurface(*unicorn);
		SDL_FreeSurface(*platform);
		SDL_FreeSurface(*background);
		for (int i = 0; i < OBSTACLES_NUMBER; i++) SDL_FreeSurface(obstacle[i]);
		for (int i = 0; i < SHELVES_NUMBER; i++) SDL_FreeSurface(shelf[i]);
		SDL_FreeSurface(*stalagtite);
		for (int i = 0; i < LIVES_NUMBER; i++) SDL_FreeSurface(life[i]);
		SDL_FreeSurface(*menu);
		SDL_FreeSurface(*mainMenu);
		SDL_FreeSurface(*star);
		SDL_FreeSurface(*fairy);
		SDL_DestroyTexture(*scrtex);
		SDL_DestroyWindow(*window);
		SDL_DestroyRenderer(*renderer);
		SDL_Quit();
		return 1;
	};
	return 0;
}

//randomizing the movement of the fairy
void randomizeMovement(double* fairyXMovement, double* fairyYMovement) {
	switch (rand() % MOVE_FREQUENCY) {
	case 0:
		*fairyXMovement = fmin((*fairyXMovement) + FAIRY_HASTE, AREA_SIZE / 2);
		break;
	case 1:
		*fairyXMovement = fmax((*fairyXMovement) - FAIRY_HASTE, -AREA_SIZE / 2);
		break;

	}
	switch (rand() % MOVE_FREQUENCY) {
	case 0:
		*fairyYMovement = fmin((*fairyYMovement) + FAIRY_HASTE, AREA_SIZE / 2);
		break;
	case 1:
		*fairyYMovement = fmax((*fairyYMovement) - FAIRY_HASTE, -AREA_SIZE / 2);
		break;
	}
}

// main
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	//variables
	int t1, t2, rc, quit = 0, jumping = 0, controls = 0,
		dash = 0, dashCd = 0, doubleJump = 0, peak = 0, walking = 0,
		lives = 0, play = 0, starDestroyed = 0, fairyCaught = 0,
		starStreak = 0, fairyStreak = 0, starSpawn[STAR_NUMBER] = {8,5,6}, 
		fairySpawn[FAIRY_NUMBER] = {5,4,8}, tempStar=0, tempFairy=0;

	const double gravity = GRAVITY_VALUE, 
			shelfWidth[SHELVES_NUMBER] = { 350, 700, 300, 700, 700, 200, 600, 500, 700 }, //px
			shelfHeight[SHELVES_NUMBER] = { 30, 30, 30, 30, 30, 60, 30, 30, 30 }, //px
			shelfElevation[SHELVES_NUMBER] = { 100, 270, 420, 170, 270, 310, 280, 170, 320 }, //px
			shelfShift[SHELVES_NUMBER] = { 0, SCREEN_WIDTH, SCREEN_WIDTH + 250, SCREEN_WIDTH + 900, //px
				SCREEN_WIDTH + 2000, SCREEN_WIDTH + 2300, SCREEN_WIDTH + 2900, SCREEN_WIDTH + 3700,
				SCREEN_WIDTH + 4000 };
			
	double delta, worldTime, worldRealTime,
		stalagtiteDistance, backgroundDistance, obstacleDistance,
		shelfDistance[SHELVES_NUMBER], velocity, velocityCap,
		yPosition, yVelocity, baseYPosition = 0, screenMovement = 0, 
		yMovement = 0, yCap=0, fairyXMovement=0, fairyYMovement=0, points = 0,
		maxWidth = 0, maxShift = 0;

	SDL_Event event;
	SDL_Texture* scrtex;
	SDL_Window* window=NULL;
	SDL_Renderer* renderer=NULL;

	SDL_Surface* screen = NULL, * charset = NULL, * unicorn = NULL, 
			* platform = NULL, * background = NULL, * obstacle[OBSTACLES_NUMBER], 
			* shelf[SHELVES_NUMBER], * stalagtite = NULL, * life[LIVES_NUMBER], 
			* menu = NULL, * mainMenu = NULL, * star = NULL, * fairy = NULL;

	SDL_Rect unicornHitbox, obstacleHitbox[OBSTACLES_NUMBER], platformHitbox,
			shelfHitbox[SHELVES_NUMBER], stalagtiteHitbox, starHitbox, fairyHitbox;

	//clearing the sprites
	for (int i = 0; i < OBSTACLES_NUMBER; i++) obstacle[i] = NULL;
	for (int i = 0; i < SHELVES_NUMBER; i++) shelf[i] = NULL;
	for (int i = 0; i < LIVES_NUMBER; i++) life[i] = NULL;

	//clearing the distances made by the platforms
	for (int i = 0; i < SHELVES_NUMBER; i++) shelfDistance[i] = 0;

	//searching for largest length value amongst platforms and its shift
	for (int i = 1; i < SHELVES_NUMBER; i++) {
		maxShift = fmax(maxShift, shelfShift[i]);
		maxWidth = fmax(maxWidth, shelfWidth[i]);
	}

	//randomizer
	srand(time(NULL));

	//sdl
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	//screen
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Koptworz atak Stefana");
	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	//disabling the cursor
	SDL_ShowCursor(SDL_DISABLE);

	//loading the charset
	if (LoadFile(&charset, "./cs8x8.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	SDL_SetColorKey(charset, true, 0x000000);

	//loading the endless platform
	if (LoadFile(&platform, "./floor.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;

	//loading the unicorn
	if (LoadFile(&unicorn, "./unicorn.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	unicorn->h = UNICORN_HEIGHT;
	unicorn->w = UNICORN_WIDTH;
	unicornHitbox.h = UNICORN_HEIGHT;
	unicornHitbox.w = UNICORN_WIDTH;

	//loading the death screen
	if (LoadFile(&menu, "./menu.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	menu->h = SCREEN_HEIGHT;
	menu->w = SCREEN_WIDTH;

	//loading the menu
	if (LoadFile(&mainMenu, "./main_menu.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	mainMenu->h = SCREEN_HEIGHT;
	mainMenu->w = SCREEN_WIDTH;

	//loading the shelves
	for (int i = 0; i < SHELVES_NUMBER; i++) {
		if (LoadFile(&shelf[i], "./floor.bmp", &screen, &charset,
			&unicorn, &platform, &background, obstacle, shelf,
			&stalagtite, life, &menu, &mainMenu,
			&star, &fairy, &scrtex, &window, &renderer)) return 1;
		shelf[i]->h = shelfHeight[i];
		shelf[i]->w = shelfWidth[i];
		shelfHitbox[i].h = shelfHeight[i];
		shelfHitbox[i].w = shelfWidth[i];
	}

	//loading the stalagtites
	if (LoadFile(&stalagtite, "./stalagtite.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	stalagtite->h = STALAGTITE_HEIGHT;
	stalagtite->w = STALAGTITE_WIDTH;
	stalagtiteHitbox.h = STALAGTITE_HEIGHT;
	stalagtiteHitbox.w = STALAGTITE_WIDTH;

	//loading the star
	if (LoadFile(&star, "./star.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	star->h = STAR_HEIGHT;
	star->w = STAR_WIDTH;
	starHitbox.h = STAR_HEIGHT;
	starHitbox.w = STAR_WIDTH;

	//loading the fairy
	if (LoadFile(&fairy, "./fairy.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	fairy->h = FAIRY_HEIGHT;
	fairy->w = FAIRY_WIDTH;
	fairyHitbox.h = FAIRY_HEIGHT;
	fairyHitbox.w = FAIRY_WIDTH;

	//loading the obstacle
	for (int i = 0; i < OBSTACLES_NUMBER; i++) {
		if (LoadFile(&obstacle[i], "./obstacle.bmp", &screen, &charset,
			&unicorn, &platform, &background, obstacle, shelf,
			&stalagtite, life, &menu, &mainMenu,
			&star, &fairy, &scrtex, &window, &renderer)) return 1;
		obstacle[i]->h = OBSTACLE_HEIGHT;
		obstacle[i]->w = OBSTACLE_WIDTH;
		obstacleHitbox[i].h = OBSTACLE_HEIGHT;
		obstacleHitbox[i].w = OBSTACLE_WIDTH;
	}

	//loading the life
	for (int i = 0; i < LIVES_NUMBER; i++) {
		if (LoadFile(&life[i], "./life.bmp", &screen, &charset,
			&unicorn, &platform, &background, obstacle, shelf,
			&stalagtite, life, &menu, &mainMenu,
			&star, &fairy, &scrtex, &window, &renderer)) return 1;
		life[i]->h = LIFE_HEIGHT;
		life[i]->w = LIFE_WIDTH;
	}

	//loading the background
	if (LoadFile(&background, "./background.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 1;
	background->h = BACKGROUND_HEIGHT;
	background->w = BACKGROUND_WIDTH;

	//colors
	char text[MAX_STRING_SIZE];
	char pointsString[MAX_STRING_SIZE];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	//setting initial values
	ClearGame(&backgroundDistance, &obstacleDistance,
			&worldTime, &worldRealTime, &baseYPosition, &yPosition,
			&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
			shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
			&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
			&fairyStreak, &fairyCaught, &tempFairy);

	while (!quit) {
		if (!play && !lives) {
			//drawing the menu
			DrawSurface(screen, mainMenu, 0, 0);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case(SDL_QUIT):
					quit = 1;
					break;
				case(SDL_KEYDOWN):
					if (event.key.keysym.sym == SDLK_s) {
						//starting the game
						t1 = SDL_GetTicks();
						play = 1;
						points = 0;
						lives = LIVES_NUMBER;
					}
					break;
				}
			}
		}
		else if (!play && lives) {
			//drawing the death screen
			DrawSurface(screen, menu, 0, 0);
			sprintf(pointsString, "Points = %d ", (int)points);
			DrawString(screen, SCREEN_WIDTH/8, SCREEN_HEIGHT / 1.7, pointsString, charset);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case(SDL_QUIT):
					quit = 1;
					break;
				case(SDL_KEYDOWN):
					if (event.key.keysym.sym == SDLK_s) {
						//continue the game
						t1 = SDL_GetTicks();
						points = 0;
						play = 1;
					}
					//n->new game
					if (event.key.keysym.sym == SDLK_n) {
						ClearGame(&backgroundDistance, &obstacleDistance,
							&worldTime, &worldRealTime, &baseYPosition, &yPosition,
							&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
							shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
							&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
							&fairyStreak, &fairyCaught, &tempFairy);
						lives = 0;
						continue;
					}
					break;
				}
			}
		}
		else {
			//time since last drawing of the screen
			t2 = SDL_GetTicks();
			delta = (t2 - t1);

			//time in ms
			worldRealTime += delta;
			
			//ms into seconds
			delta *= 0.001;
			t1 = t2;

			//time in seconds
			worldTime += delta;

			//camera movement
			if (yPosition >= 0) {
				yMovement = fmin(yPosition, MAX_Y);
				screenMovement = fmax(0, yPosition - MAX_Y);
			}
			else if (yPosition < 0) {
				yMovement = fmax(yPosition, - MAX_Y);
				screenMovement = fmin(0, yPosition + MAX_Y);
			}

			//filling the screen with black
			SDL_FillRect(screen, NULL, czarny);

			//randomizing the fairires movement
			randomizeMovement(&fairyXMovement, &fairyYMovement);

			//background loops
			if (BACKGROUND_WIDTH - backgroundDistance < SCREEN_WIDTH) backgroundDistance = 0;
			//stalagtite loops
			if (SCREEN_WIDTH + STALAGTITE_SHIFT - stalagtiteDistance < -STALAGTITE_WIDTH) stalagtiteDistance = 0;
			//obstacle loops
			if (SCREEN_WIDTH + OBSTACLE_SHIFT - obstacleDistance < MINIMAL_Y) obstacleDistance = 0;
			//platform loops
			for (int i = 1; i < SHELVES_NUMBER; i++) {
				if (maxShift - shelfDistance[i] < -maxWidth) {
					shelfDistance[i] = 0;
					if (i == 1) {
						if (starDestroyed) starStreak++;
						else starStreak = 0;
						if (fairyCaught) fairyStreak++;
						else fairyStreak = 0;
						starDestroyed = 0;
						fairyCaught = 0;
						tempStar = rand() % STAR_NUMBER;
						tempFairy = rand() % FAIRY_NUMBER;
					}
					
				}
			}
			//create the background
			DrawSurface(screen, background, -backgroundDistance, 0);
			//create the unicorn
			unicornHitbox.x = 0;
			unicornHitbox.y = SCREEN_HEIGHT / 2 - UNICORN_HEIGHT / 2 - screenMovement;
			DrawSurface(screen, unicorn, 0, 
					(SCREEN_HEIGHT / 2 - UNICORN_HEIGHT / 2 - screenMovement));

			//create the stalagtite
			stalagtiteHitbox.x = SCREEN_WIDTH + STALAGTITE_SHIFT - stalagtiteDistance;
			stalagtiteHitbox.y = -MAX_Y + yMovement;
			DrawSurface(screen, stalagtite, SCREEN_WIDTH + STALAGTITE_SHIFT - stalagtiteDistance,
					-MAX_Y + yMovement);

			//create the floor
			for (int i = 0; i < SHELVES_NUMBER; i++) {
				shelfHitbox[i].x = shelfShift[i] - shelfDistance[i];
				shelfHitbox[i].y = SCREEN_HEIGHT - shelfElevation[i] + yMovement;
				DrawSurface(screen, shelf[i], shelfShift[i] - shelfDistance[i],
						SCREEN_HEIGHT - shelfElevation[i] + yMovement);
				//create obstacles on 1st and 2nd platforms
				if (i == 2) {
					obstacleHitbox[0].x = shelfShift[i] - shelfDistance[i] + OBSTACLE_SHIFT;
					obstacleHitbox[0].y = SCREEN_HEIGHT - shelfElevation[i] + yMovement - OBSTACLE_HEIGHT;
					DrawSurface(screen, obstacle[0], shelfShift[i] - shelfDistance[i] + OBSTACLE_SHIFT,
						SCREEN_HEIGHT - shelfElevation[i] + yMovement - OBSTACLE_HEIGHT);
				}
				else if (i == 3) {
					obstacleHitbox[1].x = shelfShift[i] - shelfDistance[i] + OBSTACLE_SHIFT;
					obstacleHitbox[1].y = SCREEN_HEIGHT - shelfElevation[i] + yMovement - OBSTACLE_HEIGHT;
					DrawSurface(screen, obstacle[1], shelfShift[i] - shelfDistance[i] + OBSTACLE_SHIFT,
						SCREEN_HEIGHT - shelfElevation[i] + yMovement - OBSTACLE_HEIGHT);
				}
				//randomly create stars
				if (i == starSpawn[tempStar] && !starDestroyed) {
					starHitbox.x = shelfShift[i] - shelfDistance[i] + STAR_SHIFT;
					starHitbox.y = SCREEN_HEIGHT - shelfElevation[i] + yMovement - STAR_HEIGHT;
					DrawSurface(screen, star, shelfShift[i] - shelfDistance[i] + STAR_SHIFT,
							SCREEN_HEIGHT - shelfElevation[i] + yMovement - STAR_HEIGHT);
				}
				//randomly create fairies
				if (i == fairySpawn[tempFairy] && !fairyCaught) {
					fairyHitbox.x = shelfShift[i] - shelfDistance[i] + FAIRY_SHIFT - fairyXMovement;
					fairyHitbox.y = SCREEN_HEIGHT - shelfElevation[i] + yMovement
						- FAIRY_HEIGHT - fairyYMovement - AREA_SIZE;
					DrawSurface(screen, fairy, shelfShift[i] - shelfDistance[i] + FAIRY_SHIFT - 
						fairyXMovement, SCREEN_HEIGHT - shelfElevation[i] + yMovement 
						- FAIRY_HEIGHT - fairyYMovement - AREA_SIZE);
				}
			}
			//create lives
			for (int i = 0; i < lives; i++) {
				DrawSurface(screen, life[i], SCREEN_WIDTH - LIFE_SHIFT * (i + 1), 4);
			}

			velocity += delta * HASTE;

			if (!dash)velocityCap = velocity;

			//info text
			DrawRectangle(screen, 4, 4, INFO_WIDTH, INFO_HEIGHT, czerwony, niebieski);
			sprintf(text, "Game time = %.1lf ", worldTime);
			sprintf(pointsString, "Points = %d ", (int)points);
			DrawString(screen, 12, 8, text, charset);
			DrawString(screen, 12, 26, pointsString, charset);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			//handling events if there are any
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case(SDL_QUIT):
					quit = 1;
					play = 0;
					break;
				case(SDL_KEYDOWN):
					//changing the controls
					if (event.key.keysym.sym == SDLK_d) {
						if (!controls)controls = 1;
						else controls = 0;
					}
					//dash
					if (event.key.keysym.sym == SDLK_x && controls && (worldRealTime - dashCd) > DASH_COOLDOWN) {
						dashCd = worldRealTime;
						dash = 1;
					}
					//jump
					if ((event.key.keysym.sym == SDLK_z && controls) || (event.key.keysym.sym == SDLK_UP && !controls))
					{
						jumping++;
						if(jumping==1)yCap = yPosition;
					}
					break;
				}
			}

			//getting the keyboard state
			const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
			//printf("%f\n", points);
			//reset the game after pressing n
			if (currentKeyStates[SDL_SCANCODE_N] || !lives) {
				ClearGame(&backgroundDistance, &obstacleDistance,
					&worldTime, &worldRealTime, &baseYPosition, &yPosition,
					&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
					shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
					&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
					&fairyStreak, &fairyCaught, &tempFairy);
				lives = 0;
				continue;
			}
			//quiting the game after esc is pressed (doesn't work for some reason)
			if (currentKeyStates[SDL_SCANCODE_ESCAPE]) quit = 1;
			//jak spadne pod mape to resetuje
			if (yPosition == MINIMAL_Y) {
				ClearGame(&backgroundDistance, &obstacleDistance,
					&worldTime, &worldRealTime, &baseYPosition, &yPosition,
					&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
					shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
					&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
					&fairyStreak, &fairyCaught, &tempFairy);
				continue;
			}
			//moving right
			if (currentKeyStates[SDL_SCANCODE_RIGHT] || controls) {
				backgroundDistance += velocity;
				obstacleDistance += velocity;
				for (int i = 0; i < SHELVES_NUMBER; i++) shelfDistance[i] += velocity;
				stalagtiteDistance += velocity;
				points += velocity/10;
			}
			//moving left
			if (currentKeyStates[SDL_SCANCODE_LEFT]) {
				backgroundDistance -= velocity;
				obstacleDistance -= velocity;
				for (int i = 0; i < SHELVES_NUMBER; i++) shelfDistance[i] -= velocity;
				stalagtiteDistance -= velocity;
				points -= velocity / 10;
			}
			//dash
			if (dash) {
				yVelocity = 0;
				//one-time speed boost
				if (dash == 1) {
					velocity += DASH_HASTE;
					dash++;
				}
				velocity -= DASH_HASTE_REDUCTION;
				//jump possible after dashing
				jumping = 3;
				//no double jump though
				if (yPosition > baseYPosition)doubleJump = 1;
			}
			//jump
			if (jumping && !dash) {
				//increasing height till reaching the peak
				if ((currentKeyStates[SDL_SCANCODE_Z]||currentKeyStates[SDL_SCANCODE_UP]) && !peak) {
					yVelocity += JUMP_HASTE;
				}
				//if too high then peak = true
				if (yPosition - yCap > JUMP_HEIGHT) peak = 1;
				//double jump
				if (jumping == 2) {
					yVelocity = 0;
					peak = 0;
					yCap = yPosition;
					jumping++;
				}
				//changing height
				yVelocity -= gravity * delta;
				yPosition += 2 * (yVelocity * delta);
			}

			//terminating dash after the speed reaches the base value
			if (velocity < velocityCap) {
				velocity = velocityCap;
				dash = 0;
			}

			//jump after dash
			if (doubleJump == 1 && jumping == 4)
			{
				yCap = yPosition;
				yVelocity = 0;
				peak = 0;
				doubleJump = 0;
				//making cooldown longer so dash is not possible until reaching the ground
				dashCd = worldRealTime + DASH_COOLDOWN;
			}

			//moment of teaching the ground
			if (yPosition < baseYPosition) {
				yPosition = baseYPosition;
				yVelocity = 0;
				jumping = 0;
				peak = 0;
			}

			//colliding with obstacles
			for (int i = 0; i < OBSTACLES_NUMBER; i++) {
				if (CheckCollision(unicornHitbox, obstacleHitbox[i], 'o')) {
					ClearGame(&backgroundDistance, &obstacleDistance,
						&worldTime, &worldRealTime, &baseYPosition, &yPosition,
						&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
						shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
						&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
						&fairyStreak, &fairyCaught, &tempFairy);
					continue;
				}
			}

			//colliding with stalagtites
			if (CheckCollision(unicornHitbox, stalagtiteHitbox, 'o')) {
				ClearGame(&backgroundDistance, &obstacleDistance,
					&worldTime, &worldRealTime, &baseYPosition, &yPosition,
					&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
					shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
					&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
					&fairyStreak, &fairyCaught, &tempFairy);
				continue;
			}

			//destroying the stars
			if (dash && !starDestroyed && CheckCollision(unicornHitbox, starHitbox, 'o') ) {
				starDestroyed = 1;
				points += (starStreak + 1) * STAR_POINTS;
			}
			//colliding with stars
			else if (!starDestroyed && CheckCollision(unicornHitbox, starHitbox, 'o')  ) {
				ClearGame(&backgroundDistance, &obstacleDistance,
					&worldTime, &worldRealTime, &baseYPosition, &yPosition,
					&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
					shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
					&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
					&fairyStreak, &fairyCaught, &tempFairy);
				continue;
			}
			//catching the fairy
			if (!fairyCaught && CheckCollision(unicornHitbox, fairyHitbox, 'o')) {
				fairyCaught = 1;
				points += (fairyStreak + 1) * FAIRY_POINTS;
			}

			walking = 0;
			//handling platforms
			for (int i = 0; i < SHELVES_NUMBER; i++) {
				//colliding with platform
				if (CheckCollision(unicornHitbox, shelfHitbox[i], 'p') == 1) {
					ClearGame(&backgroundDistance, &obstacleDistance,
						&worldTime, &worldRealTime, &baseYPosition, &yPosition,
						&dashCd, &dash, &screenMovement, &yMovement, &jumping, &peak,
						shelfDistance, &stalagtiteDistance, &velocity, &velocityCap,
						&yVelocity, &lives, &play, &starStreak, &starDestroyed, &tempStar,
						&fairyStreak, &fairyCaught, &tempFairy);
					continue;
				}
				//jumping onto the plotform
				if (CheckCollision(unicornHitbox, shelfHitbox[i], 'p') == 2) {
					baseYPosition = yPosition;
					walking = 1;
				}
				//hitting the platform from below
				if (CheckCollision(unicornHitbox, shelfHitbox[i], 'p') == 3) {
					yVelocity = -5;
					peak = 1;
				}
			}
			//if I fell out of the platform then im no longer waiting on it
			if (baseYPosition != MINIMAL_Y && !walking) {
				if (jumping == 0) {
					peak = 1;
					jumping = 1;
				}
				walking = 0;
				baseYPosition = MINIMAL_Y;
			}
		}
	}

	//free the memory
	if (LoadFile(&fairy, "./endgame.bmp", &screen, &charset,
		&unicorn, &platform, &background, obstacle, shelf,
		&stalagtite, life, &menu, &mainMenu,
		&star, &fairy, &scrtex, &window, &renderer)) return 0;
};
