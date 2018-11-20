#include "FluxPak.h"
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <algorithm>
#include "Paths.h"
#include <windows.h>
#include "External/LZ4/lz4.h"
#include "External/LZ4/lz4hc.h"

bool FluxPak::CreatePakFile(
	const std::string& responseFilePath,
	const std::string& targetPath,
	std::string& virtualDirPath,
	const int contentVersion,
	const bool compress,
	const int minCompressBias,
	const bool hq)
{
	//Open the response file
	std::ifstream responseFileStream(responseFilePath);
	if (responseFileStream.fail())
	{
		throw std::exception("Failed to open response file");
	}

	//Create the header
	PakHeader header = {};

	std::string pakName = Paths::GetFileName(targetPath);
	std::string fileDir = Paths::GetDirectoryPath(targetPath);
	memcpy(header.PakName, pakName.data(), pakName.length() + 1);
	memcpy(header.FolderPath, fileDir.data(), fileDir.length() + 1);

	Paths::FixPath(virtualDirPath);

	header.NumEntries = 0;
	header.PakVersion = (char)PAK_FILE_VERSION;
	header.ContentVersion = contentVersion;

	std::vector<char> dataBuffer;
	std::vector<PakFileTableEntry> fileEntries;

	int compressBias = minCompressBias >= 0 ? minCompressBias : MIN_COMPRESSION_FILE_SIZE;

	//Iterate over all the files in the response file
	std::string filePath;
	while (std::getline(responseFileStream, filePath))
	{
		for (char& c : filePath)
			c = (char)tolower(c);

		std::ifstream fileStream(filePath, std::ios::ate | std::ios::binary);

		if (fileStream.fail())
		{
			std::cout << "Warning: File not found '" << filePath << "'" << std::endl;
			continue;
		}

		//Create the file entry
		PakFileTableEntry pakFileEntry;
		memset(&pakFileEntry, 0, sizeof(PakFileTableEntry));
		pakFileEntry.UncompressedSize = (unsigned int)fileStream.tellg();
		std::string fileName = filePath.substr(virtualDirPath.size());
		memcpy(pakFileEntry.FilePath, fileName.data(), fileName.length() + 1);
		pakFileEntry.Offset = (unsigned int)dataBuffer.size();
		pakFileEntry.Compressed = (pakFileEntry.UncompressedSize > (unsigned int)compressBias && compress == true);
		fileStream.seekg(0);

		//Read the file into memory
		std::vector<char> fileData;
		fileData.resize(pakFileEntry.UncompressedSize);
		fileStream.read(fileData.data(), pakFileEntry.UncompressedSize);

		//Compress the data
		if (pakFileEntry.Compressed)
		{
			std::vector<char> compressedData;
			bool result = CompressLZ4(fileData.data(), fileData.size(), hq, compressedData);
			if (result == false)
				return result;
			pakFileEntry.CompressedSize = (unsigned int)compressedData.size();
			dataBuffer.insert(dataBuffer.end(), compressedData.data(), compressedData.data() + compressedData.size());

			std::cout << "Compressed file '" << pakFileEntry.FilePath << "'. " << "Uncompressed = " << pakFileEntry.UncompressedSize << ". Compressed = " << pakFileEntry.CompressedSize << std::endl;
		}
		else
		{
			pakFileEntry.CompressedSize = pakFileEntry.UncompressedSize;
			dataBuffer.insert(dataBuffer.end(), fileData.begin(), fileData.end());
		}

		//Add the entry and close the file stream
		fileEntries.push_back(pakFileEntry);
		fileStream.close();
	}
	responseFileStream.close();

	header.NumEntries = (unsigned char)fileEntries.size();

	//Write to file
	std::ofstream output(targetPath, std::ios::binary);

	//Write header
	output.write(reinterpret_cast<char*>(&header), sizeof(PakHeader));

	//Write file table
	const unsigned int baseOffset = sizeof(PakHeader) + (unsigned int)fileEntries.size() * sizeof(PakFileTableEntry);
	for (PakFileTableEntry& e : fileEntries)
	{
		e.Offset += baseOffset;
		output.write(reinterpret_cast<char*>(&e), sizeof(PakFileTableEntry));
	}

	//Write file data
	output.write(dataBuffer.data(), dataBuffer.size());

	output.close();

	return true;
}

bool FluxPak::ExtractPakFile(const std::string& inputPath, const std::string& outputPath)
{
	std::ifstream pakFile(inputPath, std::ios::binary);
	if (pakFile.fail())
		throw std::exception("Failed to open pak file");
	PakHeader header;
	pakFile.read(reinterpret_cast<char*>(&header), sizeof(PakHeader));
	if (std::string(header.ID) != "PAK")
		throw std::exception("Pak file is of bad format");
	PakFileTableEntry* pEntries = new PakFileTableEntry[header.NumEntries];
	pakFile.read(reinterpret_cast<char*>(pEntries), sizeof(PakFileTableEntry) * header.NumEntries);

	for (int i = 0; i < header.NumEntries ; i++)
	{
		PakFileTableEntry& entry = pEntries[i];
		//pakFile.seekg(entry.Offset);
		std::vector<char> buffer;
		if (entry.Compressed)
		{
			std::vector<char> compressedBuffer(entry.CompressedSize);
			pakFile.read(buffer.data(), buffer.size());
			if (!DecompressLZ4(compressedBuffer.data(), compressedBuffer.size(), entry.UncompressedSize, buffer))
				throw std::exception("Failed to decompress file");
		}
		else
		{
			buffer.resize(entry.UncompressedSize);
			pakFile.read(buffer.data(), buffer.size());
		}
		CreateDirectoryA((outputPath + Paths::GetDirectoryPath(entry.FilePath)).c_str(), nullptr);
		std::ofstream outputStream(outputPath + entry.FilePath, std::ios::binary);
		if (outputStream.fail())
			throw std::exception("Failed to create file");
		outputStream.write(buffer.data(), buffer.size());
	}
	return true;
}

bool FluxPak::DecompressLZ4(const void *pInData, size_t compressedSize, size_t uncompressedSize, std::vector<char> &outData)
{
	outData.resize(uncompressedSize);
	const int decompressedSize = LZ4_decompress_safe((const char*)pInData, outData.data(), (int)compressedSize, (int)uncompressedSize);
	return decompressedSize > 0;
}

bool FluxPak::CompressLZ4(void *pInData, size_t inDataSize, const bool hc, std::vector<char> &outData)
{
	const int maxDstSize = LZ4_compressBound((int)inDataSize);
	outData.resize((size_t)maxDstSize);

	if (hc)
	{
		const int compressDataSize = LZ4_compress_HC((const char*)pInData, outData.data(), (int)inDataSize, (int)maxDstSize, LZ4HC_CLEVEL_MAX);
		if (compressDataSize < 0)
		{
			return false;
		}

		outData.resize((size_t)compressDataSize);
	}
	else
	{
		const int compressDataSize = LZ4_compress_default((const char*)pInData, outData.data(), (int)inDataSize, (int)maxDstSize);
		if (compressDataSize < 0)
		{
			return false;
		}

		outData.resize((size_t)compressDataSize);
	}
	return true;

}
