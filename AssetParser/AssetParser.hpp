#pragma once

#include <string>
#include <vector>

namespace Assets {

struct AssetFile
{
	uint32_t version;

	enum class Type : uint32_t
	{
		Texture,
		Mesh,
		Material,
	} type;

	std::string json;
	std::vector<uint8_t> blob;
};

enum class CompressionMode : uint32_t
{
	None,
	LZ4,
	LZ4HC,
};

bool SaveBinaryFile(const char* path, const AssetFile& inFile);
bool LoadBinaryFile(const char* path, AssetFile& outFile);

} // namespace Assets
