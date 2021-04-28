/*
 * Needs SDL2 >= 2.0, SDL2_ttf >= 2.0.15.
 */

/*#define _WITH_FONTCONFIG*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <gc/gc.h>

#include <SDL.h>
#include <SDL_ttf.h>

#ifdef _WITH_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif /* _WITH_FONTCONFIG */

#ifdef __linux__
#include <linux/limits.h>
#else
#include <sys/syslimits.h>
#endif

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800

#define CHESSBOARD_SQUARES		64		/* chess board has 64 squares */
#define CHESSBOARD_SQ_LENGTH	8		/* chess board is 8 squares length */

enum Rank { RANK_8, RANK_7, RANK_6, RANK_5, RANK_4, RANK_3, RANK_2, RANK_1 };
enum File { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };

typedef enum Rank Rank;
typedef enum File File;

typedef bool PieceColor;
#define PIECE_COLOR_WHITE true
#define PIECE_COLOR_BLACK false

typedef struct Piece {
	char *name; 
	char *representation;	/* single null terminated UTF-8 character */
	char *representationSelected;

	PieceColor color;
	
	/* move pattern? */
	/* atack pattern? */

} Piece;

typedef struct SquareState {
	bool isSquareSelected;
	bool isPieceSelected;
	const Piece *p;
} SquareState;

typedef struct Board {
	SDL_Rect area;				/* board dimensions and position on the screen */
	const SDL_Color *fgColor;
	const SDL_Color *bgColor;

	uint16_t squareLen;			/* length of square cell of the board */

	bool flipped;				/* is board view flipped? */
	uint8_t selectedSquare;		/* currently selected square of the board */

	//const Piece **pos;			/* array of pieces on the board */
	SquareState **pos;
} Board;

bool chessPieceToTex(const char *, SDL_Texture **, SDL_Color, int);
void chessPieceRender(Board *, Piece, Rank, File, bool);
void renderTex(SDL_Texture *, int, int);
static void LogRendererInfo(SDL_RendererInfo);
void boardRedrawEmpty(Board *);
void boardRedraw(Board *);
void boardSetupGraphics(Board *);
bool boardInit(Board *);
void boardSquareRedraw(Board *, Rank, File);
bool controllerInit();

static inline char rankToChar(Rank);
static inline char fileToChar(File);


static const SDL_Color white = { 255, 255, 255, 0 };
static const SDL_Color black = { 0, 0, 0, 0 };
static const SDL_Color selectedBg = { 57, 182, 191, 0 };
static const SDL_Color bgColor = { 179, 122, 43, 0 };
static const SDL_Color fgColor = { 219, 148, 48, 0 };

static SDL_Window *win;
static SDL_Renderer *ren;

static SDL_GameController *gamepad = NULL;

static char *fontPath = NULL;

const Piece pieceWhiteKing = {
	"White King", "♚", "♔", PIECE_COLOR_WHITE
};
const Piece pieceWhiteQueen = {
	"White Queen", "♛", "♕", PIECE_COLOR_WHITE
};
const Piece pieceWhiteRook = {
	"White Rook", "♜", "♖", PIECE_COLOR_WHITE
};
const Piece pieceWhiteBishop = {
	"White Bishop", "♝", "♗", PIECE_COLOR_WHITE
};
const Piece pieceWhiteKnight = {
	"White Knight", "♞", "♘", PIECE_COLOR_WHITE
};
const Piece pieceWhitePawn = {
	"White Pawn", "♟️", "♙", PIECE_COLOR_WHITE
};

const Piece pieceBlackKing = {
	"Black King", "♚", "♔", PIECE_COLOR_BLACK
};
const Piece pieceBlackQueen = {
	"Black Queen", "♛", "♕", PIECE_COLOR_BLACK
};
const Piece pieceBlackRook = {
	"Black Rook", "♜", "♖", PIECE_COLOR_BLACK
};
const Piece pieceBlackBishop = {
	"Black Bishop", "♝", "♗", PIECE_COLOR_BLACK
};
const Piece pieceBlackKnight = {
	"Black Knight", "♞", "♘", PIECE_COLOR_BLACK
};
const Piece pieceBlackPawn = {
	"Black Pawn", "♟️", "♙", PIECE_COLOR_BLACK
};

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
	return "chess_merida_unicode.ttf"; /* expected to be found in current directory */
