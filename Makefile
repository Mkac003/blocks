native:
	mkdir -p build
	gcc src/main.c -lSDL2 -lm -lSDL2_image -o build/main
	cp -R res build/res

run:
	cd build;./main