#include <cstdint>
#include "../Win32Stub.h"
#include "../../Packer/Win32Structure.h"
#include "../../Packer/PEHeader.h"
#include <intrin.h>

#pragma warning(disable:4733) //we don't use safeseh.

#define STAGE2_SIZE 2097152

uint32_t unused;
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

inline int unneededCopy(uint8_t *dst, const uint8_t *src)
{
	for(size_t i = 0; i < 1000; i ++)
		dst[i] = src[i];
	return src[0];
}

int __stdcall Handler(EXCEPTION_RECORD *record, void *, CONTEXT *context, void*)
{
	if(!context)
		return 0;
	if(context->Ecx == 0)
	{
		size_t stage2Data = context->Edi;
		int temp = context->Eip;
		_asm
		{
			sub esp, 14h
			mov ecx, cont
			mov [esp], ecx
		}
		if(reinterpret_cast<uint32_t>(record->ExceptionAddress) == context->Eip)
		{
			Handler(record, 0, nullptr, 0);
			unneededCopy(stage2, reinterpret_cast<uint8_t *>(stage2Data));
		}
		_asm
		{
			mov eax, [esp]
			retn 10h
cont:
			mov ecx, temp
			lea ebx, dword ptr stage2
			mov edx, [stage2Data]
			mov [eax], ebx //re-trigger exception handler, start copy

			mov eax, fs:[0] //top level exception handler
			mov eax, [eax] //next exception handler
			mov fs:[0], eax //remove this exception handler
		}
		Win32StubStage2Header *header = reinterpret_cast<Win32StubStage2Header *>(stage2);
		if(header->magic != WIN32_STUB_STAGE2_MAGIC || buildSignature(stage2 + sizeof(Win32StubStage2Header), header->imageSize) != header->signature)
			return ExceptionContinueSearch;

		context->Eip = reinterpret_cast<size_t>(stage2 + sizeof(Win32StubStage2Header) + header->entryPoint);
		return ExceptionContinueExecution; //continue to stage2 code
	}

	decompress(reinterpret_cast<const uint8_t *>(context->Edx), reinterpret_cast<uint8_t *>(context->Ebx));
	context->Eax = reinterpret_cast<size_t>(&unused); //don't trigger exception again.
	return ExceptionContinueExecution;
}

size_t getStage2DataAddress()
{
	uint32_t pebAddress;
#ifndef _WIN64 
	pebAddress = __readfsdword(0x30);
#elif defined(_WIN32)
	pebAddress = __readgsqword(0x60);
#endif
	PEB *peb = reinterpret_cast<PEB *>(pebAddress);
	LDR_MODULE *myModule = reinterpret_cast<LDR_MODULE *>(peb->LoaderData->InLoadOrderModuleList.Flink);
	uint8_t *myBase = reinterpret_cast<uint8_t *>(myModule->BaseAddress);

	IMAGE_DOS_HEADER *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(myBase);
#ifndef _WIN64
	IMAGE_NT_HEADERS32 *ntHeader = reinterpret_cast<IMAGE_NT_HEADERS32 *>(myBase + dosHeader->e_lfanew);
#elif defined(_WIN32)
	IMAGE_NT_HEADERS64 *ntHeader = reinterpret_cast<IMAGE_NT_HEADERS64 *>(myBase + dosHeader->e_lfanew);
#endif
	IMAGE_SECTION_HEADER *sectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER *>(myBase + dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + ntHeader->FileHeader.SizeOfOptionalHeader);
	char stage2Name[] = WIN32_STUB_STAGE2_SECTION_NAME;
	for(int i = 0; i < ntHeader->FileHeader.NumberOfSections; ++ i)
	{
		int j;
		for(j = 0; sectionHeader[i].Name[j] != 0; ++ j)
			if(sectionHeader[i].Name[j] != stage2Name[j])
				break;
		if(sectionHeader[i].Name[j] == stage2Name[j])
			return sectionHeader[i].VirtualAddress + reinterpret_cast<size_t>(myBase);
	}
	return 0;
}

int __declspec(naked) Entry()
{
	__asm
	{
		push Handler
		push fs:[0]
		mov fs:[0], esp //set exception handler

		call getStage2DataAddress
		mov edi, eax
		xor ecx, ecx
		in eax, 4
	}
}