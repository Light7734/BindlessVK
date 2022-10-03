#pragma once

#include "AssetParser.hpp"

namespace Assets {

enum class TextureFormat
{
	None = 0,
	RGBA8,
};

struct TextureInfo
{
	size_t size;
	CompressionMode compressionMode;
	TextureFormat format;
	uint32_t pixelsSize[3];
	std::string originalFile;
};

TextureInfo ReadTextureInfo(AssetFile* file);

void UnpackTexture(TextureInfo* info, const void* sourceBuffer, size_t sourceSize, void* destination);

AssetFile PackTexture(TextureInfo* info, void* pixelData);

} // namespace Assets
