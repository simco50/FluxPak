# FluxPak

This minimal command line tool is made to create *.pak files for FluxEngine
The tool uses Zlib to compress files with a size above a certain threshold to gain disk space

## Usage:
FluxPak.exe
* -r [responsefile]
* -p [pafile filepath]
* -v [opt:version]

## Pak file

Each pak file follows the structure below:
* PakHeader
* Multiple PakFileEntries
* Data

~~~~
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
	unsigned int UncompressedSize = 0
	unsigned int CompressedSize = 0;
	unsigned int Offset = 0;
};
~~~~

## External Libraries
* LZ4 Compression
