#include "AssetParser.hpp"

#include <fstream>
#include <istream>
#include <ostream>

namespace Assets {

bool SaveBinaryFile(const char* path, const AssetFile& file)
{
	std::ofstream outstream(path, std::ios::binary | std::ios::out);

	outstream.write((const char*)&file.version, sizeof(uint32_t));
	outstream.write((const char*)&file.type, sizeof(AssetFile::Type));

	uint32_t jsonSize = file.json.size();
	uint32_t blobSize = file.blob.size();
	outstream.write((const char*)&jsonSize, sizeof(uint32_t));
	outstream.write((const char*)&blobSize, sizeof(uint32_t));

	outstream.write(file.json.c_str(), jsonSize);
	outstream.write((const char*)file.blob.data(), blobSize);

	outstream.close();

	outstream.close();

	return true;
}

bool LoadBinaryFile(const char* path, AssetFile& outFile)
{
	std::ifstream instream(path, std::ios::binary);
	instream.seekg(0ull);

	if (!instream.is_open())
		return false;

	instream.read((char*)&outFile.version, sizeof(uint32_t));
	instream.read((char*)&outFile.type, sizeof(AssetFile::Type));

	uint32_t jsonSize;
	uint32_t blobSize;
	instream.read((char*)&jsonSize, sizeof(uint32_t));
	instream.read((char*)&blobSize, sizeof(uint32_t));

	outFile.json.resize(jsonSize);
	outFile.blob.resize(blobSize);
	instream.read((char*)outFile.json.data(), jsonSize);
	instream.read((char*)outFile.blob.data(), blobSize);

	instream.close();

	return true;
}

} // namespace Assets
