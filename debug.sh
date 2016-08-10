cd "${0%/*}"
git submodule update --init
mkdir -p ./debug
(cd ./debug && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j 4 $@)
