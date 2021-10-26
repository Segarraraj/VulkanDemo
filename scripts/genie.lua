solution "VulkanDemo"
    language "C++"
    location "../build"
    libdirs {
        "../deps/vulkan/Lib"
    }

    links {
        "vulkan-1"
    }

    configurations {
        "Debug",
        "Release",
        "Shipping"
    }

    platforms { 
        "x32",
        "x64",
    }

    configuration "Debug"
        defines {
            "DEBUG",
            "VERBOSE",
        }

        flags { 
            "Symbols"
        }

    configuration "Release"
        defines { 
            "RELEASE",
            "VERBOSE"
        }

        buildoptions { 
            "-O3"
        }

    configuration "Shipping"
        defines { 
            "SHIPPING",
        }

        buildoptions { 
            "-O3"
        }

    project "Demo"
        location "../build/Demo"
        kind "ConsoleApp"        
        objdir "../build/Demo/obj"

        files {
            "../src/**.cc",
            "../include/**.h",
            "../deps/**.h",
            "../deps/**.hpp",
            "../deps/**.cc",
            "../deps/**.cpp",
        }

        includedirs {
            "../include",
            "../deps/vulkan/Include",
            "../deps/glm/",
        }

        configuration "Debug"
            targetdir "../bin/Demo/Debug"

        configuration "Release"
            targetdir "../bin/Demo/Release"