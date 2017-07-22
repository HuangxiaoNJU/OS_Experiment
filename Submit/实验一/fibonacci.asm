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
prompt:			db		"Please input n = ", 0
promptLen:		equ		$ - prompt
output:			db		"Fibonacci Sequence is:", 0AH
outputLen:		equ		$ - output
newLine:		db		0AH

section	.bss
input:		resb	128
inputLen:	equ		$ - input
buffer:		resb	4
bufferLen:	equ		$ - buffer
digit		resb	1
carry 		resb 	1
num1		resb	128
num2		resb	128
temp 		resb	128
numLen		equ 	$ - temp
count 		resb 	2

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
	mov 	dword[buffer], 0
	inc 	dl
	mov 	dh, 1
	and 	dh, dl
	cmp 	dh, 0
	je 		ChangeColor
	call 	SetColorBlue
ChangeColor:
	push 	edx
	call	ParseInput
	push 	ecx
	call	Compute
	pop 	ecx
	pop 	edx
	jmp		Judge

; 输入解析
ParseInput:
	mov 	al, byte[ecx]
	sub 	al, 30H
	mov 	byte[digit], al
	mov 	eax, dword[buffer]
	mov		ebx, 10
	mul		ebx
	mov 	dl, byte[digit]
	movzx 	edx, dl
	add 	eax, edx
	mov		dword[buffer], eax
	inc 	ecx
	cmp 	byte[ecx], 0AH
	je 		Return
	cmp		byte[ecx], 20H
	je		Return
	jmp 	ParseInput

; 计算fibonacci值
Compute:
	;set num1 num2 temp = 0
	push 	eax
	mov 	eax, num1
	call 	Refresh
	mov 	eax, num2
	call 	Refresh
	mov 	eax, temp
	call 	Refresh
	pop 	eax
	mov 	byte[num1], 1
	mov 	byte[num2], 1
	;输入数小于等于1则直接输出1
	cmp 	dword[buffer], 1
	jna 	FinishCompute
Compare:
	dec 	dword[buffer]
	cmp 	dword[buffer], 0
	je 		FinishCompute
	;循环体
	call 	Iterator
	jmp 	Compare
FinishCompute:
	call 	PrintNum
	ret

;循环体：temp=num1+num2, num1=num2, num2=temp
Iterator:
	pusha
	;初始化
	mov 	byte[carry], 0
	mov 	word[count], numLen
	mov 	ebx, num1
	mov 	ecx, num2
	mov 	edx, temp
NextBit:
	mov 	al, byte[ebx]
	mov 	ah, byte[ecx]
	add 	al, ah
	add 	al, byte[carry]
	mov 	byte[carry], 0
	cmp 	al, 10
	jb 		NoCarry
	sub 	al, 10
	mov 	byte[carry], 1
NoCarry:
	mov 	byte[edx], al
	inc 	ebx
	inc 	ecx
	inc 	edx
	dec 	word[count]
	cmp 	word[count], 0
	jne 	NextBit 	
	mov 	eax, num1
	mov 	ebx, num2
	call 	CopyNum
	mov 	eax, num2
	mov 	ebx, temp
	call 	CopyNum
	popa
	ret

; 将内存地址置为0
Refresh:
	pusha
	mov 	ebx, numLen
RefreshNextByte:
	mov 	byte[eax], 0
	inc 	eax
	dec 	ebx
	cmp 	ebx, 0
	jne 	RefreshNextByte
	popa
	ret

; 复制内存地址内容
CopyNum:
	pusha
	mov 	ecx, numLen
CopyNextByte:
	mov 	dl, byte[ebx]
	mov 	byte[eax], dl
	inc 	eax
	inc 	ebx
	dec 	ecx
	cmp 	ecx, 0
	jne	 	CopyNextByte
	popa
	ret

;打印num2 即最终结果
PrintNum:
	push 	eax
	mov 	eax, num2
	add 	eax, numLen
Search:
	dec 	eax
	cmp 	byte[eax], 0
	je 		Search
	inc 	eax
PrintByte:
	dec 	eax
	add 	byte[eax], 30H
	mov 	ecx, eax
	mov 	edx, 1
	call 	DisplayText
	cmp 	eax, num2
	jne 	PrintByte
	call 	NewLine
	pop 	eax
	ret

;设置颜色为红色
SetColorRed:
	pusha
	mov 	eax, sys_write
	mov 	ebx, stdout
	mov 	ecx, color_red
	mov 	edx, color_red.len
	int  	80h
	popa
	ret
;设置颜色为蓝色
SetColorBlue:
	pusha
	mov 	eax, sys_write
	mov 	ebx, stdout
	mov 	ecx, color_blue
	mov 	edx, color_blue.len
	int 	80h
	popa
	ret

Return:
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