#endif /* _WITH_FONTCONFIG */
}

bool
controllerInit()
{
	int maxJoys;
	int joyIdx;
	const char* name;

	maxJoys = SDL_NumJoysticks();
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "controllerInit: we have %d joysticks\n", maxJoys);

	for(joyIdx = 0; joyIdx < maxJoys; ++joyIdx) {
		if (!SDL_IsGameController(joyIdx)) {
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "controllerInit: not a GameControler at idx %d", joyIdx);
		} else {
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "controllerInit: GameController at idx %d", joyIdx);
			gamepad = SDL_GameControllerOpen(joyIdx);
			name = SDL_GameControllerNameForIndex(joyIdx);
		}
	}

	if (gamepad) {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "controllerInit: %s", name);
		return true;
	}

	return false;
}

int
main(int argc, char *argv[]) 
{
	GC_INIT();

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_Init %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);

	controllerInit();

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
	boardInit(&b);
	boardSetupGraphics(&b);
	/* draw inital state of board */
	boardRedraw(&b);

	b.pos[0]->isSquareSelected = true; /* test redraw of selected square */
	/* redraw some squares */
						/* 0,	  0	*/
	boardSquareRedraw(&b, RANK_8, FILE_A);
						/* 7,	  0 */
	boardSquareRedraw(&b, RANK_1, FILE_F);
	boardSquareRedraw(&b, RANK_4, FILE_C);
	boardSquareRedraw(&b, RANK_1, FILE_A);


	SDL_RenderPresent(ren);

	bool end;
	SDL_Event e;

	end = false;

	while (!end && SDL_WaitEvent(&e)) {

		switch (e.type) {

			case SDL_QUIT:
				end = true;
				break;

			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
					end = true;
				break;

			case SDL_CONTROLLERBUTTONDOWN:
				//if (e.cbutton.button == SDL_CONTROLLER_BUTTON_X)
					end = true;
				break;

			default:
				break;
		}
		//SDL_RenderPresent(ren);
	}


	TTF_Quit();	
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	if (gamepad)
		SDL_GameControllerClose(gamepad);

	SDL_Quit();

	return 0;
}

void 
chessPieceRender(Board *b, Piece pc, Rank r, File f, bool isSelected)
{
	static SDL_Texture *txttex;
	int iW, iH, x, y;

	/* could be handled more elegantly if PieceColor was a struct */
	const SDL_Color *pieceColor;

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Rendering %s at r %d f %d %c%c", 
	    pc.name, r, f, rankToChar(r), fileToChar(f));

	if (pc.color == PIECE_COLOR_WHITE)
		pieceColor = &white;
	else
		pieceColor = &black;

	if (!chessPieceToTex(pc.representation, &txttex, *pieceColor, b->squareLen)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "chessPieceToTex failed\n");
	}

	SDL_QueryTexture(txttex, NULL, NULL, &iW, &iH);
	x = b->area.x + (b->squareLen * f);
	y = b->area.y + (b->squareLen * r);
	renderTex(txttex, x, y);

}

bool 
chessPieceToTex(const char *msg, SDL_Texture **fonttex, SDL_Color color, 
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

void 
renderTex(SDL_Texture *tex, int x, int y)
{
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(ren, tex, NULL, &dst);
}

static void 
LogRendererInfo(SDL_RendererInfo rendererInfo) 
{ 
	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
	    "renderer: %s software=%d accelerated=%d presentvsync=%d targettexture=%d\n", 
	    rendererInfo.name, 
	    (rendererInfo.flags & SDL_RENDERER_SOFTWARE) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_ACCELERATED) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0, 
	    (rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) != 0 ); 
} 

