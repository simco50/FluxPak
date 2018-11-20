#pragma once

class Paths
{
public:
	static bool Paths::IsSlash(const char c)
	{
		if (c == '\\')
			return true;
		return c == '/';
	}

	static std::string Paths::GetFileName(const std::string& filePath)
	{
		auto it = std::find_if(filePath.rbegin(), filePath.rend(), [](const char c)
		{
			return IsSlash(c);
		});
		if (it == filePath.rend())
			return filePath;

		return filePath.substr(it.base() - filePath.begin());
	}

	static std::string Paths::GetFileNameWithoutExtension(const std::string& filePath)
	{
		std::string fileName = GetFileName(filePath);
		size_t dotPos = fileName.find('.');
		if (dotPos == std::string::npos)
			return fileName;
		return fileName.substr(0, dotPos);
	}

	static std::string Paths::GetFileExtenstion(const std::string& filePath)
	{
		size_t dotPos = filePath.rfind('.');
		if (dotPos == std::string::npos)
			return filePath;
		return filePath.substr(dotPos + 1);
	}

	static std::string Paths::GetDirectoryPath(const std::string& filePath)
	{
		auto it = std::find_if(filePath.rbegin(), filePath.rend(), [](const char c)
		{
			return IsSlash(c);
		});
		if (it == filePath.rend())
		{
			if (filePath.rfind('.'))
				return "/";
			return filePath;
		}

		return filePath.substr(0, it.base() - filePath.begin());
	}

	static void FixPath(std::string& path)
	{
		for (char& c : path)
		{
			if (c == '\\')
				c = '/';
		}
	}
};