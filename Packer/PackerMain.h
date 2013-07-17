#pragma once

#include <list>
#include <memory>

#include "Option.h"

struct ImportLibrary;
class File;
class FormatBase;

class PackerMain
{
private:
	const Option &option_;
	std::list<std::shared_ptr<FormatBase>> loadedFiles_;

	void processFile(std::shared_ptr<File> file);
	std::list<ImportLibrary> loadImport(std::shared_ptr<FormatBase> input);
public:
	PackerMain(const Option &option);
	int process();
};
