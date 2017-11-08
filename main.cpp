#include "FluxPak.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <map>

using namespace std;

map<string, string> ParseArguments(int argc, char *argv[])
{
	map<string, string> arguments;

	string currentCommand = "";
	bool command = false;
	for (int i = 1; i < argc; i++)
	{
		if (command)
		{
			arguments[currentCommand] = argv[i];
			command = false;
		}
		else
		{
			if (argv[i][0] == '-')
			{
				command = true;
				string cmd = argv[i];
				currentCommand = cmd.substr(1);
			}
			else
			{
				command = false;
				arguments[argv[i]] = argv[i];
			}
		}
	}
	return arguments;
}

#define REQUIRE(argumentMap, argument, description) \
if(argumentMap.find(argument) == argumentMap.end()) \
{ \
cout << "Missing argument '-" << argument << "'. The script requires a " << description << "." << endl; \
cout << "Usage: " << endl; \
cout << "\t FluxPak.exe -r [responsefile] -p [pakfile filepath] -b [base virtual path]" << endl; \
return -1; \
}

int main(int argc, char *argv[])
{
	auto argumentMap = ParseArguments(argc, argv);

	REQUIRE(argumentMap, "r", "ResponseFile path");
	REQUIRE(argumentMap, "p", "Pakfile name");
	REQUIRE(argumentMap, "b", "Virtual directory");
	string responseFilePath = argumentMap["r"];
	string pakFileName = argumentMap["p"];
	string baseDirectoryPath = argumentMap["b"];

	LARGE_INTEGER start, end, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	cout << "Creating pak file..." << endl;
	FluxPak::PAK_RESULT result = FluxPak::CreatePakFile(responseFilePath, pakFileName, baseDirectoryPath, FluxPak::PAK_COMPRESSION_QUALITY::FAST);
	cout << FluxPak::GetError(result) << endl;

	QueryPerformanceCounter(&end);
	cout << "Pak file created in " << (double)(end.QuadPart - start.QuadPart) / freq.QuadPart << " second(s)"<< endl;

	return 0;
}