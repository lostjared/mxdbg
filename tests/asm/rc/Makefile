all:
	gcc -g3 `sdl2-config --cflags` main.c surface.s -o rc-test -no-pie -lSDL2 -lm
	gcc -O3 `sdl2-config --cflags` main.c surface.s -o rc-test-release -lSDL2 -lm
clean:
	rm -f rc-test
	rm -f rc-test-release