static inline uint8_t
pieceRankFileToBoardPos(Rank r, File f)
{
	return r * CHESSBOARD_SQ_LENGTH + f;
}

static inline Rank
pieceBoardPosToRank(uint8_t bp)
{
	return bp / CHESSBOARD_SQ_LENGTH;
}

static inline File
pieceBoardPosToFile(uint8_t bp)
{
	return bp % CHESSBOARD_SQ_LENGTH;
}

static inline char
rankToChar(Rank r)
{
	return '0' + (CHESSBOARD_SQ_LENGTH - r);
}

static inline char
fileToChar(File f)
{
	return 'a' + f;
}

static void
piecePlaceOnBoard(Board *b, const Piece *p, Rank r, File f)
{
	uint8_t bIdx;
	bIdx = pieceRankFileToBoardPos(r,f);

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Placing piece %s r %d f %d at board array idx %d",
		p->name, r, f, bIdx);

	b->pos[bIdx]->p = p;
}

bool
boardInit(Board *b)
{
	uint8_t i;

	b->pos = GC_MALLOC(sizeof(SquareState *) * CHESSBOARD_SQUARES);

	for (i = 0; i < CHESSBOARD_SQUARES; i++) {
		b->pos[i] = GC_MALLOC(sizeof(SquareState));
	}

	if(b->pos == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Could not allocate memory");
		return false;
	}

	b->selectedSquare = pieceRankFileToBoardPos(RANK_8, FILE_A);

	piecePlaceOnBoard(b, &pieceBlackRook, RANK_8, FILE_A);
	piecePlaceOnBoard(b, &pieceBlackKnight, RANK_8, FILE_B); 
	piecePlaceOnBoard(b, &pieceBlackBishop, RANK_8, FILE_C);
	piecePlaceOnBoard(b, &pieceBlackQueen, RANK_8, FILE_D);
	piecePlaceOnBoard(b, &pieceBlackKing, RANK_8, FILE_E);
	piecePlaceOnBoard(b, &pieceBlackBishop, RANK_8, FILE_F);
	piecePlaceOnBoard(b, &pieceBlackKnight, RANK_8, FILE_G);
	piecePlaceOnBoard(b, &pieceBlackRook, RANK_8, FILE_H);

	for (i = 0; i < CHESSBOARD_SQ_LENGTH; i++)
		piecePlaceOnBoard(b, &pieceBlackPawn, RANK_7, FILE_A+i);

	piecePlaceOnBoard(b, &pieceWhiteRook, RANK_1, FILE_A);
	piecePlaceOnBoard(b, &pieceWhiteKnight, RANK_1, FILE_B);
	piecePlaceOnBoard(b, &pieceWhiteBishop, RANK_1, FILE_C);
	piecePlaceOnBoard(b, &pieceWhiteQueen, RANK_1, FILE_D);
	piecePlaceOnBoard(b, &pieceWhiteKing, RANK_1, FILE_E);
	piecePlaceOnBoard(b, &pieceWhiteBishop, RANK_1, FILE_F);
	piecePlaceOnBoard(b, &pieceWhiteKnight, RANK_1, FILE_G);
	piecePlaceOnBoard(b, &pieceWhiteRook, RANK_1, FILE_H);

	for (i = 0; i < CHESSBOARD_SQ_LENGTH; i++)
		piecePlaceOnBoard(b, &pieceWhitePawn, RANK_2, FILE_A+i);

	return true;
}

