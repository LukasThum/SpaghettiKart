# git clone https://github.com/HarbourMasters/SpaghettiKart.git
# cd SpaghettiKart
# git submodule update --init

# cmake -H. -Bbuild-cmake -GNinja
# cmake --build build-cmake --target ExtractAssets
cmake --build build-cmake
