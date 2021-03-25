main: src/*/*.c src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*.c -O3 -fshort-enums

debug: src/*/*.c src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*.c -D DEBUG -Og -Wall -fshort-enums
