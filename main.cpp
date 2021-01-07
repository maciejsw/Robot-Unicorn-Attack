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


// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
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


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};


// rysowanie prostok¹ta o d³ugoœci boków l i k
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
	//deklaruje boki prostok¹tów
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

	//jeœli któryœ bok pierwszego hitboxa jest poza drugim to nie ma kolizji
	if (firstBottom < secondTop) return 0;
	if (firstTop > secondBottom) return 0;
	if (firstRight < secondLeft) return 0;
	if (firstLeft > secondRight) return 0;
	//jeœli sprawdzam kolizje z platforma to rozpatruje przypadek wskoczenia na ni¹
	//lub uderzenia w ni¹ od spodu
	//abs bo program moze przegapic moment zetkniecia z platforma
	if (type == 'p') {
		if (abs(firstBottom - secondTop) < EPSILON) return 2;
		if (abs(firstTop - secondBottom) < EPSILON) return 3;
	}

	//jeœli nie, to wystapila kolizja
	return 1;
}

void ClearGame(double* distance, double* distance2, double* distance3, double* distance4, double* worldTime, double* worldRealTime, double* baseYPosition, double* yPosition, int* dashCd, double* screenMovement) {
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
}

// main
#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc, jumping = 0, controls = 0, dash = 0, dashCd = 0, double_jump = 0, peak = 0, walking=0;
	const double gravity = 300;
	double delta, worldTime, worldRealTime, fpsTimer, fps, distance, distance2, distance3, distance4, velocity, yPosition, yVelocity, baseYPosition = 0, screenMovement = 0, yMovement = 0;
	SDL_Event event;
	SDL_Surface *screen, *charset, *unicorn, *platform, *background, *obstacle, *shelf, *hitbox, *hitbox2;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Rect unicorn_hitbox, obstacle_hitbox, platform_hitbox, shelf_hitbox[3];

	// okno konsoli nie jest widoczne, je¿eli chcemy zobaczyæ
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmieniæ na "Console"
	// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
		}

	// tryb pe³noekranowy / fullscreen mode
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

	//wy³¹czenie widocznoœci kursora myszy
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

	//³aduje plik z unicornem
	unicorn = SDL_LoadBMP("./unicorn.bmp");
	if(unicorn == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};

	//³aduje plik z platform¹
	shelf = SDL_LoadBMP("./floor.bmp");
	if ( shelf == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	//³aduje pokazywanie hitboxa
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
	
	//³aduje pokazywanie hitboxa
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

	//wymiary hitboxu unicorna
	unicorn_hitbox.h = UNICORN_HEIGHT;
	unicorn_hitbox.w = UNICORN_WIDTH;

	//wymiary hitboxu przeszkody
	obstacle_hitbox.h = OBSTACLE_HEIGHT;
	obstacle_hitbox.w = OBSTACLE_WIDTH;

	/*platform_hitbox.h = PLATFORM_HEIGHT;
	platform_hitbox.w = PLATFORM_WIDTH;*/

	//wymiary hitboxu pod³ogi
	shelf_hitbox[0].h = PLATFORM_HEIGHT;
	shelf_hitbox[0].w = PLATFORM_WIDTH;
	
	//wymiary hitboxów platform
	for (int i = 1; i < 3; i++) {
		shelf_hitbox[i].h = SHELF_HEIGHT;
		shelf_hitbox[i].w = SHELF_WIDTH;
	}
	
	//³aduje plik z przeszkoda
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

	//przycinam przeszkodê
	obstacle->h = 40;
	obstacle->w = 70;
	
	//³aduje plik z pod³og¹
	platform = SDL_LoadBMP("./floor.bmp");
	if (platform == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	//³aduje plik z t³em
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

		//obs³ugujê pracê kamery
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
		unicorn_hitbox.y = SCREEN_HEIGHT - UNICORN_HEIGHT - PLATFORM_HEIGHT - screenMovement;

		//hitbox przeszkód
		obstacle_hitbox.x = SCREEN_WIDTH + 100 - distance3;
		obstacle_hitbox.y = SCREEN_HEIGHT - OBSTACLE_HEIGHT - PLATFORM_HEIGHT + yMovement;

		//hitboxy platform
		shelf_hitbox[0].x = -distance4;
		shelf_hitbox[0].y = SCREEN_HEIGHT - PLATFORM_HEIGHT - 150 + yMovement;
		shelf_hitbox[1].x = 800 - distance4;
		shelf_hitbox[1].y = SCREEN_HEIGHT - PLATFORM_HEIGHT - 240 + yMovement;
		shelf_hitbox[2].x = 1400 - distance4;
		shelf_hitbox[2].y = SCREEN_HEIGHT - PLATFORM_HEIGHT - 380 + yMovement;

		//wype³niam ekran czarnym
		SDL_FillRect(screen, NULL, czarny);

		//t³o sie zapetla
		if (distance > BACKGROUND_WIDTH - SCREEN_WIDTH) distance = 0;
		//podloga sie zapetla
		if (distance2 > PLATFORM_WIDTH - SCREEN_WIDTH) distance2 = 0;
		//przeszkoda sie zapetla
		if (SCREEN_WIDTH + 100 - distance3 < MINIMAL_Y) distance3 = 0;
		//platformy sie zapetlaja
		if (SCREEN_WIDTH + 100 - distance4 < -1200) distance4 = 0;

		//tworze t³o
		DrawSurface(screen, background, -distance, 0);
		//tworze unicorna
		DrawSurface(screen, unicorn, 0, (SCREEN_HEIGHT - UNICORN_HEIGHT - PLATFORM_HEIGHT - screenMovement));
		//tworze podloge
		DrawSurface(screen, platform, -distance2, SCREEN_HEIGHT - PLATFORM_HEIGHT + yMovement);
		//tworze przeszkode
		DrawSurface(screen, obstacle, SCREEN_WIDTH + 100 - distance3, SCREEN_HEIGHT - OBSTACLE_HEIGHT - PLATFORM_HEIGHT + yMovement);
		//tworze platforme1
		DrawSurface(screen, shelf, 800 - distance4, SCREEN_HEIGHT - PLATFORM_HEIGHT - 150 + yMovement);
		//tworze platforme2
		DrawSurface(screen, shelf, 1400 - distance4, SCREEN_HEIGHT - PLATFORM_HEIGHT - 320 + yMovement);
		
		//wyœwietlam hitboxy i troche useless protok¹tów
		DrawSurface(screen, hitbox, shelf_hitbox[2].x, shelf_hitbox[2].y);
		//DrawSurface(screen, hitbox, platform_hitbox.x, platform_hitbox.y);
		DrawSurface(screen, hitbox2, unicorn_hitbox.x, unicorn_hitbox.y);

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

		//obs³uga zdarzeñ (o ile jakieœ zasz³y)
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
			ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement);
		}
		if (currentKeyStates[SDL_SCANCODE_ESCAPE]) quit = 1;
		
		if (controls) {
			distance += velocity;
			distance2 += velocity;
			distance3 += velocity;
			distance4 += velocity;
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
			ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement);
		}

		//printf("%d\t%d\t%d\n", CheckCollision(unicorn_hitbox, shelf_hitbox[0], 'p'), CheckCollision(unicorn_hitbox, shelf_hitbox[1], 'p'), CheckCollision(unicorn_hitbox, shelf_hitbox[2], 'p'));
		//printf("%d\n", jumping);
		printf("%f\t%f\t%f\t%f\t%d\n", yPosition, baseYPosition, screenMovement, yMovement, CheckCollision(unicorn_hitbox, shelf_hitbox[2], 'p'));
		walking = 0;
		for (int i = 0; i < 3; i++) {
			if (CheckCollision(unicorn_hitbox, shelf_hitbox[i], 'p') == 1) {
				ClearGame(&distance, &distance2, &distance3, &distance4, &worldTime, &worldRealTime, &baseYPosition, &yPosition, &dashCd, &screenMovement);
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
