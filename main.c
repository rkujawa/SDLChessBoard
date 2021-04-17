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

#include <sys/syslimits.h>

#define SCREEN_WIDTH 1440
#define SCREEN_HEIGHT 900

bool kfcTextToTex(const char *, SDL_Texture **, SDL_Color, int);
void kfcRenderChar(const char);
void kfcRenderTex(SDL_Texture *, int, int);
static void LogRendererInfo(SDL_RendererInfo rendererInfo);

static char *fontPath = NULL;

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

//typedef struct ChessBoard {
	

static SDL_Color white = { 255, 255, 255 };
static SDL_Color black = { 0, 0, 0 };

static SDL_Window *win;
static SDL_Renderer *ren;


const Piece piece_white_king = {
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

	/*
	 * XXX: prehaps this could be configurable, it's not that chess is so extremely
	 * demanding game... we could allow for software renderer here.
	 */
	ren = SDL_CreateRenderer(win, -1, 
	    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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

	/* draw inital state of board */
//	boardRedraw(b);

	kfcRenderChar('a');
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

void kfcRenderChar(const char c)
{
	static SDL_Texture *txttex;
	int iW, iH, x, y;

	char charBuf[2];
	charBuf[0] = c;	
	charBuf[1] = '\0';

	if (!kfcTextToTex(charBuf, &txttex, white, 300))
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

//	surf = TTF_RenderText_Solid(font, msg, color);
	surf = TTF_RenderUTF8_Solid(font, piece_white_king.representation, color);
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

