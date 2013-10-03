#include <cstdint>
#include <windows.h>

#pragma warning(disable:4733) //we don't use safeseh.

#pragma section(".stage2", write) //stage2 section should start at 0x50000000

__declspec(allocate(".stage2")) size_t entryPoint;
__declspec(allocate(".stage2")) uint8_t data[1]; //will be modified by packer

#define STAGE2_SIZE 16777216

uint8_t stage2[STAGE2_SIZE];

int Handler(EXCEPTION_RECORD *record, void *, CONTEXT *context, void*)
{
	if(context->Eax == reinterpret_cast<size_t>(stage2) && context->Edi == reinterpret_cast<size_t>(data)) //exception while copying
	{
		context->Esi = 0;
		context->Ecx = 0; //reset counter to exit
		return ExceptionContinueExecution;
	}
	if(context->Ecx == 0) //initialize copy, move start address to ecx
	{
		int temp = context->Eip;
		_asm
		{
			call get_eip
			jmp cont
get_eip: //unneeded, just for more complexity.
			mov eax, [esp]
			ret
cont:
			mov ecx, temp
			lea ebx, dword ptr stage2
			mov [eax], ebx //re-trigger exception handler, start copy

			mov eax, fs:[0] //top level exception handler
			mov eax, [eax] //next exception handler
			mov fs:[0], eax //remove this exception handler
		}
		return ExceptionContinueExecution; //continue to stage2 code
	}
	uint8_t *ptr = reinterpret_cast<uint8_t *>(context->Ecx);
	if(*ptr != 0)
		return ExceptionContinueSearch;

	_asm
	{
		mov ecx, 1 //loop initialization
		lea eax, stage2
		lea edi, data
		mov edx, STAGE2_SIZE
copy_loop:
		mov esi, ecx
		dec esi
		mov ebx, [edi + esi]
		mov [eax + esi], ebx //copy from data to stage2.
		cmp ecx, 0 //ecx is zero only if exception occurred.
		jz quit
		inc ecx
		cmp edx, ecx //end reached
		jg copy_loop
	}

quit:
	context->Eax = reinterpret_cast<size_t>(data); //don't trigger exception again.
	return ExceptionContinueExecution;
}

int __declspec(naked) Entry()
{
	__asm
	{
		push Handler
		push fs:[0]
		mov fs:[0], esp //set exception handler

		lea edx, stage2
		add edx, entryPoint
		xor eax, eax
		xor ecx, ecx
		jmp edx //triggers an exception
	}
}