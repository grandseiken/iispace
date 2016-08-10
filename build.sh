cd "${0%/*}"
git submodule update --init
mkdir -p ./build
(cd ./build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j 4 $@)
