#include "Option.h"

#include "File.h"
#include "Util.h"

Option::Option(int argc, char **argv)
{
	List<std::string> rawOptions;
	for(int i = 1; i < argc; i ++)
		rawOptions.push_back(std::string(argv[i]));
	parseOptions(argc, rawOptions);
}

Option::Option(int argc, wchar_t **argv)
{
	List<std::string> rawOptions;
	for(int i = 1; i < argc; i ++)
		rawOptions.push_back(WStringToString(std::wstring(argv[i])));
	parseOptions(argc, rawOptions);
}

bool Option::isBooleanOption(const std::string &optionName)
{
	return false;
}

void Option::handleStringOption(const std::string &name, const std::string &value)
{

}

void Option::parseOptions(int argc, List<std::string> rawOptions)
{
	for(auto it = rawOptions.begin(); it != rawOptions.end(); it ++)
	{
		std::string arg = *it;
		if(arg.length() > 0 && arg.at(0) == '-')
		{
			std::string optionName(arg.begin(), arg.end());
			if(isBooleanOption(optionName))
				booleanOptions_[optionName] = true;
			else
			{
				it ++;
				if(it != rawOptions.end())
					handleStringOption(optionName, arg);
			}
		}
		else
		{
			//input file
			inputFiles_.push_back(File::open(arg));
		}
	}
}

List<std::shared_ptr<File>> Option::getInputFiles() const
{
	return inputFiles_;
}
