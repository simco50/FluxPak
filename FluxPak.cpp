#include "FluxPak.h"
#include <fstream>
#include <iostream>
#include <string>
#include <zlib.h>
#include <assert.h>
#include <algorithm>

FluxPak::PAK_RESULT FluxPak::CreatePakFile(const std::string& responseFilePath, const std::string& targetPath, std::string& virtualDirPath, const PAK_COMPRESSION_QUALITY quality)
{
	//Open the response file
	std::ifstream responseFileStream(responseFilePath);
	if (responseFileStream.fail())
		return PAK_RESULT::ERROR_RESPONSEFILE_NOT_FOUND;

	//Create the header
	PakHeader header = {};

	std::string pakName = GetFileName(targetPath);
	std::string fileDir = GetDirectoryPath(targetPath);
	memcpy(header.PakName, pakName.data(), pakName.length() + 1);
	memcpy(header.FolderPath, fileDir.data(), fileDir.length() + 1);

	FixPath(virtualDirPath);

	header.NumEntries = 0;
	header.Version = PAK_FILE_VERSION;

	std::vector<char> dataBuffer;
	std::vector<PakFileTableEntry> fileEntries;

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
		pakFileEntry.Compressed = pakFileEntry.UncompressedSize > MIN_COMPRESSION_FILE_SIZE;
		fileStream.seekg(0);

		//Read the file into memory
		std::vector<char> fileData;
		fileData.resize(pakFileEntry.UncompressedSize);
		fileStream.read(fileData.data(), pakFileEntry.UncompressedSize);

		//Compress the data
		if (pakFileEntry.Compressed)
		{
			std::vector<char> compressedData;
			PAK_RESULT result = Compress(fileData.data(), fileData.size(), compressedData, quality);
			if (IsError(result))
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

	return PAK_RESULT::OK;
}

FluxPak::PAK_RESULT FluxPak::LoadFileFromPak(const std::string& pakFilePath, const std::string& fileName, std::vector<char>& outputData)
{
	std::vector<char> pakFile;

	PAK_RESULT result = LoadPakData(pakFilePath, pakFile);;
	if (IsError(result))
		return result;
	
	PakHeader* pHeader = reinterpret_cast<PakHeader*>(pakFile.data());
	return LoadFileFromPak(pHeader, fileName, outputData);
}

FluxPak::PAK_RESULT FluxPak::LoadFileFromPak(FluxPak::PakHeader* pHeader, const std::string& fileName, std::vector<char>& outputData)
{
	PakFileTableEntry* pEntry = reinterpret_cast<PakFileTableEntry*>(reinterpret_cast<char*>(pHeader) + sizeof(PakHeader));
	for (unsigned char i = 0; i < pHeader->NumEntries; ++i)
	{
		if (strcmp(pEntry->FilePath, fileName.c_str()) != 0)
		{
			if (i == pHeader->NumEntries - 1)
				return PAK_RESULT::ERROR_FILE_NOT_FOUND;

			pEntry = reinterpret_cast<PakFileTableEntry*>(reinterpret_cast<char*>(pEntry) + sizeof(PakFileTableEntry));
		}
		else
			break;
	}

	char* pData = reinterpret_cast<char*>(pHeader) + pEntry->Offset + sizeof(PakHeader) + pHeader->NumEntries * sizeof(PakFileTableEntry);
	if (pEntry->Compressed)
	{
		PAK_RESULT result = Decompress(pData, pEntry->CompressedSize, outputData);
		if (IsError(result))
			return result;
	}
	else
	{
		outputData.resize(pEntry->UncompressedSize);
		memcpy(outputData.data(), pData, pEntry->UncompressedSize);
	}
	return PAK_RESULT::OK;
}

FluxPak::PAK_RESULT FluxPak::LoadPakData(const std::string& pakFileName, std::vector<char>& outputData)
{
	std::ifstream fileStream(pakFileName, std::ios::ate | std::ios::binary);
	if (fileStream.fail())
		return PAK_RESULT::ERROR_FILE_TOO_LARGE;
	size_t size = (size_t)fileStream.tellg();
	outputData.resize(size);
	fileStream.seekg(0);
	fileStream.read(outputData.data(), size);
	return PAK_RESULT::OK;
}

std::string FluxPak::GetFileName(const std::string& filePath)
{
	size_t slashPos = filePath.rfind('/');
	size_t dotPos = filePath.rfind('.');
	if (slashPos == std::string::npos)
	{
		if (dotPos == std::string::npos)
			return filePath;
		return filePath.substr(0, dotPos);
	}
	if (dotPos == std::string::npos)
		return filePath.substr(slashPos + 1);
	return filePath.substr(slashPos + 1, dotPos - slashPos - 1);
}

std::string FluxPak::GetDirectoryPath(const std::string& filePath)
{
	size_t slashPos = filePath.rfind('/');
	if (slashPos == std::string::npos)
		return filePath;
	return filePath.substr(0, slashPos);
}

void FluxPak::FixPath(std::string& filePath)
{
	replace(filePath.begin(), filePath.end(), '\\', '/');
}

FluxPak::PAK_RESULT FluxPak::Decompress(void *pInData, size_t inDataSize, std::vector<char> &outData)
{
	const size_t BUFSIZE = 128 * 1024;
	unsigned char temp_buffer[BUFSIZE];

	z_stream strm;
	strm.zalloc = 0;
	strm.zfree = 0;
	strm.next_in = reinterpret_cast<unsigned char*>(pInData);
	strm.avail_in = (uInt)inDataSize;
	strm.next_out = temp_buffer;
	strm.avail_out = BUFSIZE;

	inflateInit(&strm);

	while (strm.avail_in != 0)
	{
		int res = inflate(&strm, Z_NO_FLUSH);
		if (res != Z_OK && res != Z_STREAM_END)
		{
			return PAK_RESULT::ERROR_DECOMPRESSION;
		}
		if (strm.avail_out == 0)
		{
			outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE);
			strm.next_out = temp_buffer;
			strm.avail_out = BUFSIZE;
		}
	}

	int deflate_res = Z_OK;
	while (deflate_res == Z_OK)
	{
		if (strm.avail_out == 0)
		{
			outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE);
			strm.next_out = temp_buffer;
			strm.avail_out = BUFSIZE;
		}
		deflate_res = inflate(&strm, Z_FINISH);
	}

	if (deflate_res != Z_STREAM_END)
	{
		return PAK_RESULT::ERROR_DECOMPRESSION;
	}

	outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
	inflateEnd(&strm);

	return PAK_RESULT::OK;
}

