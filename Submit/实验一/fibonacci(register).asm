sys_exit	equ		1
sys_read	equ		3
sys_write	equ		4
stdin		equ		0
stdout		equ		1

section	.data
color_red:      db  	1Bh, '[31;1m', 0
.len            equ 	$ - color_red
color_blue:     db  	1Bh, '[34;1m', 0
.len            equ 	$ - color_blue
color_default:  db  	1Bh, '[37;0m', 0
.len            equ 	$ - color_default
prompt:			db		"Please input n = ", 0
promptLen:		equ		$ - prompt
output:			db		"Fibonacci Sequence is:", 0AH
outputLen:		equ		$ - output
newLine:		db		0AH

section	.bss
input:		resb	128
inputLen:	equ		$ - input
buffer:		resb	1
bufferLen:	equ		$ - buffer
digit		resb	1
num1		resb	4
num2		resb	4
num3		resb	4
numLen		equ 	$ - num3

section	.text
global _start

_start:
	mov		ecx, prompt
	mov		edx, promptLen
	call	DisplayText

	mov		ecx, input
	mov		edx, inputLen
	call 	ReadText
	
	mov		ecx, output
	mov		edx, outputLen
	call	DisplayText

	mov		ecx, input
	mov 	dl, 0
Judge:
	cmp		byte[ecx], 0AH
	je		Exit
	cmp		byte[ecx], 20H
	jne		Run
	inc		ecx
	jmp		Judge
Run:
	call 	SetColorRed
	mov 	byte[buffer], 0
	inc 	dl
	mov 	dh, 1
	and 	dh, dl
	cmp 	dh, 0
	je 		ChangeColor
	call 	SetColorBlue
ChangeColor:
	push 	edx
	call	ParseInput
	call	Compute
	pop 	edx
	jmp		Judge

ParseInput:
	mov 	al, byte[ecx]
	sub 	al, 30H
	mov 	byte[digit], al
	mov 	al, byte[buffer]
	mov		bl, 10
	mul		bl
	add 	al, byte[digit]
	mov		byte[buffer], al
	inc 	ecx
	cmp 	byte[ecx], 0AH
	je 		Return
	cmp		byte[ecx], 20H
	je		Return
	jmp 	ParseInput
Return:
	ret

Compute:
	mov 	dword[num1], 1
	mov 	dword[num2], 1
	mov 	dword[num3], 1
	mov 	al, byte[buffer]
	cmp 	al, 1
	jna		FinishCompute
Repeat:
	dec 	al
	cmp 	al, 0
	je 		FinishCompute
	mov 	ebx, dword[num2]
	mov 	dword[num1], ebx
	mov 	ebx, dword[num3]
	mov 	dword[num2], ebx
	add 	ebx, dword[num1]
	mov 	dword[num3], ebx
	jmp 	Repeat
FinishCompute:
	pusha
	call 	PrintNum
	popa
	ret

PrintNum:
	mov 	bl, 0
Parse:
	mov 	ax, word[num3]
	mov 	edx, num3
	add 	edx, 2
	mov 	dx, word[edx]
	mov 	cx, 10
	div 	cx
	movzx 	eax, ax
	mov 	dword[num3], eax
	add 	dx, 30H
	push 	dx
	inc 	bl
	cmp 	ax, 0
	je 		Print
	jmp 	Parse
Print:
	cmp 	bl, 0
	je 		FinishPrint
	pop 	dx
	mov 	byte[digit], dl
	mov 	ecx, digit
	mov 	edx, 1
	call 	DisplayText
	dec 	bl
	jmp 	Print
FinishPrint:
	call 	NewLine
	ret

SetColorRed:
	pusha
	mov 	eax, sys_write
	mov 	ebx, stdout
	mov 	ecx, color_red
	mov 	edx, color_red.len
	int  	80h
	popa
	ret

SetColorBlue:
	pusha
	mov 	eax, sys_write
	mov 	ebx, stdout
	mov 	ecx, color_blue
	mov 	edx, color_blue.len
	int 	80h
	popa
	ret

;打印换行符
NewLine:
	pusha
	mov		eax, sys_write
	mov		ebx, stdout
	mov		ecx, newLine
	mov		edx, 1
	int		80h
	popa
	ret
;打印字符
DisplayText:
	pusha
	mov		eax, sys_write
	mov		ebx, stdout
	int		80h
	popa
	ret
;键盘读入字符
ReadText:
	mov		eax, sys_read
	mov		ebx, stdin
	int		80h
	ret
;结束程序
Exit:
	mov		eax, sys_exit
	xor		ebx, ebx
	int		80h