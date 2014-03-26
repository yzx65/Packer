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
		uint32_t sectionName = context->Edx;
		uint32_t stage2Data = context->Eax;
		uint32_t stage2Size = context->Ebx;
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
			mov esi, [sectionName]
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

	simpleDecrypt(context->Esi, reinterpret_cast<uint8_t *>(context->Edx), context->Ecx);
	simpleRLEDecompress(reinterpret_cast<const uint8_t *>(context->Edx), reinterpret_cast<uint8_t *>(context->Ebx));
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

	uint32_t pebAddress = __readfsdword(0x30);
	PEB *peb = reinterpret_cast<PEB *>(pebAddress);
	LDR_MODULE *myModule = reinterpret_cast<LDR_MODULE *>(peb->LoaderData->InLoadOrderModuleList.Flink);
	uint8_t *myBase = reinterpret_cast<uint8_t *>(myModule->BaseAddress);

	IMAGE_DOS_HEADER *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(myBase);
	IMAGE_NT_HEADERS32 *ntHeader = reinterpret_cast<IMAGE_NT_HEADERS32 *>(myBase + dosHeader->e_lfanew);
	IMAGE_SECTION_HEADER *sectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER *>(myBase + dosHeader->e_lfanew + sizeof(uint32_t)+ sizeof(IMAGE_FILE_HEADER)+ ntHeader->FileHeader.SizeOfOptionalHeader);

	uint32_t sectionName = *reinterpret_cast<uint32_t *>(sectionHeader[2].Name);
	uint8_t *sectionData = reinterpret_cast<uint8_t *>(sectionHeader[2].VirtualAddress + reinterpret_cast<size_t>(myBase));

	size_t sectionSize = sectionHeader[2].VirtualSize;
	__asm
	{
		mov edx, sectionName
		mov eax, sectionData
		mov ebx, sectionSize
		xor ecx, ecx
		in eax, 4
	}
}