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
	dst.append(data, size);
}

template <>
void appendToVector(Vector<uint8_t> &dst, const String &src)
{
	appendToVector(dst, static_cast<uint32_t>(src.length()));
	appendToVector(dst, reinterpret_cast<const uint8_t *>(src.c_str()), src.length());
}

Vector<uint8_t> Image::serialize() const
{
	Vector<uint8_t> result;
	appendToVector(result, static_cast<uint32_t>(0));
	
	//imageinfo
	appendToVector(result, info.architecture);
	appendToVector(result, info.baseAddress);
	appendToVector(result, info.entryPoint);
	appendToVector(result, info.flag);
	appendToVector(result, info.platformData);
	appendToVector(result, info.platformData1);
	appendToVector(result, info.size);

	appendToVector(result, filePath);
	appendToVector(result, fileName);

	appendToVector(result, nameExportLen);
	appendToVector(result, static_cast<uint32_t>(exports.size()));
	for(auto i : exports)
	{
		appendToVector(result, i.address);
		appendToVector(result, i.forward);
		appendToVector(result, i.name);
		appendToVector(result, i.ordinal);
	}

	appendToVector(result, static_cast<uint32_t>(sections.size()));
	for(auto i : sections)
	{
		appendToVector(result, i.name);
		appendToVector(result, i.baseAddress);
		appendToVector(result, i.size);
		appendToVector(result, i.flag);
	}

	appendToVector(result, static_cast<uint32_t>(imports.size()));
	for(auto i : imports)
	{
		appendToVector(result, i.libraryName);
		appendToVector(result, i.functions.size());
		for(auto j : i.functions)
		{
			appendToVector(result, j.iat);
			appendToVector(result, j.name);
			appendToVector(result, j.ordinal);
		}
	}

	appendToVector(result, static_cast<uint32_t>(relocations.size()));
	for(auto i : relocations)
		appendToVector(result, i);

	appendToVector(result, header);
	for(auto i : sections)
		appendToVector(result, i.data->map(), i.data->size());

	*reinterpret_cast<uint32_t *>(&result[0]) = result.size();

	return result;
}

Image Image::unserialize(const Vector<uint8_t> &data)
{
	return Image();
}
