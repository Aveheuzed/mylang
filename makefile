main: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fcf-protection -Wl,-z,now -Wl,-z,relro -Wl,-z,notext

debug: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -D DEBUG -Og -Wall -fshort-enums -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fcf-protection -Wl,-z,now -Wl,-z,relro -Wl,-z,notext


profiling: src/*/*.c src/*/*.s src/*.c headers/*/*.h
	rm -f profiling.d/*
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums -fprofile-dir=profiling.d/ -fprofile-generate -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fcf-protection -Wl,-z,now -Wl,-z,relro -Wl,-z,notext

optimized: src/*/*.c src/*/*.s src/*.c headers/*/*.h profiling.d/
	gcc -include headers/utils/debug.h -iquote headers/ -o $@ src/*/*.c src/*/*.s src/*.c -O3 -fshort-enums -fprofile-dir=profiling.d/ -fprofile-use -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fcf-protection -Wl,-z,now -Wl,-z,relro -Wl,-z,notext

