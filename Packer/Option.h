#pragma once

#include <memory>
#include <string>
#include <map>

#include "List.h"

class File;
class Option
{
private:
	List<std::shared_ptr<File>> inputFiles_;
	std::map<std::string, bool> booleanOptions_;
	std::map<std::string, std::string> stringOptions_;

	void parseOptions(int argc, List<std::string> rawOptions);
	void handleStringOption(const std::string &name, const std::string &value);
	bool isBooleanOption(const std::string &optionName);
public:
	Option(int argc, char **argv);
	Option(int argc, wchar_t **argv);

	List<std::shared_ptr<File>> getInputFiles() const;
};
