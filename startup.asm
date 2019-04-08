; ********************************************************************** ;
;									 ;
;									 ;
;			   MEMTEST 3 ASM MODULE 			 ;
;									 ;
;									 ;
; ********************************************************************** ;

PM_CODE_SEG_DESC  equ 0x8
PM_DATA_SEG_DESC  equ 0x10
STACK		  equ 0xA000

org 0x8000

use16

start:
   jmp 0x0:code
   nop
   nop
   nop

timer_call_back  dd 0
keyb_call_back	 dd 0


e820max 	 dd 0
e820map:
   times 1280	 db 0  ; 64 * 20


; GLOBAL DESCRIPTOR TABLE ;
gdt:
   dw 0, 0, 0, 0   ; null descriptor

   db 0xFF	   ; protected mode code seg DPL=0 BASE=0 LIMIT=4GB
   db 0xFF
   db 0x00
   db 0x00
   db 0x00
   db 10011010b
   db 11001111b
   db 0x00
   
   db 0xFF	    ; protected mode data seg DPL=0 BASE=0 LIMIT=4GB
   db 0xFF
   db 0x00 
   db 0x00
   db 0x00
   db 10010010b
   db 11001111b
   db 0x00

   dw 0, 0, 0, 0    ; not used

gdt_size equ $ - gdt

gdtr:
   dw  gdt_size - 1
   dd  gdt


; PROTECTED MODE INTERRUPT DESCRIPTOR TABLE
idt:
   ; EXCEPTIONS ;
   dw DE_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #div by zero
   dw DB_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #debug
   dw NMI_Handler, PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #NMI
   dw BP_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #break
   dw OF_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #overflow
   dw BR_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #bound
   dw UD_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #opcode
   dw NM_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #device
   dw DF_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #double fault
   dw CSO_Handler, PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #coproc
   dw TS_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #tss
   dw NP_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #seg not present
   dw SS_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #stack fault
   dw GP_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #GP
   dw PF_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #page fault
   dd 0, 0						     ; 15 RESERVED
   dw MF_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #x87
   dw AC_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #align
   dw MC_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #macine check
   dw XF_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0    ; #SIMD
   dd 0, 0						     ; 20 RESERVED
   dd 0, 0						     ; 21 RESERVED
   dd 0, 0						     ; 22 RESERVED
   dd 0, 0						     ; 23 RESERVED
   dd 0, 0						     ; 24 RESERVED
   dd 0, 0						     ; 25 RESERVED
   dd 0, 0						     ; 26 RESERVED
   dd 0, 0						     ; 27 RESERVED
   dd 0, 0						     ; 28 RESERVED
   dd 0, 0						     ; 29 RESERVED
   dd 0, 0 ;dw SX_Handler,  PM_CODE_SEG_DESC, 1000111000000000b, 0   ; #security
   dd 0, 0						     ; 31 RESERVED
   ; PIC INTERRUPTS
   ; MASTER
   dw timer_handler,	PM_CODE_SEG_DESC, 1000111000000000b, 0 ; TIMER
   dw keyboard_handler, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; KEYBOARD
   dd 0, 0						       ; CASCADE
   dw interrupt_stub_m, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; COM2
   dw interrupt_stub_m, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; COM1
   dw interrupt_stub_m, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; LPT2
   dw interrupt_stub_m, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; FDD
   dw interrupt_stub_m, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; LPT1
   ; SLAVE
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; RTC
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; NULL
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; NULL
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; NULL
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; MOUSE
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; CO-PROC
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; HDD1
   dw interrupt_stub_s, PM_CODE_SEG_DESC, 1000111000000000b, 0 ; HDD2

idt_size equ $ - idt

pm_idtr:
   dw idt_size - 1
   dd idt

remap_irq:

   mov al, 0x11       ; init
   out 0x20, al
   out 0xA0, al

   mov al, 0x20       ; privary
   out 0x21, al
   mov al, 0x28       ; secondary
   out 0xA1, al

   mov al, 0x4	      ; set master
   out 0x21, al
   mov al, 0x2	      ; set slave
   out 0xA1, al

   mov al, 0x1	      ; set mode
   out 0x21, al
   out 0xA1, al

   mov al, 11111000b  ; master: unmask timer and keyboard and sec cascade
   out 0x21, al

   mov al, 0xFF       ; slave: mask all
   out 0xA1, al

   ret


