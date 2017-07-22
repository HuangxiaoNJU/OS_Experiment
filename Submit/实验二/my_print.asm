section .data

setcolor db 1Bh, '[49;34m', 0 
    .len equ $ - setcolor
basecolor db 1Bh, '[0;0m', 0 
    .len equ $ - basecolor

GLOBAL myprint
GLOBAL changeColor
GLOBAL retColor

section .text
myprint:
	mov 	ecx, [esp + 4]
	mov 	edx, [esp + 8]
	mov 	eax, 4
	mov 	ebx, 1
	int 	80h
	ret

changeColor:
    mov eax, 4
    mov ebx, 1
    mov ecx, setcolor
    mov edx, setcolor.len
    int 80h
    ret

retColor:
	mov eax, 4
    mov ebx, 1
    mov ecx, basecolor
    mov edx, basecolor.len
    int 80h
    ret
