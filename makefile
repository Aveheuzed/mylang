build: src/*
	gcc -o main src/*

optimize: src/
	gcc -O3 -o main src/*
