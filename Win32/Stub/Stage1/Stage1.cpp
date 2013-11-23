#include <cstdint>
#include "../Win32Stub.h"
#include "../../Win32Structure.h"
#include "../../../Runtime/PEHeader.h"
#include "../../../Util/Util.h"
#include <intrin.h>

#pragma warning(disable:4733) //we don't use safeseh.

#define STAGE2_SIZE 2097152

uint8_t stage2[STAGE2_SIZE];

inline int unneededCopy(uint8_t *dst, const uint8_t *src)
{
	for(size_t i = 0; i < 1000; i ++)
		dst[i] = src[i];
	return src[0];
}

int __stdcall Handler(EXCEPTION_RECORD *record, void *, CONTEXT *context, void*)
{
	size_t temp;
	if(!context)
		return 0;
	if(context->Ecx == 0)
	{
		size_t stage2Data = context->Eax;
		size_t stage2Size = context->Ebx;
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
			mov ecx, [stage2Size]
			mov [eax], ebx //re-trigger exception handler, start copy

			mov eax, fs:[0] //top level exception handler
			mov eax, [eax] //next exception handler
			mov fs:[0], eax //remove this exception handler
		}
		Win32StubStage2Header *header = reinterpret_cast<Win32StubStage2Header *>(stage2);
		uint8_t *stage2Start = stage2 + sizeof(Win32StubStage2Header);
		if(header->magic != WIN32_STUB_STAGE2_MAGIC || buildSignature(stage2Start, header->imageSize) != header->signature)
			return ExceptionContinueSearch;

		//relocate
		uint64_t *relocationData = reinterpret_cast<uint64_t *>(stage2Start + header->imageSize);
		for(size_t i = 0; i < header->numberOfRelocations; ++ i)
			*reinterpret_cast<int32_t *>(stage2Start + relocationData[i]) += -static_cast<int32_t>(header->originalBase) + reinterpret_cast<int32_t>(stage2Start);

		context->Eip = reinterpret_cast<size_t>(stage2Start + header->entryPoint);
		return ExceptionContinueExecution; //continue to stage2 code
	}

	simpleDecrypt(reinterpret_cast<uint8_t *>(context->Edx), context->Ecx);
	decompress(reinterpret_cast<const uint8_t *>(context->Edx), reinterpret_cast<uint8_t *>(context->Ebx));
	context->Eax = reinterpret_cast<size_t>(&temp); //don't trigger exception again.
	return ExceptionContinueExecution;
}

int Entry()
{
	__asm
	{
		push Handler
		push fs:[0]
		mov fs:[0], esp //set exception handler
	}

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
	IMAGE_SECTION_HEADER *sectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER *>(myBase + dosHeader->e_lfanew + sizeof(uint32_t)+ sizeof(IMAGE_FILE_HEADER)+ ntHeader->FileHeader.SizeOfOptionalHeader);
	char stage2Name[] = WIN32_STUB_STAGE2_SECTION_NAME;
	for(int i = 0; i < ntHeader->FileHeader.NumberOfSections; ++ i)
	{
		int j;
		for(j = 0; sectionHeader[i].Name[j] != 0; ++ j)
		if(sectionHeader[i].Name[j] != stage2Name[j])
			break;
		if(sectionHeader[i].Name[j] == stage2Name[j])
		{
			size_t sectionSize = sectionHeader[i].VirtualSize;
			size_t address = sectionHeader[i].VirtualAddress + reinterpret_cast<size_t>(myBase);
			__asm
			{
				mov eax, address
				mov ebx, sectionSize
				xor ecx, ecx
				in eax, 4
			}
		}
	}
}