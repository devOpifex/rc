build:
	gcc -Wall -Wextra -o bin/rc main.c

run: build
	./bin/rc
