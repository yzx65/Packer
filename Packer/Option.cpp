#include "Option.h"

#include "File.h"

Option::Option(const List<String> &args)
{
	parseOptions(args);
}

bool Option::isBooleanOption(const String &optionName)
{
	return false;
}

void Option::handleStringOption(const String &name, const String &value)
{

}

void Option::parseOptions(List<String> rawOptions)
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

List<SharedPtr<File>> Option::getInputFiles() const
{
	return inputFiles_;
}
