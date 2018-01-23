#include "FluxPak.h"
#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include "cxxopts.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	cxxopts::Options options(argv[0], "-r [ResponseFile] -p [Pakfile Name] -b [Virtual directory]");
	options.add_options()
		("r, responsefile", "The input response file", cxxopts::value<std::string>())
		("p, pakfile", "The output pak file", cxxopts::value<std::string>())
		("b, virtualdir", "The virtual base directory", cxxopts::value<string>());

	cxxopts::ParseResult result = options.parse(argc, argv);

	if (result.count("r") != 1 || result.count("p") != 1 || result.count("b") != 1)
	{
		cout << options.help() << endl;
		return 1;
	}
	string responseFilePath = result["i"].as<std::string>();
	string pakFileName = result["o"].as<std::string>();
	string baseDirectoryPath = result["e"].as<string>();

	LARGE_INTEGER start, end, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	cout << "Creating pak file..." << endl;
	FluxPak::PAK_RESULT pakResult = FluxPak::CreatePakFile(responseFilePath, pakFileName, baseDirectoryPath, FluxPak::PAK_COMPRESSION_QUALITY::FAST);
	cout << FluxPak::GetError(pakResult) << endl;

	QueryPerformanceCounter(&end);
	cout << "Pak file created in " << (double)(end.QuadPart - start.QuadPart) / freq.QuadPart << " second(s)"<< endl;

	return 0;
}