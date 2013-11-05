#pragma once

#include "Option.h"
#include "List.h"
#include "SharedPtr.h"

class File;
class FormatBase;
struct Image;

class PackerMain
{
private:
	const Option &option_;
	List<String> loadedFiles_;

	void outputPE(Image &image, const List<Image> imports, SharedPtr<File> output);
	void processFile(SharedPtr<File> inputf, SharedPtr<File> output);
	List<Image> loadImport(SharedPtr<FormatBase> input);
public:
	PackerMain(const Option &option);
	int process();
};
