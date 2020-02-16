# MemTest
**Utility for RAM diagnostic**

***Run from GRUB***
```
title MemTest
chainloader --force --load-segment=0x0 --load-offset=0x8000 --boot-cs=0x0 --boot-ip=0x8000 /memtest
```

***Build***

**ASM** module (startup.asm) compiled by **FASM**

**C** modules compiled by **MinGW** (build.bat)

The OUT directory contain compiled binary (memtest)

![Emulator](https://github.com/dx8vb/Memory-Test/blob/master/screenshot/emulator1.png)
![Emulator](https://github.com/dx8vb/Memory-Test/blob/master/screenshot/emulator2.png)
![Hardware](https://github.com/dx8vb/Memory-Test/blob/master/screenshot/hardware.png)