FluxPak::PAK_RESULT FluxPak::Compress(void *pInData, size_t inDataSize, std::vector<char> &outData, const PAK_COMPRESSION_QUALITY quality)
{
	const size_t BUFSIZE = 128 * 1024;
	unsigned char temp_buffer[BUFSIZE];

	z_stream strm;
	strm.zalloc = 0;
	strm.zfree = 0;
	strm.next_in = reinterpret_cast<unsigned char*>(pInData);
	strm.avail_in = (uInt)inDataSize;
	strm.next_out = temp_buffer;
	strm.avail_out = BUFSIZE;

	switch (quality)
	{
	case PAK_COMPRESSION_QUALITY::FAST:
		deflateInit(&strm, Z_BEST_SPEED);
		break;
	case PAK_COMPRESSION_QUALITY::HIGHQUALITY:
		deflateInit(&strm, Z_BEST_COMPRESSION);
		break;
	case PAK_COMPRESSION_QUALITY::DEFAULT:
	default:
		deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		break;
	}

	while (strm.avail_in != 0)
	{
		int res = deflate(&strm, Z_NO_FLUSH);
		if (res != Z_OK)
		{
			return PAK_RESULT::ERROR_COMPRESSION;
		}

		if (strm.avail_out == 0)
		{
			outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE);
			strm.next_out = temp_buffer;
			strm.avail_out = BUFSIZE;
		}
	}

	int deflate_res = Z_OK;
	while (deflate_res == Z_OK)
	{
		if (strm.avail_out == 0)
		{
			outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE);
			strm.next_out = temp_buffer;
			strm.avail_out = BUFSIZE;
		}
		deflate_res = deflate(&strm, Z_FINISH);
	}

	if (deflate_res != Z_STREAM_END)
	{
		return PAK_RESULT::ERROR_COMPRESSION;
	}

	outData.insert(outData.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
	deflateEnd(&strm);

	return PAK_RESULT::OK;
}

bool FluxPak::IsError(const PAK_RESULT result)
{
	return (int)result < 0;
}

std::string FluxPak::GetError(const PAK_RESULT result)
{
	switch (result)
	{
	case PAK_RESULT::OK:
		return "Success";
	case PAK_RESULT::ERROR_RESPONSEFILE_NOT_FOUND:
		return "Response file not found";
	case PAK_RESULT::ERROR_COMPRESSION:
		return "Zlib compression failed";
	case PAK_RESULT::ERROR_DECOMPRESSION:
		return "Zlib decompression failed";
	case PAK_RESULT::ERROR_FILE_TOO_LARGE:
		return "File is too large";
	case PAK_RESULT::ERROR_FILE_NOT_FOUND:
		return "File not found";
	case PAK_RESULT::ERROR_PAK_NOT_FOUND:
		return "Pak not found";
	default:
		return "Unknown error";
	}
}
