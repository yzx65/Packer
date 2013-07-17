#pragma once

#include <memory>
#include <string>
#include <list>
#include <map>

#include "Util.h"
#include "File.h"

class Option
{
private:
	std::list<std::shared_ptr<File>> inputFiles_;
	std::map<std::string, bool> booleanOptions_;
	std::map<std::string, std::string> stringOptions_;

	void parseOptions(int argc, std::list<std::string> rawOptions)
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
						handleStringOption(optionName, *it);
				}
			}
			else
			{
				//input file
				inputFiles_.push_back(File::open(arg));
			}
		}
	}

	template<typename StringType>
	void handleStringOption(const std::string &name, const StringType &value)
	{

	}
	bool isBooleanOption(const std::string &optionName);
public:
	Option(int argc, char **argv)
	{
		std::list<std::string> rawOptions;
		for(int i = 1; i < argc; i ++)
			rawOptions.push_back(std::string(argv[i]));
		parseOptions(argc, rawOptions);
	}

	Option(int argc, wchar_t **argv)
	{
		std::list<std::string> rawOptions;
		for(int i = 1; i < argc; i ++)
			rawOptions.push_back(WStringToString(std::wstring(argv[i])));
		parseOptions(argc, rawOptions);
	}


	std::list<std::shared_ptr<File>> getInputFiles() const
	{
		return inputFiles_;
	}
};
