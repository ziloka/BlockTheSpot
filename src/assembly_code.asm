.DATA
; External declarations
EXTERN get_file_name : PROC
EXTERN modify_source : PROC
EXTERN ZipScan : QWORD
EXTERN file_name_rcx : QWORD
EXTERN ret_addr_file_name : QWORD
EXTERN ret_addr_file_source : QWORD

.CODE

; get_str - C++ to MASM conversion
; RCX = pRequest
; Return value in RAX

; get_str PROC
;     mov rax, rcx
;     mov rdx, qword ptr [rax]
;     mov rax, rdx
;     ret
; get_str ENDP

; PUSH_ALL_REGISTERS MACRO
;     sub rsp, 120    ; Reserve space for 15 registers (8 bytes each)
;     mov [rsp], rax  ; Store rax on the stack
;     mov [rsp+8], rbx
;     mov [rsp+16], rcx
;     mov [rsp+24], rdx
;     mov [rsp+32], rsi
;     mov [rsp+40], rdi
;     mov [rsp+48], rbp
;     mov [rsp+56], r8
;     mov [rsp+64], r9
;     mov [rsp+72], r10
;     mov [rsp+80], r11
;     mov [rsp+88], r12
;     mov [rsp+96], r13
;     mov [rsp+104], r14
;     mov [rsp+112], r15
; ENDM
; 
; POP_ALL_REGISTERS MACRO
;     mov rax, [rsp]      ; Restore r15 from the stack
;     mov rbx, [rsp+8]
;     mov rcx, [rsp+16]
;     mov rdx, [rsp+24]
;     mov rsi, [rsp+32]
;     mov rdi, [rsp+40]
;     mov rbp, [rsp+48]
;     mov r8, [rsp+56]
;     mov r9, [rsp+64]
;     mov r10, [rsp+72]
;     mov r11, [rsp+80]
;     mov r12, [rsp+88]
;     mov r13, [rsp+96]
;     mov r14, [rsp+104]
;     mov r15, [rsp+112]
;     add rsp, 120       ; Adjust the stack pointer to remove the stored values
; ENDM

; Macro to push all registers onto the stack
align 8
PUSH_ALL_REGISTERS MACRO
	push rax      ;save current rax
	push rbx      ;save current rbx
	push rcx      ;save current rcx
	push rdx      ;save current rdx
	push rbp      ;save current rbp
	push rdi      ;save current rdi
	push rsi      ;save current rsi
	push r8         ;save current r8
	push r9         ;save current r9
	push r10      ;save current r10
	push r11      ;save current r11
	push r12      ;save current r12
	push r13      ;save current r13
	push r14      ;save current r14
	push r15      ;save current r15
ENDM

; Macro to pop all registers from the stack
align 8
POP_ALL_REGISTERS MACRO
	pop r15         ;restore current r15
	pop r14         ;restore current r14
	pop r13         ;restore current r13
	pop r12         ;restore current r12
	pop r11         ;restore current r11
	pop r10         ;restore current r10
	pop r9         ;restore current r9
	pop r8         ;restore current r8
	pop rsi         ;restore current rsi
	pop rdi         ;restore current rdi
	pop rbp         ;restore current rbp
	pop rdx         ;restore current rdx
	pop rcx         ;restore current rcx
	pop rbx         ;restore current rbx
	pop rax         ;restore current rax
ENDM

; Procedure to hook file name

hook_file_name PROC 
	test rcx, rcx 
	mov file_name_rcx, rcx 
	PUSH_ALL_REGISTERS 
	call get_file_name 
	POP_ALL_REGISTERS 
	push ret_addr_file_name 
	ret 
hook_file_name ENDP 

; Procedure to hook zip buffer

hook_zip_buffer PROC 
	movsxd rcx, eax 
	mov ZipScan, r15 
	mov ZipScan+8, rcx 
	; PUSH_ALL_REGISTERS 
	call modify_source 
	; POP_ALL_REGISTERS 
	push ret_addr_file_source
	ret 
hook_zip_buffer ENDP 

END