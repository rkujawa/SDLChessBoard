/*
 * Needs SDL2 >= 2.0, SDL2_ttf >= 2.0.15.
 */

/*#define _WITH_FONTCONFIG*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <gc/gc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef _WITH_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif /* _WITH_FONTCONFIG */

#ifdef __linux__
#include <linux/limits.h>
#else
#include <sys/syslimits.h>
#endif

#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 900

enum Rank { RANK_8, RANK_7, RANK_6, RANK_5, RANK_4, RANK_3, RANK_2, RANK_1 };
enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

typedef enum Rank Rank;
typedef enum File File;

typedef bool PieceColor;
#define PIECE_COLOR_WHITE true
#define PIECE_COLOR_BLACK false

typedef struct Piece {
	char* name; 
	char* representation;	/* single null terminated UTF-8 character */
	PieceColor color;
	
	/* move pattern? */
	/* atack pattern? */

//	king, rook, bishop, queen, knight, and pawn
} Piece;

typedef struct Board {

	SDL_Rect area;

	/* ... pieces */
} Board;

//typedef struct ChessBoard {

bool kfcTextToTex(const char *, SDL_Texture **, SDL_Color, int);
void chessPieceRender(Piece, Rank, File);
void kfcRenderTex(SDL_Texture *, int, int);
static void LogRendererInfo(SDL_RendererInfo rendererInfo);
void drawEmptyBoard(Board *b);
void setupBoardDimensions(Board *b);



static SDL_Color white = { 255, 255, 255, 0 };
static SDL_Color black = { 0, 0, 0, 0 };

static SDL_Window *win;
static SDL_Renderer *ren;

static char *fontPath = NULL;


const Piece pieceWhiteKing = {
	"White King", "♔", PIECE_COLOR_WHITE
};
	

//static chessBoard b;

char *
getDefaultFont() 
{
#ifdef _WITH_FONTCONFIG 
	char *fontPathFinal;
	char *fontPathTmp;

	fontPathFinal = NULL;

	FcInit();
	FcConfig* config = FcInitLoadConfigAndFonts();

	FcPattern* pat = FcNameParse((const FcChar8*)"Chess Merida Unicode");

	FcConfigSubstitute(config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);

	FcResult result;

	FcPattern* font = FcFontMatch(config, pat, &result);

	if (font)
	{
		FcChar8* file = NULL; 

		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
		{
			size_t fontPathLen;

			fontPathTmp = (char*)file;
			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "FontConfig found font %s", fontPathTmp);
			fontPathLen = strnlen(fontPathTmp, PATH_MAX);
			fontPathFinal = GC_STRNDUP(fontPathTmp, fontPathLen);
			
		}
	}
	FcPatternDestroy(font);
	FcPatternDestroy(pat);
	FcConfigDestroy(config);
	FcFini();

	return fontPathFinal;
#else
	return "chess_merida_unicode.ttf"; /* expect found to be found in current directory */
#endif /* _WITH_FONTCONFIG */
}

