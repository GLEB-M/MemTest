# Memory Test
**Utility for memory test**

***Run via GRUB***
```
title MemTest
chainloader --force --load-segment=0x0 --load-offset=0x8000 --boot-cs=0x0 --boot-ip=0x8000 /memtest
```

***Build***

ASM module (startup.asm) compiled by FASM

C modules compiled by MinGW (build.bat)

The OUT directory contain compiled binary (memtest)
