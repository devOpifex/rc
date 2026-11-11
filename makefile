build:
	gcc -o bin/rc main.c

run: build
	./bin/rc