get_memory_map:

   xor ebx, ebx
   mov di,  e820map
@e820:
   cmp [e820max], 64	; check limit
   je @e820err2

   mov eax, 0x0000E820
   mov edx, 0x534D4150	; SMAP
   mov ecx, 20
   push ds
   pop es

   int 0x15

   jc @e820err1 	; check CF

   cmp eax, 0x534D4150	; check SMAP
   jne @e820err1

   inc [e820max]
   add di, 20

   cmp ebx, 0		; done
   je @e820end

   jmp @e820	       

@e820err1:
   mov esi, err_820_1
   jmp fatal_error

@e820err2:
   mov esi, err_820_2
   jmp fatal_error

@e820end:
   ret


; **************************************** CODE START **************************************** 
code:
   ; set segments
   cli
   xor ax, ax
   mov ds, ax
   mov es, ax
   mov ss, ax
   mov sp, STACK
   sti

   ; clear screen
   mov ax, 0x3
   int 0x10

   ; hide cursor
   mov ah, 0x1
   mov cx, 0x2607
   int 0x10

   ; get memory map
   call get_memory_map

   ; **************************** SWITCH TO PROTECTED MODE *************************** 

   ; enable A20
   in al, 0x92
   or al, 0x2
   out 0x92, al

   ; disable interrupts
   cli
   in al, 0x70
   or al, 0x80
   out 0x70, al
   ;in al, 0x71

   ; remap interrupt base vector
   call remap_irq

   ; load GDTR
   lgdt [gdtr]

   ; load IDTR
   lidt [pm_idtr]

   ; enable protected mode
   mov eax, cr0
   or al, 1
   mov cr0, eax

   ; jump to protected mode segment
   jmp PM_CODE_SEG_DESC:pmode_entry

; ***************************** PROTECTED MODE CODE **************************** 

use32

pmode_entry:
   ; set data and stack segments
   mov ax, PM_DATA_SEG_DESC
   mov ds, ax
   mov es, ax
   mov fs, ax
   mov gs, ax
   mov ss, ax
   mov esp, STACK

   ; test A20 gate
   mov byte[100000111110000000000b], 0x00 ; 0x107C00
   mov byte[000000111110000000000b], 0xFF ; 0x7C00
   cmp byte[100000111110000000000b], 0x00 ; 0x107C00
   je pm_start

   ; A20 Gate Error
   mov esi, err_a20
   jmp fatal_error

pm_start:
   ; co-processor initialize
   fninit

   ; enable interrupts
   in  al, 0x70
   and al, 0x7F
   out 0x70, al
   ;  in al, 0x71
   sti

   ; JUMP TO "C" CODE
   jmp 0xA000

;**********************************************************************************;
;										   ;
;				EXCEPTION HANDLERS				   ;
;										   ;
;**********************************************************************************;

; Div By Zero
DE_Handler:
   mov esi, div_by_zero
   jmp fatal_error
   iretd

;Debug
DB_Handler:
   mov esi, db_fault
   jmp fatal_error
   iretd

;NMI
NMI_Handler:
   mov esi, nmi_fault
   jmp fatal_error
   iretd

; Breakpoint
BP_Handler:
   mov esi, bp_fault
   jmp fatal_error
   iretd

; Overflow
OF_Handler:
   mov esi, of_fault
   jmp fatal_error
   iretd

;Bound
BR_Handler:
   mov esi, bound_fault
   jmp fatal_error
   iretd

; Invalid opcode
UD_Handler:
   mov esi, opcode_fault
   jmp fatal_error
   iretd

; Device not avail
NM_Handler:
   mov esi, device_fault
   jmp fatal_error
   iretd

;Double fault
DF_Handler:
   add esp, 4
   mov esi, double_fault
   jmp fatal_error
   iretd

