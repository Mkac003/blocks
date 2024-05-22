native:
	mkdir -p build
	mkdir -p build/native
	gcc src/main.c -lSDL2 -lm -lSDL2_image -o build/native/main
	cp -R res build/native/

web:
	cd build/emcc/emsdk; \
	./emsdk activate latest; \
	. ./emsdk_env.sh; \
	cd ../../../; \
	mkdir -p build; \
	mkdir -p build/web; \
	emcc src/main.c -O3 --shell-file web/shell.html --preload-file res -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -o build/web/main.html; \

run:
	cd build/native/;./main