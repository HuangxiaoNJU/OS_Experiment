
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

INT_VECTOR_SYS_CALL equ 0x90
_NR_get_ticks       equ 0
_NR_write	    	equ 1
_NR_process_sleep	equ 2
_NR_disp_str		equ 3
_NR_sem_p			equ 4
_NR_sem_v			equ 5

; 导出符号
global	get_ticks
global	write
global  process_sleep
global 	my_disp_str
global 	sem_p
global 	sem_v

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================================
;                          void write(char* buf, int len);
; ====================================================================================
write:
        mov     eax, _NR_write
        mov     ebx, [esp + 4]
        mov     ecx, [esp + 8]
        int     INT_VECTOR_SYS_CALL
        ret

process_sleep:
	mov eax, _NR_process_sleep
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

my_disp_str:
	mov eax, _NR_disp_str
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

sem_p:
	mov eax, _NR_sem_p
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

sem_v:
	mov eax, _NR_sem_v
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret
