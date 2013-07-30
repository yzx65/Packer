#pragma once

#include "List.h"
#include "Map.h"
#include "String.h"
#include "SharedPtr.h"

class File;
class Option
{
private:
	List<SharedPtr<File>> inputFiles_;
	Map<String, bool> booleanOptions_;
	Map<String, String> stringOptions_;

	void parseOptions(int argc, List<String> rawOptions);
	void handleStringOption(const String &name, const String &value);
	bool isBooleanOption(const String &optionName);
public:
	Option(int argc, char **argv);

	List<SharedPtr<File>> getInputFiles() const;
};
