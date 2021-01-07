#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BACKGROUND_WIDTH 1366
#define PLATFORM_WIDTH 7000
#define PLATFORM_HEIGHT 100
#define UNICORN_WIDTH 40
#define UNICORN_HEIGHT 40
#define OBSTACLE_WIDTH 70
#define OBSTACLE_HEIGHT 40
#define SHELF_WIDTH 250
#define SHELF_HEIGHT 35
#define DASH_COOLDOWN 1000
#define DASH_HASTE 1.2
#define DASH_HASTE_REDUCTION 0.005
#define JUMP_HASTE 2.5
#define PEAK_HEIGHT 200
#define BASE_VELOCITY 0.3
#define MINIMAL_Y -400
#define EPSILON 5
#define SHELVES_NUMBER 3

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
                SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
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


//narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x; 
	dest.y = y; 
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};


//rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};


// rysowanie prostokπta o d≥ugoúci bokÛw l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

//wykrywanie kolizji
int CheckCollision(SDL_Rect first, SDL_Rect second, char type)
{
	//deklaruje boki prostokπtÛw
	int firstLeft, firstRight, firstTop, firstBottom;
	int secondLeft, secondRight, secondTop, secondBottom;

	//obliczam boki pierwszego hitboxa
	firstLeft = first.x;
	firstRight = first.x + first.w;
	firstTop = first.y;
	firstBottom = first.y + first.h;

	//obliczam boki drugiego hitboxa
	secondLeft = second.x;
	secondRight = second.x + second.w;
	secondTop = second.y;
	secondBottom = second.y + second.h;

	//jeúli ktÛryú bok pierwszego hitboxa jest poza drugim to nie ma kolizji
	if (firstBottom < secondTop) return 0;
	if (firstTop > secondBottom) return 0;
	if (firstRight < secondLeft) return 0;
	if (firstLeft > secondRight) return 0;
	//jeúli sprawdzam kolizje z platforma to rozpatruje przypadek wskoczenia na niπ
	//lub uderzenia w niπ od spodu
	//abs bo program moze przegapic moment zetkniecia z platforma
	if (type == 'p') {
		if (abs(firstBottom - secondTop) < EPSILON) return 2;
		if (abs(firstTop - secondBottom) < EPSILON) return 3;
	}

	//jeúli nie, to wystapila kolizja
	return 1;
}

void ClearGame(double* distance, double* distance2, double* distance3, double* distance4, double* worldTime, double* worldRealTime, double* baseYPosition, double* yPosition, int* dashCd, double* screenMovement, double* yMovement, int* jumping, int* peak) {
	*distance = 0;
	*distance2 = 0;
	*distance3 = 0;
	*distance4 = 0;
	*worldTime = 0;
	*worldRealTime = 0;
	*baseYPosition = MINIMAL_Y;
	*yPosition = 0;
	*dashCd = 0;
	*screenMovement = 0;
	*yMovement = 0;
	*jumping = 1;
	*peak = 1;
}

