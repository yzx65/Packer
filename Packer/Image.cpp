#include "Image.h"

//both serialization and unserialization are done on machine with same endian.

template<typename T>
void appendToVector(Vector<uint8_t> &dst, const T &src)
{
	dst.append(reinterpret_cast<const uint8_t *>(&src), sizeof(T));
}

template<typename Ty>
void appendToVector(Vector<uint8_t> &dst, const Vector<Ty> &src)
{
	appendToVector(dst, static_cast<uint32_t>(src.size()));
	dst.append(src);
}

void appendToVector(Vector<uint8_t> &dst, const uint8_t *data, size_t size)
{
	appendToVector(dst, static_cast<uint32_t>(size));
	dst.append(data, size);
}

template <>
void appendToVector(Vector<uint8_t> &dst, const String &src)
{
	appendToVector(dst, static_cast<uint32_t>(src.length()));
	dst.append(reinterpret_cast<const uint8_t *>(src.c_str()), src.length());
}

Vector<uint8_t> Image::serialize() const
{
	Vector<uint8_t> result;
#define A(...) appendToVector(result, __VA_ARGS__);
	A(static_cast<uint32_t>(0));
	
	//imageinfo
	A(info.architecture);
	A(info.baseAddress);
	A(info.entryPoint);
	A(info.flag);
	A(info.platformData);
	A(info.platformData1);
	A(info.size);

	A(filePath);
	A(fileName);

	A(nameExportLen);
	A(static_cast<uint32_t>(exports.size()));
	for(auto &i : exports)
	{
		A(i.address);
		A(i.forward);
		A(i.name);
		A(i.ordinal);
	}

	A(static_cast<uint32_t>(sections.size()));
	for(auto &i : sections)
	{
		A(i.name);
		A(i.baseAddress);
		A(i.size);
		A(i.flag);
	}

	A(static_cast<uint32_t>(imports.size()));
	for(auto &i : imports)
	{
		A(i.libraryName);
		A(static_cast<uint32_t>(i.functions.size()));
		for(auto &j : i.functions)
		{
			A(j.iat);
			A(j.name);
			A(j.ordinal);
		}
	}

	A(static_cast<uint32_t>(relocations.size()));
	for(auto &i : relocations)
		A(i);

	for(auto &i : sections)
		A(i.data->get(), i.data->size());

	A(header->get(), header->size());

#undef A
	*reinterpret_cast<uint32_t *>(&result[0]) = result.size();

	return result;
}

template<typename T>
T readFromVector(uint8_t *data, size_t &offset)
{
	offset += sizeof(T);
	return *reinterpret_cast<T *>(data + offset - sizeof(T));
}

template <>
String readFromVector(uint8_t *data, size_t &offset)
{
	uint32_t len = readFromVector<uint32_t>(data, offset);
	String result(data + offset, data + offset + len);
	offset += len;
	return result;
}

template <>
Vector<uint8_t> readFromVector(uint8_t *data, size_t &offset)
{
	uint32_t len = readFromVector<uint32_t>(data, offset);
	Vector<uint8_t> result(data + offset, data + offset + len);
	offset += len;
	return result;
}

template<typename T>
T readFromVector(uint8_t *data, size_t &offset, SharedPtr<DataView> original);

template<>
SharedPtr<DataView> readFromVector(uint8_t *data, size_t &offset, SharedPtr<DataView> original)
{
	uint32_t len = readFromVector<uint32_t>(data, offset);
	offset += len;
	return original->getView(offset - len, len);
}

Image Image::unserialize(SharedPtr<DataView> data_, size_t *processedSize)
{
	size_t offset = 0;
	uint8_t *data = data_->get();
	Image result;

	if(processedSize)
		*processedSize = *reinterpret_cast<uint32_t *>(data);
	offset = sizeof(uint32_t);

#define R(t, ...) readFromVector<t>(data, offset, __VA_ARGS__)
	result.info.architecture = R(ArchitectureType);
	result.info.baseAddress = R(uint64_t);
	result.info.entryPoint = R(uint64_t);
	result.info.flag = R(uint32_t);
	result.info.platformData = R(uint64_t);
	result.info.platformData1 = R(uint64_t);
	result.info.size = R(uint64_t);

	result.filePath = R(String);
	result.fileName = R(String);

	result.nameExportLen = R(uint32_t);
	uint32_t exportLen = R(uint32_t);
	result.exports.reserve(exportLen);
	for(size_t i = 0; i < exportLen; ++ i)
	{
		ExportFunction item;
		item.address = R(uint64_t);
		item.forward = R(String);
		item.name = R(String);
		item.ordinal = R(uint16_t);

		result.exports.push_back(std::move(item));
	}

	uint32_t sectionLen = R(uint32_t);
	for(size_t i = 0; i < sectionLen; ++ i)
	{
		Section item;
		item.name = R(String);
		item.baseAddress = R(uint64_t);
		item.size = R(uint64_t);
		item.flag = R(uint32_t);

		result.sections.push_back(std::move(item));
	}

	uint32_t importLen = R(uint32_t);
	for(size_t i = 0; i < importLen; ++ i)
	{
		Import item;
		item.libraryName = R(String);
		uint32_t functionLen = R(uint32_t);
		for(size_t j = 0; j < functionLen; ++ j)
		{
			ImportFunction function;
			function.iat = R(uint64_t);
			function.name = R(String);
			function.ordinal = R(uint16_t);

			item.functions.push_back(std::move(function));
		}

		result.imports.push_back(std::move(item));
	}

	uint32_t relocationLen = R(uint32_t);
	for(size_t i = 0; i < relocationLen; ++ i)
		result.relocations.push_back(R(uint64_t));

	for(auto &i : result.sections)
		i.data = R(SharedPtr<DataView>, data_);

	result.header = R(SharedPtr<DataView>, data_);
#undef R
	return result;
}
