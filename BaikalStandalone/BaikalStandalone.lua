project "BaikalStandalone"
    kind "ConsoleApp"
    location "../BaikalStandalone"
    links {"Baikal", "RadeonRays", "Calc", "CLW"}
    files { "../BaikalStandalone/**.inl", "../BaikalStandalone/**.h", "../BaikalStandalone/**.cpp", "../BaikalStandalone/**.cl", "../BaikalStandalone/**.fsh", "../BaikalStandalone/**.vsh" }

    includedirs{ "../RadeonRays/RadeonRays/include", "../RadeonRays/CLW", "../Baikal", "."}

    if os.is("macosx") then
        sysincludedirs {"/usr/local/include"}
        includedirs{"../3rdparty/glfw/include"}
        libdirs {"/usr/local/lib", "../3rdparty/glfw/lib/x64"}
        linkoptions{ "-framework OpenGL -framework CoreFoundation -framework CoreGraphics -framework IOKit -framework AppKit -framework QuartzCore" }
        buildoptions "-std=c++14 -stdlib=libc++"
        links {"OpenImageIO", "glfw3"}
    end

    if os.is("windows") then
        includedirs { "../3rdparty/glew/include", "../3rdparty/freeglut/include",
        "../3rdparty/oiio/include", "../3rdparty/glfw/include"}
        links {"glew", "OpenGL32", "glfw3", "FreeImage"}
        libdirs {   "../3rdparty/glew/lib/%{cfg.platform}",
                    "../3rdparty/freeglut/lib/%{cfg.platform}",
                    "../3rdparty/embree/lib/%{cfg.platform}",
                    "../3rdparty/FreeImage/lib/",
                    "../3rdparty/oiio/lib/%{cfg.platform}",
        "../3rdparty/glfw/lib/%{cfg.platform}" }

        configuration {"Debug"}
            links {"OpenImageIOD"}
        configuration {"Release"}
            links {"OpenImageIO"}
        configuration {}

        if _OPTIONS["fbx"] then
            defines {"ENABLE_FBX"}
            ;includedirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/include"}
            includedirs {"K:/apps/FbxSDK/2017.1/include"}

            if _ACTION == "vs2015" then
                configuration {"x64", "Debug"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2015/x64/debug"}
                configuration {"x64", "Release"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2015/x64/release"}
                configuration {"x32", "Debug"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2015/x86/debug"}
                configuration {"x32", "Release"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2015/x86/release"}
                configuration {}
            end

            if _ACTION == "vs2013" then
                configuration {"x64", "Debug"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2013/x64/debug"}
                configuration {"x64", "Release"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2013/x64/release"}
                configuration {"x32", "Debug"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2013/x86/debug"}
                configuration {"x32", "Release"}
                    libdirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/lib/vs2013/x86/release"}
                configuration {}
            end

            links {"libfbxsdk-md"}
        end
    end

    if os.is("linux") then
        buildoptions "-std=c++14"
        includedirs { "../3rdparty/glfw/include"}
        links {"OpenImageIO", "pthread"}
        links{"GLEW", "GL", "glfw"}
        os.execute("rm -rf obj");
    end

    if _OPTIONS["denoiser"] then
        defines{"ENABLE_DENOISER"}
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
          'copy "..\\3rdparty\\glew\\bin\\%{cfg.platform}\\glew32.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\glfw\\bin\\%{cfg.platform}\\glfw3.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\embree.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\tbb.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIO.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"'
        }
    end
