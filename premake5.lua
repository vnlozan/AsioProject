workspace "AsioProject"
    architecture "x64"
    startproject "AsioProject"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

	flags
	{
		"MultiProcessorCompile"
	}

    defines
    {
        "ASIO_STANDALONE",
        "_CRT_SECURE_NO_WARNINGS"
    }

-- Example : Debug/Windows/x64
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "AsioProject"
    location "AsioProject"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

    targetdir ( "bin/" .. outputdir .. "/%{prj.name}" )
    objdir ( "bin-int/" .. outputdir .. "/%{prj.name}" )

-- All file extensions used in project
    files
    {
        "%{prj.name}/vendor/asio-1.18.2/**.hpp",
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

-- C/C++ -> General -> Additional Include Directories
    includedirs
    {
        "%{prj.name}/vendor/asio-1.18.2/include"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"
    
    filter "configurations:Debug"
        symbols "On"
    
    filter "configurations:Release"
        optimize "On"

    filter "configurations:Dist"
        optimize "On"

    filter "platforms:Win32"
        defines
        {
            "_WIN32_WINNT=0x0601"
        }