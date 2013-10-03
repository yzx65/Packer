#include "../../Packer/Win32Runtime.h"
#include "../../Packer/PEFormat.h"
#include "../../Packer/Win32File.h"

void Entry()
{
	Win32NativeHelper::get()->init(nullptr);
	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	if(arguments.size() < 4)
		return;

	auto it = arguments.begin();
	Win32File stage1(*it ++);
	Win32File stage2(*it ++);
	Win32File result(*it ++, true);

	Vector<uint8_t> data;
	data.push_back(1);
	result.write(data);
}