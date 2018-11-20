#pragma once
#include <vector>
class FluxPak
{
public:
	struct PakHeader
	{
		char ID[4] = { "PAK" };
		char PakVersion = 0;
		int ContentVersion = 0;
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

	bool CreatePakFile(
		const std::string& responseFilePath,
		const std::string& targetPath,
		std::string& virtualDirPath,
		const int contentVersion,
		const bool compress,
		const int minCompressBias);

	bool ExtractPakFile(
		const std::string& inputPath,
		const std::string& outputPath
	);

private:
	//Minimum file size to compress
	const int MIN_COMPRESSION_FILE_SIZE = 524288;
	const int PAK_FILE_VERSION = 2;

	bool DecompressLZ4(const void *pInData, size_t compressedSize, size_t uncompressedSize, std::vector<char> &outData);
	bool CompressLZ4(void *pInData, size_t inDataSize, std::vector<char> &outData);
};