FORMAT PE GUI 4.0 DLL

ENTRY code_start

section 'code' code readable writeable executable
code_start = $

jmp GL001
DD code_size
GL001:
call GetCurrentAddress
mov ebp, eax

call GetKernelModuleHandle

lea ecx, [ebp+_str.LoadLibraryA.Pos]
push ecx
push eax 
call SimpleGetProcAddress

lea ecx, [ebp+_str.cheat_entry_dll.Pos]
push ecx
call eax

Self_loop: jmp Self_loop

GetCurrentAddress:
mov eax, [esp]
sub eax, 11
ret

GetKernelModuleHandle:
mov eax, [fs:30h]
mov eax, [eax+0Ch]
mov eax, [eax+14h]
mov eax, [eax]
mov eax, [eax]
mov eax, [eax+10h]
ret


SimpleGetProcAddress: ;HMODULE module, const char* name

.ExportDirectory = -4
.NumberOfName = -8
.NamePointer = -12
.OrdinalTable = -16
.ExportTable = -20

push ebp
mov ebp, esp
sub esp, 20
push ecx
push edx

mov eax, [ebp+8]
mov ecx, [eax+3Ch]
add eax, ecx
mov ecx, [eax+78h]
add ecx, [ebp+8]
mov [ebp+.ExportDirectory], ecx

mov eax, [ecx+20h] ;Name Pointer
add eax, [ebp+8]
mov [ebp+.NamePointer], eax

mov eax, [ecx+24h] ;Ordinal Table
add eax, [ebp+8]
mov [ebp+.OrdinalTable], eax

mov eax, [ecx+1Ch] ;Export Table
add eax, [ebp+8]
mov [ebp+.ExportTable], eax


mov ecx, [ecx+18h] ;NumberOfName
xor edx, edx

.L1:
mov eax, [ebp+.NamePointer]
mov eax, [eax+edx*4]
add eax, [ebp+8]
push DWORD PTR ebp+12
push eax
call SimpleStrEqual
jnz .Equal
inc edx 
cmp ecx, edx
jne .L1
xor eax, eax 
jmp .return
.Equal:
mov eax, [ebp+.OrdinalTable]
movzx eax, WORD PTR eax+edx*2
mov ecx, [ebp+.ExportTable]
mov eax, [ecx+eax*4]
add eax, [ebp+8]
.return:
pop edx
pop ecx
mov esp, ebp
pop ebp
ret 8

SimpleStrEqual: ;const char* str1, const char* str2
push ebp
mov ebp, esp
push ecx
push esi
push edi

push DWORD PTR ebp+8
call SimpleStrLen
mov ecx, eax

push DWORD PTR ebp+12
call SimpleStrLen
cmp ecx, eax
jne .NotMatch

cld
mov esi, [ebp+8]
mov edi, [ebp+12]
repe cmpsb

jne .NotMatch

xor eax, eax
or eax, 1
jmp .return
.NotMatch:
xor eax, eax
.return:
pop edi
pop esi
pop ecx
mov esp, ebp
pop ebp
ret 8

SimpleStrLen: ;const char* str
push ebp
mov ebp, esp
push ecx
xor eax, eax
mov ecx, [ebp+8]
.L1:
cmp BYTE PTR ecx+eax, 0
jz .return
inc eax
jmp .L1
.return:
pop ecx
mov esp, ebp
pop ebp
ret 4


_str:
	.LoadLibraryA.Pos = $-code_start
	.LoadLibraryA DB "LoadLibraryA",0
	.cheat_entry_dll.Pos = $-code_start
	.cheat_entry_dll DB "cheat_entry.dll",0
	
code_end = $

code_size = code_end-code_start