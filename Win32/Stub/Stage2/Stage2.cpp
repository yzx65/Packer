#include "../../../Runtime/PEFormat.h"
#include "../../Win32NativeHelper.h"
#include "../../Win32SysCall.h"
#include "../../../Util/DataSource.h"
#include "../../Win32Loader.h"
#include "../Win32Stub.h"

uint8_t *mainData;
uint8_t *impData;

void Execute();

int Entry()
{
	uint8_t *newLocation, *stage2Start;
	{
		Win32NativeHelper::get()->init();

		Win32StubStage2Header *stage2Header;

		size_t myBase = Win32NativeHelper::get()->getMyBase();
		PEFormat format;
		SharedPtr<MemoryDataSource> selfImageSource = MakeShared<MemoryDataSource>(reinterpret_cast<uint8_t *>(myBase), 0);
		format.load(selfImageSource, true);

		int cnt = 0;
		for(auto &i : format.getSections())
		{
			if(cnt == 4)
			{
				mainData = new uint8_t[i.data->size()];
				copyMemory(mainData, i.data->get(), i.data->size());
				uint32_t seed = *reinterpret_cast<const uint32_t *>(i.name.c_str());
				simpleDecrypt(seed, mainData, i.data->size());
			}
			else if(cnt == 5)
			{
				impData = new uint8_t[i.data->size()];
				copyMemory(impData, i.data->get(), i.data->size());
				uint32_t seed = *reinterpret_cast<const uint32_t *>(i.name.c_str());
				simpleDecrypt(seed, impData, i.data->size());
			}
			else
			{
				size_t entryAddress = reinterpret_cast<size_t>(Entry);
				if(i.baseAddress + myBase < entryAddress && entryAddress < i.baseAddress + i.size + myBase)
					stage2Header = reinterpret_cast<Win32StubStage2Header *>(i.data->get());
			}
			cnt ++;
		}

		//self relocation
		newLocation = reinterpret_cast<uint8_t *>(Win32SystemCaller::get()->allocateVirtual(0, stage2Header->imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		stage2Start = reinterpret_cast<uint8_t *>(stage2Header) + sizeof(Win32StubStage2Header);
		copyMemory(newLocation, stage2Start, stage2Header->imageSize);
		uint64_t *relocationData = reinterpret_cast<uint64_t *>(stage2Start + stage2Header->imageSize);
		for(size_t i = 0; i < stage2Header->numberOfRelocations; ++ i)
			*reinterpret_cast<int32_t *>(newLocation + relocationData[i]) += -reinterpret_cast<int32_t>(stage2Start) + reinterpret_cast<int32_t>(newLocation);
	}

	typedef void (*ExecuteType)();
	ExecuteType executeFunction = reinterpret_cast<ExecuteType>(reinterpret_cast<size_t>(Execute) - reinterpret_cast<int32_t>(stage2Start) + reinterpret_cast<int32_t>(newLocation));
	executeFunction();

	return 0;
}

void Execute()
{
	Win32NativeHelper::get()->init();
	Win32SystemCaller::get()->unmapViewOfSection(reinterpret_cast<void *>(Win32NativeHelper::get()->getMyBase()));

	Image mainImage = Image::unserialize(MakeShared<MemoryDataSource>(mainData)->getView(0), nullptr);

	List<Image> importImages;
	if(impData)
	{
		SharedPtr<MemoryDataSource> impDataSource = MakeShared<MemoryDataSource>(impData);
		uint32_t count = *reinterpret_cast<uint32_t *>(&impData[0]);
		size_t off = sizeof(count);
		size_t size = 0;

		for(size_t j = 0; j < count; ++ j)
		{
			importImages.push_back(Image::unserialize(impDataSource->getView(off), &size));
			off += size;
		}
	}

	Win32Loader loader(std::move(mainImage), std::move(importImages));
	loader.execute();
}