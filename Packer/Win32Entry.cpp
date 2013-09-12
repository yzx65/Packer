#include "PackerMain.h"
#include "Win32Runtime.h"
#include "Util.h"

void WindowsEntry()
{
	String str = WStringToString(Win32NativeHelper::get()->getCommandLine());

	int argc = 0;
	char **argv;

	String item;
	Vector<String> items;
	bool quote = false;
	bool slash = false;
	for(size_t i = 0; i < str.length(); i ++)
	{
		if(slash == false && quote == false && str[i] == '\"')
		{
			quote = true;
			continue;
		}
		if(slash == false && quote == true && str[i] == '\"')
		{
			quote = false;
			continue;
		}
		if(slash == true && quote == true && str[i] == '\"')
		{
			item.push_back('\"');
			slash = false;
			continue;
		}
		if(slash == true && str[i] != '\"')
		{
			item.push_back('\\');
			slash = false;
		}
		if(slash == false && str[i] == '\\')
		{
			slash = true;
			continue;
		}
		if(quote == false && str[i] == ' ')
		{
			if(item.length() == 0)
				continue;
			argc ++;
			items.push_back(std::move(item));
			item = "";
			continue;
		}

		item.push_back(str[i]);
	}
	argc ++;
	items.push_back(std::move(item));

	argv = new char *[argc];
	int c = 0;
	for(auto &i : items)
		argv[c ++] = const_cast<char *>(i.c_str());

	PackerMain(Option(argc, argv)).process();
}