#include <cstdint>
#include <windows.h>

#pragma warning(disable:4733) //we don't use safeseh.

#pragma section(".stage2", write) //stage2 section should start at 0x50000000

__declspec(allocate(".stage2")) size_t entryPoint;
__declspec(allocate(".stage2")) size_t dataSize;
__declspec(allocate(".stage2")) uint8_t data[1]; //will be modified by packer

#define STAGE2_SIZE 2097152

uint8_t stage2[STAGE2_SIZE];

const uint8_t *decodeSize(const uint8_t *ptr, uint8_t *flag, uint32_t *size)
{
	//f1xxxxxx
	//f01xxxxx xxxxxxxx
	//f001xxxx xxxxxxxx xxxxxxxx
	//f0001xxx xxxxxxxx xxxxxxxx xxxxxxxx
	//f0000100 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	*size = *ptr ++;
	*flag = (*size & 0x80) >> 7;
	if(*size & 0x40)
		*size &= 0x3f;
	else if(*size & 0x20)
	{
		*size &= 0x1f;
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x10)
	{
		*size &= 0x0f;
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x08)
	{
		*size &= 0x07;
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x04)
	{
		*size = *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}

	return ptr;
}

uint32_t decompress(const uint8_t *compressedData, uint8_t *decompressedData)
{
	uint32_t controlSize = *reinterpret_cast<const uint32_t *>(compressedData);
	const uint8_t *data = compressedData + controlSize + 4;
	const uint8_t *control = compressedData + 4;
	const uint8_t *controlPtr = control;
	uint32_t resultSize = 0;

	uint8_t flag;
	size_t size;
	while(controlPtr < control + controlSize)
	{
		controlPtr = decodeSize(controlPtr, &flag, &size);
		resultSize += size;
		if(flag)
		{
			//succession
			for(; size > 0; size --)
				*decompressedData ++ = *data;
			data ++;
		}
		else
		{
			//nonsuccession
			for(; size > 0; size --)
				*decompressedData ++ = *data ++;
		}
	}

	return resultSize;
}

inline size_t checkSignature()
{
	size_t check = 0;
	for(size_t i = 0; i < dataSize / 4; i ++)
		check += reinterpret_cast<uint32_t *>(data)[i];
	return check;
}

int __stdcall Handler(EXCEPTION_RECORD *record, void *, CONTEXT *context, void*)
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
			sub esp, 14h
			mov ecx, cont
			mov [esp], ecx
		}
		checkSignature();
		_asm
		{
			mov eax, [esp]
			retn 10h
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