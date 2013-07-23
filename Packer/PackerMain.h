#pragma once

#include <memory>

#include "Option.h"
#include "List.h"

class File;
class FormatBase;

class PackerMain
{
private:
	const Option &option_;
	List<std::string> loadedFiles_;

	void processFile(std::shared_ptr<File> file);
	List<std::shared_ptr<FormatBase>> loadImport(std::shared_ptr<FormatBase> input);
public:
	PackerMain(const Option &option);
	int process();
};
