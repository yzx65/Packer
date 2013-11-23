#include "../../Packer/PEFormat.h"
#include "../../Packer/Win32Runtime.h"
#include "../../Packer/DataSource.h"
#include "../../Packer/Win32Loader.h"
#include "../Win32Stub.h"

Image *mainImage;
List<Image> *importImages;

void Execute();

int Entry()
{
	Win32StubStage2Header *stage2Header;
	Win32NativeHelper::get()->init();

	mainImage = new Image();
	importImages = new List<Image>();

	size_t myBase = Win32NativeHelper::get()->getMyBase();
	PEFormat format;
	SharedPtr<MemoryDataSource> selfImageSource = MakeShared<MemoryDataSource>(reinterpret_cast<uint8_t *>(myBase), 0);
	format.load(selfImageSource, true);

	for(auto &i : format.getSections())
	{
		if(i.name == WIN32_STUB_MAIN_SECTION_NAME)
			*mainImage = Image::unserialize(i.data, nullptr);
		else if(i.name == WIN32_STUB_IMP_SECTION_NAME)
		{
			uint32_t count = *reinterpret_cast<uint32_t *>(i.data->get());
			size_t off = sizeof(count);
			size_t size = 0;

			for(size_t j = 0; j < count; ++ j)
			{
				importImages->push_back(Image::unserialize(i.data->getView(off, 0), &size));
				off += size;
			}
		}
		else
		{
			size_t entryAddress = reinterpret_cast<size_t>(Entry);
			if(i.baseAddress + myBase < entryAddress && entryAddress < i.baseAddress + i.size + myBase)
				stage2Header = reinterpret_cast<Win32StubStage2Header *>(i.data->get());
		}
	}

	//self relocation
	uint8_t *newLocation = reinterpret_cast<uint8_t *>(Win32NativeHelper::get()->allocateVirtual(0, stage2Header->imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	uint8_t *stage2Start = reinterpret_cast<uint8_t *>(stage2Header) + sizeof(Win32StubStage2Header);
	copyMemory(newLocation, stage2Start, stage2Header->imageSize);
	uint64_t *relocationData = reinterpret_cast<uint64_t *>(stage2Start + stage2Header->imageSize);
	for(size_t i = 0; i < stage2Header->numberOfRelocations; ++ i)
		*reinterpret_cast<int32_t *>(newLocation + relocationData[i]) += -reinterpret_cast<int32_t>(stage2Start) + reinterpret_cast<int32_t>(newLocation);

	typedef void (*ExecuteType)();
	ExecuteType executeFunction = reinterpret_cast<ExecuteType>(reinterpret_cast<size_t>(Execute) - reinterpret_cast<int32_t>(stage2Start) + reinterpret_cast<int32_t>(newLocation));
	executeFunction();

	return 0;
}

void Execute()
{
	Win32NativeHelper::get()->unmapViewOfSection(reinterpret_cast<void *>(Win32NativeHelper::get()->getMyBase()));

	Win32Loader loader(std::move(*mainImage), std::move(*importImages));
	loader.execute();
}