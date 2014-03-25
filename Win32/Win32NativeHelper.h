#pragma once

#include <cstdint>
#include "../Util/List.h"
#include "../Util/String.h"

struct _PEB;
typedef struct _PEB PEB;
struct _PEB64;
typedef struct _PEB64 PEB64;

struct Win32LoadedImage
{
	const wchar_t *fileName;
	uint64_t baseAddress;
};

class Win32NativeHelper
{
private:
	PEB *myPEB_;
	PEB64 *myPEB64_;
	size_t myBase_;
	bool isWoW64_;

	uint32_t getPEB64();
	void initHeap();
	void initModuleList();
	void relocateSelf(void *entry);
public:
	void init();
	uint8_t *getApiSet();

	wchar_t *getCommandLine();
	wchar_t *getCurrentDirectory();
	wchar_t *getEnvironments();
	PEB *getPEB();
	List<Win32LoadedImage> getLoadedImages();
	List<String> getArgumentList();

	size_t getMyBase() const;
	void setMyBase(size_t address);

	String getSystem32Directory() const;
	String getSysWOW64Directory() const;

	bool isWoW64();
	void showError(const String &message);

	uint32_t getRandomValue();
	static Win32NativeHelper *get();
};