void
boardSetupGraphics(Board *b) 
{
	SDL_Rect screenArea;

	uint16_t boardPxLen;		/* length of the board in pixels */

	b->bgColor = &bgColor;
	b->fgColor = &fgColor;

	/* obtain size of our screen/window */
	SDL_RenderGetViewport(ren, &screenArea);

	/* assume that board rectangular, so take smaller of screen dimensions */
	if (screenArea.w < screenArea.h)
		boardPxLen = screenArea.w;
	else
		boardPxLen = screenArea.h;

	/* calculate x, y so that board can be placed on the center of the screen */
	b->area.x = screenArea.w / 2 - boardPxLen / 2;
	b->area.y = screenArea.h / 2 - boardPxLen / 2;

	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "setupBoard: x %d y %d w %d h %d board_len %d x_start %d y_start %d",
		screenArea.x, screenArea.y, screenArea.w, screenArea.h, boardPxLen, b->area.x, b->area.y);

	b->area.w = boardPxLen;
	b->area.h = boardPxLen;

	b->squareLen = b->area.w / CHESSBOARD_SQ_LENGTH;	/* again, we assume that w=h */

}

/**
 * Draw the board and all pieces that are on the board.
 */
void
boardRedraw(Board *b)
{
	uint8_t i;
	Rank r;
	File f;

	boardRedrawEmpty(b);

	/* Iterate over all squares of board and draw pieces where appropripate. */
	for (i = 0; i < CHESSBOARD_SQUARES; i++) {
		if (b->pos[i]->p != NULL) {
			r = pieceBoardPosToRank(i);
			f = pieceBoardPosToFile(i);

			chessPieceRender(b, *(b->pos[i]->p), r, f, false);
		}
	}
}

void
boardSquareRedraw(Board *b, Rank r, File f)
{
	SDL_Rect sqRect;
	uint8_t bIdx;

	bIdx = pieceRankFileToBoardPos(r, f);

	if (b->pos[bIdx]->isSquareSelected)
		SDL_SetRenderDrawColor(ren, selectedBg.r, selectedBg.g, selectedBg.b, selectedBg.a);
	else if (bIdx%2)
		SDL_SetRenderDrawColor(ren, b->bgColor->r, b->bgColor->g, b->bgColor->b, b->bgColor->a);
	else
		SDL_SetRenderDrawColor(ren, b->fgColor->r, b->fgColor->g, b->fgColor->b, b->fgColor->a);

	sqRect.w = b->squareLen;
	sqRect.h = b->squareLen;
	sqRect.x = (f * sqRect.w) + b->area.x;
	sqRect.y = (r * sqRect.h) + b->area.y;

	if (b->pos[bIdx]->p != NULL)
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Redrawing square %s at r %d f %d (%c%c) (bIdx %d)", 
	        b->pos[bIdx]->p->name, r, f, rankToChar(r), fileToChar(f), bIdx);
	else
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Redrawing empty square at r %d f %d (%c%c) (bIdx %d)", 
	        r, f, rankToChar(r), fileToChar(f), bIdx);

	SDL_RenderFillRect(ren, &sqRect);

	if (b->pos[bIdx]->p != NULL)
		chessPieceRender(b, *(b->pos[bIdx]->p), r, f, b->pos[bIdx]->isPieceSelected);
}

/* inspired by one of the examples distributed with SDL */
void 
boardRedrawEmpty(Board *b) 
{
	int row = 0, column = 0, x = 0;
	SDL_Rect sqRect;

	/* draw background */
	SDL_SetRenderDrawColor(ren, b->bgColor->r, b->bgColor->g, b->bgColor->b, b->bgColor->a);
	SDL_RenderFillRect(ren, &(b->area));

	sqRect.w = b->squareLen;
	sqRect.h = b->squareLen;

	/* draw squares in foreground color */
	for( ; row < CHESSBOARD_SQ_LENGTH; row++) {
		column = row%2;
		x = column;
		for( ; column < 4+(row%2); column++) {
			SDL_SetRenderDrawColor(ren, b->fgColor->r, b->fgColor->g, b->fgColor->b, b->fgColor->a);
			sqRect.x = (x * sqRect.w) + b->area.x;
			sqRect.y = (row * sqRect.h) + b->area.y;
			x += 2;
			SDL_RenderFillRect(ren, &sqRect);
		}
	}
}

