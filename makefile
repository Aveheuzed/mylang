main: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums

debug: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -D DEBUG -Og -Wall -fshort-enums

profiling: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	rm -f profiling.d/*
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums -fprofile-dir=profiling.d/ -fprofile-generate
optimized: src/*/*.c src/*/*.s src/*.c headers/*/*.h profiling.d/
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums -fprofile-dir=profiling.d/ -fprofile-use
