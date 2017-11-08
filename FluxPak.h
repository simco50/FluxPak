#pragma once
#include <vector>

#ifdef _DEBUG
#pragma comment(lib, "zlib_DEBUG.lib")
#else
#pragma comment(lib, "zlib.lib")
#endif

class FluxPak
{
public:

	struct PakHeader
	{
		char ID[4] = { "PAK" };
		char Version = 0;
		char FolderPath[100];
		char PakName[50];
		unsigned char NumEntries = 0;
	};

	struct PakFileTableEntry
	{
		char FilePath[255];
		bool Compressed = false;
		unsigned int UncompressedSize = 0;
		unsigned int CompressedSize = 0;
		unsigned int Offset = 0;
	};

	enum class PAK_RESULT : int
	{
		OK = 0,
		ERROR_RESPONSEFILE_NOT_FOUND = -1,
		ERROR_COMPRESSION = -2,
		ERROR_DECOMPRESSION = -3,
		ERROR_FILE_TOO_LARGE = -4,
		ERROR_FILE_NOT_FOUND = -5,
		ERROR_PAK_NOT_FOUND = -6,
		ERROR_OUTPUTSTREAM_FAIL = -7,
	};

	enum class PAK_COMPRESSION_QUALITY
	{
		DEFAULT,
		FAST,
		HIGHQUALITY,
	};

	static PAK_RESULT CreatePakFile(const std::string& responseFilePath, const std::string& targetPath, std::string& virtualDirPath, PAK_COMPRESSION_QUALITY quality = PAK_COMPRESSION_QUALITY::DEFAULT);

	static PAK_RESULT LoadFileFromPak(const std::string& pakFilePath, const std::string& fileName, std::vector<char>& outputData);
	static PAK_RESULT LoadFileFromPak(PakHeader* pHeader, const std::string& fileName, std::vector<char>& outputData);

	static PAK_RESULT LoadPakData(const std::string& pakFileName, std::vector<char>& outputData);

	static bool IsError(const PAK_RESULT result);

	static std::string GetError(const PAK_RESULT result);

private:
	static std::string GetFileName(const std::string& filePath);
	static std::string GetDirectoryPath(const std::string& filePath);
	static void FixPath(std::string& filePath);

	//Minimum file size to compress
	static const int MIN_COMPRESSION_FILE_SIZE = 1048576;
	static const int PAK_FILE_VERSION = 1;

	static PAK_RESULT Decompress(void *pInData, size_t inDataSize, std::vector<char> &outData);

	static PAK_RESULT Compress(void *pInData, size_t inDataSize, std::vector<char> &outData, const PAK_COMPRESSION_QUALITY quality);
};