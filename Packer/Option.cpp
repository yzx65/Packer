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
	if(name == "o")
		outputFile_ = File::open(value, true);
}

void Option::parseOptions(List<String> rawOptions)
{
	auto it = rawOptions.begin();
	it ++;
	for(; it != rawOptions.end(); it ++)
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
			inputFile_ = File::open(*it);
	}
}

SharedPtr<File> Option::getInputFile() const
{
	return inputFile_;
}

SharedPtr<File> Option::getOutputFile() const
{
	return outputFile_;
}
