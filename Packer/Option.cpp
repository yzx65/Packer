#include "Option.h"

#include "File.h"
#include "Util.h"

Option::Option(int argc, char **argv)
{
	List<String> rawOptions;
	for(int i = 1; i < argc; i ++)
		rawOptions.push_back(String(argv[i]));
	parseOptions(argc, rawOptions);
}

Option::Option(int argc, wchar_t **argv)
{
	List<String> rawOptions;
	for(int i = 1; i < argc; i ++)
		rawOptions.push_back(WStringToString(WString(argv[i])));
	parseOptions(argc, rawOptions);
}

bool Option::isBooleanOption(const String &optionName)
{
	return false;
}

void Option::handleStringOption(const String &name, const String &value)
{

}

void Option::parseOptions(int argc, List<String> rawOptions)
{
	for(auto it = rawOptions.begin(); it != rawOptions.end(); it ++)
	{
		if(it->length() > 0 && it->at(0) == '-')
		{
			String optionName = it->substr(1);
			if(isBooleanOption(optionName))
				booleanOptions_[optionName] = true;
			else
			{
				it ++;
				if(it != rawOptions.end())
					handleStringOption(optionName, *it);
			}
		}
		else
		{
			//input file
			inputFiles_.push_back(File::open(*it));
		}
	}
}

List<std::shared_ptr<File>> Option::getInputFiles() const
{
	return inputFiles_;
}
