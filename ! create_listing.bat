@echo off
cd src

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c main.cpp -o main.o
objdump -d -M x86-64,intel-mnemonic -S main.o >main.asm

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c compare_avx.cpp -o compare_avx.o
objdump -d -M x86-64,intel-mnemonic -S compare_avx.o >compare_avx.asm

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c compare_sse.cpp -o compare_sse.o
objdump -d -M x86-64,intel-mnemonic -S compare_sse.o >compare_sse.asm

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c compare_scalar.cpp -o compare_scalar.o
objdump -d -M x86-64,intel-mnemonic -S compare_scalar.o >compare_scalar.asm

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c SearchMgr.cpp -o SearchMgr.o
objdump -d -M x86-64,intel-mnemonic -S SearchMgr.o >SearchMgr.asm

gcc -fomit-frame-pointer -O3 -Weffc++ -pedantic -Wall -std=c++14 -march=core-avx2 -funroll-loops -g -c BitmapNode.cpp -o BitmapNode.o
objdump -d -M x86-64,intel-mnemonic -S BitmapNode.o >BitmapNode.asm