int
main (int argc, char *argv[]) 
{
	char *c;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_Init %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	fontPath = getDefaultFont();
	if (fontPath == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Unable to load font, exiting.");
		return EXIT_FAILURE;
	}

	win = SDL_CreateWindow("chessboard", 100, 100, SCREEN_WIDTH, 
/*	    SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);	*/
	    SCREEN_HEIGHT, SDL_WINDOW_SHOWN);	 
	if (win == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "SDL_CreateWindow %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	ren = SDL_CreateRenderer(win, -1,  0
	    /* SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC*/);

	if (ren == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Unable to create renderer: %s", SDL_GetError());
		SDL_DestroyWindow(win);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_RendererInfo rendererInfo; 
	SDL_GetRendererInfo(ren, &rendererInfo);
	LogRendererInfo(rendererInfo);
		 	
	if (TTF_Init() == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Unable to init SDL_ttf: %s", TTF_GetError());
		return EXIT_FAILURE;
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_RenderClear(ren);

	Board b;
	setupBoardDimensions(&b);
	/* draw inital state of board */
	drawEmptyBoard(&b);
//	boardRedraw(b);

	chessPieceRender(pieceWhiteKing, 1, 2);
	SDL_RenderPresent(ren);


	while (1) {
		SDL_Event e;

		if (SDL_PollEvent(&e)) {

			if (e.type == SDL_QUIT)
				break;

			if (e.type == SDL_KEYDOWN) { 
				if (e.key.keysym.sym == SDLK_ESCAPE)
					break;

				//SDL_RenderClear(ren);
				//kfcRenderChar('b');

			
			}
		}
		//SDL_RenderPresent(ren);
	}

	TTF_Quit();	
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}

void chessPieceRender(Piece pc, Rank r, File f)
{
	static SDL_Texture *txttex;
	int iW, iH, x, y;

	if (!kfcTextToTex(pc.representation, &txttex, white, 300))
		printf("kfcTextToTex failed\n");

	SDL_QueryTexture(txttex, NULL, NULL, &iW, &iH);
	x = SCREEN_WIDTH / 2 - iW / 2;
	y = SCREEN_HEIGHT / 2 - iH / 2;
	kfcRenderTex(txttex, x, y);

}

bool kfcTextToTex(const char *msg, SDL_Texture **fonttex, SDL_Color color, 
    int fontsize)
{
	SDL_Surface *surf;
	TTF_Font *font;

	font = TTF_OpenFont(fontPath, fontsize);
	if (font == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "TTF_OpenFont %s", TTF_GetError());
		return false;
	}	

	surf = TTF_RenderUTF8_Blended(font, msg, color); /* render glyph instead of text? */

	if (surf == NULL) {
		TTF_CloseFont(font);
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "TTF_RenderText %s", TTF_GetError());
		return false;
	}

	*fonttex = SDL_CreateTextureFromSurface(ren, surf);
	if (fonttex == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "CreateTexture %s", SDL_GetError());
		return false;
	}

	TTF_CloseFont(font);
	SDL_FreeSurface(surf);
	return true;
}

void kfcRenderTex(SDL_Texture *tex, int x, int y)
{
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(ren, tex, NULL, &dst);
}

static void LogRendererInfo(SDL_RendererInfo rendererInfo) 
{ 
	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
	    "renderer: %s software=%d accelerated=%d presentvsync=%d targettexture=%d\n", 
	    rendererInfo.name, 
	    (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0 ); 
} 

void
setupBoardDimensions(Board *b) 
{
	SDL_Rect screenArea;

	uint16_t board_px_len;		/* length of the board in pixels */
	uint16_t board_x_start;		/* board starts at pixel X of screen */
	uint16_t board_y_start;		/* board starts at pixel Y of screen */

	/* obtain size of our screen/window */
	SDL_RenderGetViewport(ren, &screenArea);

	if (screenArea.w < screenArea.h)
		board_px_len = screenArea.w;
	else
		board_px_len = screenArea.h;

	board_x_start = screenArea.w / 2 - board_px_len / 2;
	board_y_start = screenArea.h / 2 - board_px_len / 2;

	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "x %d y %d w %d h %d board_len %d x_start %d y_start %d",
		screenArea.x, screenArea.y, screenArea.w, screenArea.h, board_px_len, board_x_start, board_y_start);

	b->area.x = board_x_start;
	b->area.y = board_y_start;
	b->area.w = board_px_len;
	b->area.h = board_px_len;
}

void 
drawEmptyBoard(Board *b) 
{
	int row = 0,column = 0,x = 0;
	SDL_Rect rect, darea;

	darea = b->area;

	/* Get the Size of drawing surface */
//	SDL_RenderGetViewport(ren, &darea);

	for( ; row < 8; row++) {
		column = row%2;
		x = column;
		for( ; column < 4+(row%2); column++) {
			SDL_SetRenderDrawColor(ren, 255, 0, 0, 0xFF);
			rect.w = darea.w/8;
			rect.h = darea.h/8;
			rect.x = (x * rect.w) + darea.x;
			rect.y = (row * rect.h) + darea.y;
			x = x + 2;
			SDL_RenderFillRect(ren, &rect);
		}
	}
}

