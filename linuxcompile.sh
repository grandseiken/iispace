mkdir linuxrelease
mkdir linuxrelease/wiiscore
mkdir linuxrelease/wiispace
mkdir linuxrelease/wiispace64
cd linuxrelease/wiiscore
g++ -Wall -c -O3 -D PLATFORM_REPSCORE -D PLATFORM_LINUX -D PLATFORM_64 ../../source/boss.cpp ../../source/bossenemy.cpp ../../source/enemy.cpp ../../source/game.cpp ../../source/lib.cpp ../../source/librepscore.cpp ../../source/overmind.cpp ../../source/player.cpp ../../source/powerup.cpp ../../source/ship.cpp ../../source/stars.cpp ../../source/z0.cpp ../../source/z0game.cpp ../../source/fix32.cpp
g++ -Wall -c -O3 -fno-strict-aliasing -D PLATFORM_REPSCORE -D PLATFORM_LINUX -D PLATFORM_64 ../../source/aliasing.cpp
gcc -Wall -c -O3 ../../zlib/*.c
g++ -Wall -O3 -static -o wiiscore *.o
cd ../wiispace64
g++ -Wall -c -O3 -D PLATFORM_WIN -D PLATFORM_LINUX -D PLATFORM_64 ../../source/boss.cpp ../../source/bossenemy.cpp ../../source/enemy.cpp ../../source/game.cpp ../../source/lib.cpp ../../source/libwin.cpp ../../source/overmind.cpp ../../source/player.cpp ../../source/powerup.cpp ../../source/ship.cpp ../../source/stars.cpp ../../source/z0.cpp ../../source/z0game.cpp ../../source/fix32.cpp
g++ -Wall -c -O3 -fno-strict-aliasing -D PLATFORM_WIN -D PLATFORM_LINUX -D PLATFORM_64 ../../source/aliasing.cpp
gcc -Wall -c -O3 ../../zlib/*.c
g++ -Wall -O3 -lsfml-system -lsfml-window -lsfml-graphics -lsfml-audio -lsfml-network -o wiispace *.o -Wl,-Bstatic libOIS.a -Wl,-Bdynamic
g++ -Wall -c -O3 ../../source/gamepadcfg.cpp
mv gamepadcfg.o ../gamepadcfg.o
g++ -Wall -lX11 -O3 -o gamepadconfig ../gamepadcfg.o -Wl,-Bstatic libOIS.a -Wl,-Bdynamic
cd ../wiispace
g++ -Wall -c -O3 -m32 -D PLATFORM_WIN -D PLATFORM_LINUX ../../source/boss.cpp ../../source/bossenemy.cpp ../../source/enemy.cpp ../../source/game.cpp ../../source/lib.cpp ../../source/libwin.cpp ../../source/overmind.cpp ../../source/player.cpp ../../source/powerup.cpp ../../source/ship.cpp ../../source/stars.cpp ../../source/z0.cpp ../../source/z0game.cpp ../../source/fix32.cpp
g++ -Wall -c -O3 -m32 -fno-strict-aliasing -D PLATFORM_WIN -D PLATFORM_LINUX ../../source/aliasing.cpp
gcc -Wall -c -O3 -m32 ../../zlib/*.c
g++ -Wall -O3 -m32 -L. -lsfml-system -lsfml-window -lsfml-graphics -lsfml-audio -lsfml-network -o wiispace *.o -Wl,-Bstatic libOIS32.a -Wl,-Bdynamic
g++ -Wall -m32 -c -O3 -m32 ../../source/gamepadcfg.cpp
mv gamepadcfg.o ../gamepadcfg.o
g++ -Wall -m32 -lX11 -O3 -o gamepadconfig ../gamepadcfg.o -Wl,-Bstatic libOIS32.a -Wl,-Bdynamic
