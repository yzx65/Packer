#pragma once

#include "../Util/List.h"
#include "../Util/Map.h"
#include "../Util/String.h"
#include "../Util/SharedPtr.h"

class File;
class Option
{
private:
	SharedPtr<File> inputFile_;
	SharedPtr<File> outputFile_;
	Map<String, bool> booleanOptions_;
	Map<String, String> stringOptions_;

	void parseOptions(List<String> rawOptions);
	void handleStringOption(const String &name, const String &value);
	bool isBooleanOption(const String &optionName);
public:
	Option(const List<String> &args);

	SharedPtr<File> getInputFile() const;
	SharedPtr<File> getOutputFile() const;
};
