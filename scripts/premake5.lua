workspace "FluxPak"
	filename "FluxPak_%{_ACTION}"
	basedir "../"
	configurations { "Debug", "Release" }
    platforms {"x86", "x64"}
    warnings "Extra"

    filter { "platforms:x64" }
		architecture "x64"
		defines {"x64"}

	filter { "platforms:x86" }
		architecture "x32"
		defines {"x86"}	

	filter { "configurations:Debug" }
		runtime "Debug"
	 	defines { "_DEBUG" }
	 	flags {  }
	 	symbols "On"
	 	optimize "Off"

	filter { "configurations:Release" }
	 	runtime "Release"
		defines { "NDEBUG" }
	 	flags {  }
	 	symbols "Off"
	 	optimize "Full"

	project "PakFileCompression"
		filename "FluxPak_%{_ACTION}"
		location "../"
		targetdir "../Build/$(ProjectName)_$(Platform)_$(Configuration)"
		objdir "!../Build/Intermediate/$(ProjectName)_$(Platform)_$(Configuration)"

		kind "ConsoleApp"
		characterset ("MBCS")
		defines { "_CONSOLE" }
		flags {"FatalWarnings"}
		language "C++"

		files
		{ 
			"../**.h",
			"../**.hpp",
			"../**.cpp",
			"../**.inl",
			"../**.c"
		}

newaction {
		trigger     = "clean",
		description = "Remove all binaries and generated files",

		execute = function()
			os.rmdir("../Build")
		end
	}