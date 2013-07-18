#pragma once

#include <list>
#include <memory>

#include "Option.h"

class File;
class FormatBase;

class PackerMain
{
private:
	const Option &option_;
	std::list<std::string> loadedFiles_;

	void processFile(std::shared_ptr<File> file);
	std::list<std::shared_ptr<FormatBase>> loadImport(std::shared_ptr<FormatBase> input);
public:
	PackerMain(const Option &option);
	int process();
};