// main
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {
	//wypierdol distance2 i distance4 i w ogole sprawdz czy to wszystko useful jest
	int t1, t2, quit, frames, rc, jumping = 0, controls = 0, dash = 0, dashCd = 0, double_jump = 0, peak = 0, walking=0;
	const double gravity = 300, shelfWidth[3] = { 900, 700, 1200 }, shelfHeight[3] = { 40, 60, 50 }, shelfShift[3] = { 0, 700, 1200 }, shelfElevation[3] = { 100, 240, 380 };
	double delta, worldTime, worldRealTime, fpsTimer, fps, distance, distance2, distance3, distance4, shelfDistance[3] = { 0,0,0 }, velocity, yPosition, yVelocity, baseYPosition = 0, screenMovement = 0, yMovement = 0;
	SDL_Event event;
	SDL_Surface *screen, *charset, *unicorn, *platform, *background, *obstacle, *shelf[3], *hitbox, *hitbox2;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Rect unicorn_hitbox, obstacle_hitbox, platform_hitbox, shelf_hitbox[3];

	// okno konsoli nie jest widoczne, jeøeli chcemy zobaczyÊ
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmieniÊ na "Console"
	// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
		}

	// tryb pe≥noekranowy / fullscreen mode
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
		};
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Trzaskamy projekt jazdunia");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	//wy≥πczenie widocznoúci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	//wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};

	SDL_SetColorKey(charset, true, 0x000000);

	//≥aduje plik z unicornem
	unicorn = SDL_LoadBMP("./unicorn.bmp");
	if(unicorn == NULL) {
		printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};
	/*SDL_Surface **shelf = (SDL_Surface**)malloc(4 * sizeof(SDL_Surface*));
	for (int i = 0; i < SHELVES_NUMBER; i++) shelf[i] = (SDL_Surface*)malloc(sizeof(SDL_Surface));*/
	//≥aduje plik z platformπ
	for (int i = 0; i < SHELVES_NUMBER; i++) {
		shelf[i] = SDL_LoadBMP("./floor.bmp");
		if (shelf[i] == NULL) {
			printf("SDL_LoadBMP(floor.bmp) error: %s\n", SDL_GetError());
			SDL_FreeSurface(charset);
			SDL_FreeSurface(screen);
			SDL_DestroyTexture(scrtex);
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
			return 1;
		};
		/*shelf2 = SDL_LoadBMP("./obstacle.bmp");
		if (shelf1 == NULL) {
			printf("SDL_LoadBMP(floor.bmp) error: %s\n", SDL_GetError());
			SDL_FreeSurface(charset);
			SDL_FreeSurface(screen);
			SDL_DestroyTexture(scrtex);
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
			return 1;
		};
		shelf3 = SDL_LoadBMP("./obstacle.bmp");
		if (shelf1 == NULL) {
			printf("SDL_LoadBMP(floor.bmp) error: %s\n", SDL_GetError());
			SDL_FreeSurface(charset);
			SDL_FreeSurface(screen);
			SDL_DestroyTexture(scrtex);
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
			return 1;
		};*/
	}
	
	//≥aduje pokazywanie hitboxa
	hitbox = SDL_LoadBMP("./hitbox.bmp");
	if (hitbox == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	
	//≥aduje pokazywanie hitboxa
	hitbox2 = SDL_LoadBMP("./hitbox.bmp");
	if (hitbox2 == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	//wymiary widocznych hitboxow
	hitbox->h = SHELF_HEIGHT;
	hitbox->w = SHELF_WIDTH;

	hitbox2->h = UNICORN_HEIGHT;
	hitbox2->w = UNICORN_WIDTH;


	//wymiary unicorna
	unicorn->h = UNICORN_HEIGHT;
	unicorn->w = UNICORN_WIDTH;

	for (int i = 0; i < SHELVES_NUMBER; i++) {
		shelf[i]->h = shelfHeight[i];
		shelf[i]->w = shelfWidth[i];
		/*shelf2->h = shelfHeight[1];
		shelf2->w = shelfWidth[1];
		shelf3->h = shelfHeight[2];
		shelf3->w = shelfWidth[2];*/
	}
	
	//wymiary hitboxu unicorna
	unicorn_hitbox.h = UNICORN_HEIGHT;
	unicorn_hitbox.w = UNICORN_WIDTH;

	//wymiary hitboxu przeszkody
	obstacle_hitbox.h = OBSTACLE_HEIGHT;
	obstacle_hitbox.w = OBSTACLE_WIDTH;

	//wymiary hitboxu pod≥ogi
	for (int i = 0; i < SHELVES_NUMBER; i++) {
		shelf_hitbox[i].h = shelfHeight[i];
		shelf_hitbox[i].w = shelfWidth[i];
	}
	
	//≥aduje plik z przeszkoda
	obstacle = SDL_LoadBMP("./obstacle.bmp");
	if (obstacle == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	//przycinam przeszkodÍ
	obstacle->h = OBSTACLE_HEIGHT;
	obstacle->w = OBSTACLE_WIDTH;
	
	////≥aduje plik z pod≥ogπ
	//platform = SDL_LoadBMP("./floor.bmp");
	//if (platform == NULL) {
	//	printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
	//	SDL_FreeSurface(charset);
	//	SDL_FreeSurface(screen);
	//	SDL_DestroyTexture(scrtex);
	//	SDL_DestroyWindow(window);
	//	SDL_DestroyRenderer(renderer);
	//	SDL_Quit();
	//	return 1;
	//};

	//≥aduje plik z t≥em
	background = SDL_LoadBMP("./background.bmp");
	if (background == NULL) {
		system("pause");
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	//deklaruje kolory
	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	//zapusuje czas startu gry
	t1 = SDL_GetTicks();

	//ustawiam poczatkowe wartosci
	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	worldRealTime = 0;
	distance = 0;
	distance2 = 0;
	distance3 = 0;
	distance4 = 0;
	velocity = BASE_VELOCITY;
	yPosition = baseYPosition;
	yVelocity = 0;
	jumping = 1;
	peak = 1;

	while (!quit) {

		//czas od ostatniego narysowania ekranu
		t2 = SDL_GetTicks();
		delta = (t2 - t1);

		//czas w milisekundach
		worldRealTime += delta;

		//zmieniam na sekundy
		delta *= 0.001;
		t1 = t2;

		//czas w sekundach
		worldTime += delta;

		//obs≥ugujÍ pracÍ kamery
		if (yPosition > 0) {
			yMovement = fmin(yPosition, 250);
			screenMovement = fmax(0, yPosition - 250);
		}
		else if (yPosition < 0) {
			yMovement = fmax(yPosition, -250);
			screenMovement = fmin(0, yPosition + 250);
		}

		//hitbox unicorna
		unicorn_hitbox.x = 0;
		unicorn_hitbox.y = SCREEN_HEIGHT / 2 - UNICORN_HEIGHT / 2 - screenMovement;

		//hitbox przeszkÛd
		obstacle_hitbox.x = SCREEN_WIDTH + 100 - distance3;
		obstacle_hitbox.y = SCREEN_HEIGHT - OBSTACLE_HEIGHT - PLATFORM_HEIGHT + yMovement;

		//hitboxy platform
		for (int i = 0; i < 3; i++) {
			shelf_hitbox[i].x = shelfShift[i]-shelfDistance[i];
			shelf_hitbox[i].y = SCREEN_HEIGHT - PLATFORM_HEIGHT - shelfElevation[i] + yMovement;
		}
		//wype≥niam ekran czarnym
		SDL_FillRect(screen, NULL, czarny);

		//t≥o sie zapetla
		if (distance > BACKGROUND_WIDTH - SCREEN_WIDTH) distance = 0;
		//podloga sie zapetla
		//if (distance2 > PLATFORM_WIDTH - SCREEN_WIDTH) distance2 = 0;
		//przeszkoda sie zapetla
		if (SCREEN_WIDTH + 100 - distance3 < MINIMAL_Y) distance3 = 0;
		//platformy sie zapetlaja
		//if (SCREEN_WIDTH + 100 - distance4 < -1200) distance4 = 0;
		//if (700 + SCREEN_WIDTH > shelfDistance[0]) shelfDistance[0] == 0;
		for (int i = 0; i < SHELVES_NUMBER; i++) {
			if (shelfShift[i] - shelfDistance[i] < -shelfWidth[i])shelfDistance[i]=0;
		}
		//tworze t≥o
		DrawSurface(screen, background, -distance, 0);
		//tworze unicorna
		DrawSurface(screen, unicorn, 0, (SCREEN_HEIGHT/2 - UNICORN_HEIGHT/2 - screenMovement));
		//tworze przeszkode
		DrawSurface(screen, obstacle, SCREEN_WIDTH + 100 - distance3, SCREEN_HEIGHT - OBSTACLE_HEIGHT - PLATFORM_HEIGHT + yMovement);
		//tworze podloge
		for (int i = 0; i < SHELVES_NUMBER; i++) {
			DrawSurface(screen, shelf[i], shelfShift[i] - shelfDistance[i], SCREEN_HEIGHT - PLATFORM_HEIGHT - shelfElevation[i] + yMovement);
			//DrawSurface(screen, shelf2, shelfShift[1] - shelfDistance[1], SCREEN_HEIGHT - PLATFORM_HEIGHT - shelfElevation[1] + yMovement);
			//DrawSurface(screen, shelf3, shelfShift[2] - shelfDistance[2], SCREEN_HEIGHT - PLATFORM_HEIGHT - shelfElevation[2] + yMovement);
		}
		
		//wyúwietlam hitboxy i troche useless protokπtÛw
		//DrawSurface(screen, hitbox, shelf_hitbox[2].x, shelf_hitbox[2].y);
		//DrawSurface(screen, hitbox, platform_hitbox.x, platform_hitbox.y);
		//DrawSurface(screen, hitbox2, unicorn_hitbox.x, unicorn_hitbox.y);

		//liczneie fpsow
		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		// tekst informacyjny / info text
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH / 4.5, SCREEN_HEIGHT / 30, czerwony, niebieski);
		sprintf(text, "Czas gry = %.1lf ", worldTime);
		DrawString(screen, 12, 8, text, charset);
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		//SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		//obs≥uga zdarzeÒ (o ile jakieú zasz≥y)
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case(SDL_QUIT):
				quit = 1;
				break;
			case(SDL_KEYDOWN):
				//zmiana sterowania
				if (event.key.keysym.sym == SDLK_d) {
					if (!controls)controls = 1;
					else controls = 0;
				}
				//dash
				if (event.key.keysym.sym == SDLK_x && controls && (worldRealTime - dashCd) > DASH_COOLDOWN) {
					dashCd = worldRealTime;
					dash = 1;
				}
				//skok
				if (event.key.keysym.sym == SDLK_z && controls) {
					jumping++;
				}
				break;
			}
		}
		
		//pobieram stan klawiszy
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

		//resetuje gre
		if (currentKeyStates[SDL_SCANCODE_N] || yPosition == MINIMAL_Y) {
			ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement, &yMovement, &jumping, &peak);
		}
		if (currentKeyStates[SDL_SCANCODE_ESCAPE]) quit = 1;
		
		if (controls) {
			distance += velocity;
			distance2 += velocity;
			distance3 += velocity;
			distance4 += velocity;
			for (int i = 0; i < 3; i++) shelfDistance[i] += velocity;
		}

		if (dash) {
			yVelocity = 0;
			if (dash == 1) {
				velocity += DASH_HASTE;
				dash++;
			}
			velocity -= DASH_HASTE_REDUCTION;
			jumping = 3;
			if (yPosition > baseYPosition)double_jump = 1;
		}

		if (jumping && !dash) {
			if (currentKeyStates[SDL_SCANCODE_Z] && !peak) {
				yVelocity += JUMP_HASTE;
			}
			if (yVelocity > PEAK_HEIGHT) peak = 1;
			if (jumping == 2) {
				yVelocity = 0;
				peak = 0;
				jumping++;
			}
			yVelocity -= gravity * delta;
			yPosition += 2 * (yVelocity * delta);
			screenMovement = fmax(0, yPosition - 250);
		}

		if (velocity < 0.29) {
			velocity = BASE_VELOCITY;
			dash = 0;
		}

		if (double_jump == 1 && jumping == 4)
		{
			yVelocity = 0;
			peak = 0;
			double_jump = 0;
			dashCd = worldRealTime + DASH_COOLDOWN;
		}
		
		if (yPosition < baseYPosition) {
			screenMovement = fmax(0, yPosition - 250);
			yPosition = baseYPosition;
			yVelocity = 0;
			jumping = 0;
			peak = 0;
		}
		

		if (CheckCollision(unicorn_hitbox, obstacle_hitbox, 'o')) {
			ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement, &yMovement, &jumping, &peak);
		}

		//printf("%d\t%d\t%d\n", CheckCollision(unicorn_hitbox, shelf_hitbox[0], 'p'), CheckCollision(unicorn_hitbox, shelf_hitbox[1], 'p'), CheckCollision(unicorn_hitbox, shelf_hitbox[2], 'p'));
		//printf("%d\n", jumping);
		printf("%f\t%f\t%f\t%f\t%d\n", yPosition, baseYPosition, screenMovement, yMovement, CheckCollision(unicorn_hitbox, shelf_hitbox[2], 'p'));
		walking = 0;
		for (int i = 0; i < 3; i++) {
			if (CheckCollision(unicorn_hitbox, shelf_hitbox[i], 'p') == 1) {
				ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement, &yMovement, &jumping, &peak);
			}

			if (CheckCollision(unicorn_hitbox, shelf_hitbox[i], 'p') == 2) {
				baseYPosition = yPosition;
				walking=1;	
			}
		
			if (CheckCollision(unicorn_hitbox, shelf_hitbox[i], 'p') == 3) {
				yVelocity = -5;
				peak = 1;
			}
		}
		if (baseYPosition != MINIMAL_Y && !walking) {
			if (jumping == 0) {
				peak = 1;
				jumping = 1;
			}
			walking = 0;
			baseYPosition = MINIMAL_Y;
		}
			frames++;
	};
	

	// zwolnienie powierzchni / freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};
