solution "VulkanDemo"
    language "C++"
    location "../build"

    configurations {
        "Debug",
        "Release",
    }

    platforms { 
        "x32",
        "x64",
    }

    configuration "Debug"
        defines "DEBUG"
        flags { 
            "Symbols"
        }

    configuration "Release"
        defines "RELEASE"
        buildoptions { 
            "-O3"
        }

    project "Demo"
        location "../build/Demo"
        kind "ConsoleApp"        
        objdir "../build/Demo/obj"

        files {
            "../src/*.cc",
            "../src/**.h"
        }

        includedirs {
            "../src/include"
        }

        configuration "Debug"
            targetdir "../bin/Demo/Debug"

        configuration "Release"
            targetdir "../bin/Demo/Release"

