CC=cc
SDL_LIB = -L/opt/local/lib -L/opt/X11/lib -lSDL2 -lSDL2_ttf -lgc
# -lfontconfig
SDL_INCLUDE = -I/opt/local/include/
#-I/opt/X11/include/
LDFLAGS = $(SDL_LIB)

all: chessboard

chessboard: main.o
	$(CC) $< $(LDFLAGS) -o $@

main.o: main.c
	$(CC) -c $(SDL_INCLUDE) $< 

clean:
	rm *.o && rm chessboard

