project "BaikalTest"
    kind "ConsoleApp"
    location "../BaikalTest"
    links {"RadeonRays", "CLW", "Calc", "Baikal", "Gtest"}
    files { "../BaikalTest/**.inl", "../BaikalTest/**.h", "../BaikalTest/**.cpp", "../BaikalTest/**.cl", "../BaikalTest/**.fsh", "../BaikalTest/**.vsh" }

    includedirs{ "../RadeonRays/RadeonRays/include", "../RadeonRays/CLW", "../Baikal", "../Gtest/include", "."}

    if os.is("macosx") then
        sysincludedirs {"/usr/local/include"}
        libdirs {"/usr/local/lib" }
        linkoptions{ "-framework CoreFoundation -framework AppKit" }
        buildoptions "-std=c++14 -stdlib=libc++"
        links {"OpenImageIO" }
    end

    if os.is("windows") then
        includedirs {"../3rdparty/oiio/include"}
        libdirs { "../3rdparty/oiio/lib/%{cfg.platform}" }

        configuration {"Debug"}
            links {"OpenImageIOD"}
        configuration {"Release"}
            links {"OpenImageIO"}
        configuration {}
    end

    if os.is("linux") then
        buildoptions "-std=c++11"
        links {"OpenImageIO", "pthread"}
        os.execute("rm -rf obj");
    end

    configuration {"x32", "Debug"}
        targetdir "../Bin/Debug/x86"
    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x32", "Release"}
        targetdir "../Bin/Release/x86"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}

    if os.is("windows") then
        postbuildcommands  {
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\embree.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\tbb.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIO.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"'
        }
    end
