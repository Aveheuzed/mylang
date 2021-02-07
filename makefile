main: src/*/*.c src/*.c headers/*/*.h
	gcc -iquote . -o $@ src/*/*.c src/*.c
