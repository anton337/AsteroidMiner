
all: Perlin.o FractalNoise.o
	g++ -Werror -m64 -g0 -O4 main.cpp Perlin.o FractalNoise.o -lglut -lGLEW -lGL -lGLU -lpthread; ./a.out;
	#g++ -Werror -m64 -g0 -O4 main.cpp Perlin.o FractalNoise.o -I/usr/include/SDL -lglut -lGLEW -lGL -lGLU `sdl-config --cflags --libs` -lpthread; ./a.out;

test: Perlin.o FractalNoise.o
	g++ -Werror -m64 -g3 -O0 main.cpp Perlin.o FractalNoise.o -lglut -lGLEW -lGL -lGLU -lpthread
	#g++ -Werror -m64 -g3 -O0 main.cpp Perlin.o FractalNoise.o -I/usr/include/SDL -lglut -lGLEW -lGL -lGLU `sdl-config --cflags --libs` -lpthread

FractalNoise.o:
	g++ -Werror -m64 -g0 -O4 -c -o FractalNoise.o FractalNoise.cpp
	#g++ -Werror -m64 -g3 -O0 -c -o FractalNoise.o FractalNoise.cpp

Perlin.o:
	g++ -Werror -m64 -g0 -O4 -c -o Perlin.o Perlin.cpp
	#g++ -Werror -m64 -g3 -O0 -c -o Perlin.o Perlin.cpp

clean:
	rm -f *.o a.out

