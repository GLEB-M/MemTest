@echo off
set PATH=C:\mingw-w64\i686-7.2.0-win32-dwarf-rt_v5-rev1\mingw32\bin

del /q out\*.*

gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\entry.o    entry.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\common.o   common.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\screen.o   screen.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\memsetup.o memsetup.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\hw.o       hw.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\test.o     test.c
gcc.exe -ffreestanding -c -fomit-frame-pointer -masm=intel -o out\main.o     main.c


ld.exe -s -Ttext 0xA000 --file-alignment 1 --section-alignment 1 -o out\memtest.bin out\entry.o out\common.o out\screen.o out\memsetup.o out\hw.o out\test.o out\main.o

objcopy.exe out\memtest.bin -O binary

copy /b startup.bin + out\memtest.bin out\memtest

pause
