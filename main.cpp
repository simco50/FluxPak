#include "FluxPak.h"
#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include "cxxopts.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	cxxopts::Options options(argv[0], "-r [ResponseFile] -p [Pakfile Name] -b [Virtual directory] -v [Content version]");
	options.add_options()
		("r, responsefile", "The input response file", cxxopts::value<std::string>())
		("p, pakfile", "The output pak file", cxxopts::value<std::string>())
		("b, virtualdir", "The virtual base directory", cxxopts::value<string>())
		("c, compress", "The content version of the pak file", cxxopts::value<bool>())
		("m, mincompressbias", "The minimum size of a file for it to be compressed", cxxopts::value<int>())
		("v, contentversion", "The content version of the pak file", cxxopts::value<int>());

	cxxopts::ParseResult result = options.parse(argc, argv);

	if (result.count("r") != 1 || result.count("p") != 1 || result.count("b") != 1)
	{
		cout << options.help() << endl;
		return 1;
	}
	string responseFilePath = result["r"].as<std::string>();
	string pakFileName = result["p"].as<std::string>();
	string baseDirectoryPath = result["b"].as<string>();
	int version = 0;
	int minCompressBias = -1;
	if (result.count("v"))
		version = result["v"].as<int>();
	bool compress = false;
	if (result.count("c") == 1)
		compress = true;
	if(result.count("m"))
		minCompressBias = result["m"].as<int>();

	LARGE_INTEGER start, end, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	FluxPak packer;
	cout << "Creating pak file..." << endl;
	try
	{
		packer.CreatePakFile(
			responseFilePath,
			pakFileName,
			baseDirectoryPath,
			version,
			compress,
			minCompressBias);
	}
	catch (std::exception e)
	{
		cout << e.what() << endl;
	}

	QueryPerformanceCounter(&end);
	cout << "Pak file created in " << (double)(end.QuadPart - start.QuadPart) / freq.QuadPart << " second(s)"<< endl;

	return 0;
}