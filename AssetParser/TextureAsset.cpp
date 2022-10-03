#include "TextureAsset.hpp"

#include <lz4.h>
#include <nlohmann/json.hpp>

namespace Assets {

using namespace nlohmann;

TextureInfo ReadTextureInfo(AssetFile* file)
{
	json textureMetaData = json::parse(file->json);

	return TextureInfo {
		.size            = textureMetaData["bufferSize"],
		.compressionMode = textureMetaData["compression"],
		.format          = textureMetaData["format"],
		.pixelsSize      = {
		         textureMetaData["width"],
		         textureMetaData["height"],
		         0,
        },
		.originalFile = textureMetaData["originalFile"],
	};
}

void UnpackTexture(TextureInfo* info, const void* sourceBuffer, size_t sourceSize, void* destination)
{
	if (info->compressionMode == CompressionMode::LZ4)
	{
		LZ4_decompress_safe((const char*)sourceBuffer, (char*)destination, sourceSize, info->size);
	}
	else
	{
		memcpy(destination, sourceBuffer, sourceSize);
	}
}

AssetFile PackTexture(TextureInfo* info, void* pixelData)
{
	json textureMetadata;
	textureMetadata["format"]       = info->format;
	textureMetadata["width"]        = info->pixelsSize[0];
	textureMetadata["height"]       = info->pixelsSize[1];
	textureMetadata["bufferSize"]   = info->size;
	textureMetadata["originalFile"] = info->originalFile;
	textureMetadata["compression"]  = CompressionMode::LZ4;

	AssetFile file;
	file.type    = AssetFile::Type::Texture;
	file.version = 1u;

	const int compressStaging = LZ4_compressBound(info->size);
	file.blob.resize(compressStaging);
	const int compressedSize = LZ4_compress_default((const char*)pixelData, (char*)file.blob.data(), info->size, compressStaging);
	file.blob.resize(compressedSize);


	textureMetadata["compression"] = CompressionMode::LZ4;

	file.json = textureMetadata.dump();

	return file;
}

} // namespace Assets