;COProc Seg Overrun
CSO_Handler:
   mov esi, cop_fault
   jmp fatal_error
   iretd

; Ivalid TSS
TS_Handler:
   add esp, 4
   mov esi, tss_fault
   jmp fatal_error
   iretd

; Seg Not Present
NP_Handler:
   add esp, 4
   mov esi, seg_fault
   jmp fatal_error
   iretd

; Stack Seg Fault
SS_Handler:
   add esp, 4
   mov esi, stack_fault
   jmp fatal_error
   iretd

; Genaral Protection Fault
GP_Handler:
   add esp, 4
   mov esi, gp_fault
   jmp fatal_error
   iretd

; Page Fault
PF_Handler:
   add esp, 4
   mov esi, pf_fault
   jmp fatal_error
   iretd

; x87 Float-Point
MF_Handler:
   mov esi, x87_fault
   jmp fatal_error
   iretd

; Align
AC_Handler:
   add esp, 4
   mov esi, align_fault
   jmp fatal_error
   iretd

; Machine check
MC_Handler:
   mov esi, mc_fault
   jmp fatal_error
   iretd

; SIMD
XF_Handler:
   mov esi, simd_fault
   jmp fatal_error
   iretd

;Security
;SX_Handler:
;   mov esi, sec_fault
;   jmp fatal_error
;   iretd


interrupt_stub_m:
   push ax
   mov al, 0x20
   out 0x20, al
   pop ax
   iretd

interrupt_stub_s:
   push ax
   mov al, 0x20
   out 0xA1, al
   out 0x20, al
   pop ax
   iretd


;Timer Handler
timer_handler:
   pushad
   cmp [timer_call_back], 0
   je @f
   call [timer_call_back]
   @@:
   mov al, 0x20
   out 0x20, al
  ;out 0xA1, al
   popad
   iretd

;Keyboard Handler
keyboard_handler:
   pushad
   in al, 0x60 ; read key
   cmp [keyb_call_back], 0
   je @f
   xor edx, edx
   mov dl, al
   push edx
   call [keyb_call_back]
   add esp, 4
   @@:
   mov al, 0x20
   out 0x20, al
   ;out 0xA1, al
   popad
   iretd

;EOI:
;	 push eax
;	 mov  al, 0x20
;	 out  0x20, al
;	 pop  eax
;	 iretd

;**********************************************************************************;
;										   ;
;				FATAL ERROR MESSAGE				   ;
;										   ;
;**********************************************************************************;

; ESI - message

fatal_error:
   mov edi, 0xB8000
   @@:
	cmp byte[esi], 0
	je @f
	mov al, byte[esi]
	mov byte[edi], al
	mov byte[edi + 1], 0x4F ; red background
	add esi, 1
	add edi, 2
   jmp @b
   @@:

   jmp $ ; loop

; ************************************** DATA **************************************** ;

err_a20      db " ERROR: A20 Line Disabled! ", 0
err_820_1    db " ERROR: E820 Failed! ", 0
err_820_2    db " ERROR: E820 Overflow! ", 0

div_by_zero  db " #Division By Zero! ", 0
db_fault     db " #Debug Fault! ", 0
nmi_fault    db " #NMI Fault! ", 0
bp_fault     db " #Break! ", 0
of_fault     db " #Overflow! ", 0
bound_fault  db " #Bound! ", 0
opcode_fault db " #Invalid Opcode! ", 0
device_fault db " #Device Not Available! ", 0
double_fault db " #Double Fault! ", 0
cop_fault    db " #Co-Processor Seg Overrun! ", 0
tss_fault    db " #Invalid TSS! ", 0
gp_fault     db " #General Protection Fault! ", 0
pf_fault     db " #Page Fault! ", 0
stack_fault  db " #Stack Segment Fault! ", 0
seg_fault    db " #Segment Not Present! ", 0
x87_fault    db " #x87 Float Point! ", 0
align_fault  db " #Align Fault! ", 0
mc_fault     db " #Machine Check Fault! ", 0
simd_fault   db " #SIMD Fault! ", 0


times 4096 db 0  ; reserved for stack

finish:
   times 8192 - finish + start db 0
