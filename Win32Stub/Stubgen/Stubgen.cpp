#include "../../Packer/Win32Runtime.h"
#include "../../Packer/PEFormat.h"
#include "../../Packer/Win32File.h"
#include "../../Packer/Util.h"
#include "../../Packer/Vector.h"

#include "../Win32Stub.h"

#include <limits>

Vector<uint8_t> encodeSize(uint8_t flag, uint32_t number)
{
	//f1xxxxxx
	//f01xxxxx xxxxxxxx
	//f001xxxx xxxxxxxx xxxxxxxx
	//f0001xxx xxxxxxxx xxxxxxxx xxxxxxxx
	//f0000100 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	Vector<uint8_t> result;
	if(number < 0x40)
		result.push_back((flag << 7) | 0x40 | number);
	else if(number < 0x2000)
	{
		result.push_back((flag << 7) | 0x20 | ((number & 0x1f00) >> 8));
		result.push_back(number & 0xff);
	}
	else if(number < 0x100000)
	{
		result.push_back((flag << 7) | 0x10 | ((number & 0x0f0000) >> 16));
		result.push_back((number >> 8) & 0xff);
		result.push_back(number & 0xff);
	}
	else if(number < 0x8000000)
	{
		result.push_back((flag << 7) | ((number & 0x07000000) >> 24));
		result.push_back((number >> 16) & 0xff);
		result.push_back((number >> 8) & 0xff);
		result.push_back(number & 0xff);
	}
	else if(number < std::numeric_limits<uint32_t>::max())
	{
		result.push_back((flag << 7) | ((number & 0x07000000) >> 24));
		result.push_back((number >> 24) & 0xff);
		result.push_back((number >> 16) & 0xff);
		result.push_back((number >> 8) & 0xff);
		result.push_back(number & 0xff);
	}
	return result;
}

Vector<uint8_t> compress(const Vector<uint8_t> &originalData)
{
	Vector<uint8_t> control;
	Vector<uint8_t> data;

	uint16_t lastData = 0x100;
	size_t successionCount = 1;
	size_t nonSuccessionCount = 0;

	for(uint8_t byte : originalData)
	{
		if(byte == lastData)
		{
			if(nonSuccessionCount > 1)
			{
				control.append(encodeSize(0, nonSuccessionCount - 1));
				successionCount = 1;
				nonSuccessionCount = 0;
			}
			successionCount ++;
		}
		
		if(byte != lastData)
		{
			if(successionCount > 1)
			{
				control.append(encodeSize(1, successionCount));
				successionCount = 1;
				nonSuccessionCount = 0;
			}
			nonSuccessionCount ++;
			data.push_back(byte);
		}
		lastData = byte;
	}
	if(nonSuccessionCount > 1)
		control.append(encodeSize(0, nonSuccessionCount - 1));
	if(successionCount > 1)
		control.append(encodeSize(1, successionCount));

	Vector<uint8_t> result(4);
	*reinterpret_cast<uint32_t *>(result.get()) = control.size();
	result.append(control);
	result.append(data);

	return result;
}

void Entry()
{
	Win32NativeHelper::get()->init(nullptr);
	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	if(arguments.size() < 4)
		return;

	auto it = arguments.begin();
	SharedPtr<Win32File> stage1 = MakeShared<Win32File>(*it ++);
	SharedPtr<Win32File> stage2 = MakeShared<Win32File>(*it ++);
	SharedPtr<Win32File> result = MakeShared<Win32File>(*it ++, true);

	PEFormat stage1Format;
	stage1Format.load(stage1, false);
	PEFormat stage2Format;
	stage2Format.load(stage2, false);

	//serialize stage2
	size_t originalDataSize = sizeof(Win32StubStage2Header) + static_cast<size_t>(stage2Format.getInfo().size) + stage2Format.getRelocations().size() * sizeof(uint64_t);
	Vector<uint8_t> data(originalDataSize);
	Win32StubStage2Header *stage2Header = reinterpret_cast<Win32StubStage2Header *>(data.get());
	uint8_t *stage2Data = data.get() + sizeof(Win32StubStage2Header);
	uint64_t *relocationData = reinterpret_cast<uint64_t *>(data.get() + sizeof(Win32StubStage2Header) + stage2Format.getInfo().size);

	for(auto i : stage2Format.getSections())
		copyMemory(stage2Data + i.baseAddress + sizeof(stage2Header), i.data->get(), i.data->getSize());
	size_t cnt = 0;
	for(auto i : stage2Format.getRelocations())
		relocationData[cnt ++] = i;

	stage2Header->magic = WIN32_STUB_STAGE2_MAGIC;
	stage2Header->imageSize = static_cast<size_t>(stage2Format.getInfo().size);
	stage2Header->numberOfRelocations = stage2Format.getRelocations().size();

	Vector<uint8_t> compressedStage2 = compress(data);

}
