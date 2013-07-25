#pragma once

#include "Option.h"
#include "List.h"
#include "SharedPtr.h"

class File;
class FormatBase;

class PackerMain
{
private:
	const Option &option_;
	List<String> loadedFiles_;

	void processFile(SharedPtr<File> file);
	List<SharedPtr<FormatBase>> loadImport(SharedPtr<FormatBase> input);
public:
	PackerMain(const Option &option);
	int process();
};
