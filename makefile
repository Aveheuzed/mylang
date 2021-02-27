main: src/*/*.c src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote . -o $@ src/*/*.c src/*.c -O3

debug: src/*/*.c src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote . -o $@ src/*/*.c src/*.c -D DEBUG -Og -Wall

brainfuck: bf/*.c
	gcc -include headers/utils/debug.h -iquote . -o $@ bf/*.c -O3 -fshort-enums
