all:
	g++ *.cpp -Wall -Wextra -Wpedantic -o ted -lncurses -std=c++20 -g3

install:
	@cp ./ted /usr/bin/ted