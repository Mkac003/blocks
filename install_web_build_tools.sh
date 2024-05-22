mkdir -p build
mkdir -p build/emcc
cd build/emcc/
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest