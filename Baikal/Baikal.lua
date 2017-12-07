project "Baikal"
    kind "StaticLib"
    location "../Baikal"
    links {"CLW", "Calc", "FreeImage"}
    files { "../Baikal/**.inl", "../Baikal/**.h", "../Baikal/**.cpp", "../Baikal/**.cl", "../Baikal/**.fsh", "../Baikal/**.vsh" }

    includedirs{ "../RadeonRays/RadeonRays/include", "../RadeonRays/CLW", "."}

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
        links {"glew", "OpenGL32", "glfw3"}
        libdirs {   "../3rdparty/glew/lib/%{cfg.platform}",
                    "../3rdparty/freeglut/lib/%{cfg.platform}",
                    "../3rdparty/embree/lib/%{cfg.platform}",
                    "../3rdparty/oiio/lib/%{cfg.platform}",
        "../3rdparty/glfw/lib/%{cfg.platform}" }

        configuration {"Debug"}
            links {"OpenImageIOD"}
        configuration {"Release"}
            links {"OpenImageIO"}
        configuration {}

        if _OPTIONS["fbx"] then
            defines {"ENABLE_FBX"}
            includedirs {"C:/Program Files/Autodesk/FBX/FBX SDK/2017.1/include"}

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

    if _OPTIONS["gltf"] then
        defines {"ENABLE_GLTF"}

        includedirs{"../3rdparty/FreeImage/include",
                    "../3rdparty/json/include",
                    "../3rdparty/RprSupport/include",
                    "../3rdparty/RprLoadStore/include",
                    "../3rdparty/ProRenderGLTF/include",
                    "../Rpr"}
        libdirs{"../3rdparty/FreeImage/lib/",
                "../3rdparty/RprSupport/lib/%{cfg.platform}",
                "../3rdparty/RprLoadStore/lib/%{cfg.platform}",
                "../3rdparty/ProRenderGLTF/lib/%{cfg.platform}"}
        links{"FreeImage",
              "RadeonProRender",
              "RprSupport64",
              "ProRenderGLTF"}
        postbuildcommands{'copy "..\\3rdparty\\FreeImage\\bin\\FreeImage.dll" "%{cfg.buildtarget.directory}"'}
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

    if _OPTIONS["enable_raymask"] then
       	configuration {}
	defines {"ENABLE_RAYMASK"}
    end

    if _OPTIONS["embed_kernels"] then
        defines {"BAIKAL_EMBED_KERNELS=1"}

        os.execute( "python ../Tools/scripts/baikal_stringify.py " ..
                            os.getcwd() .. "/Kernels/CL/ "  ..
                            ".cl " ..
                            "opencl " ..
                             "> ./Kernels/CL/cache/kernels.h"
                            )
        print ">> Baikal: CL kernels embedded"
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
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\ProRenderGLTF\\bin\\%{cfg.platform}\\ProRenderGLTF.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\RprLoadStore\\bin\\%{cfg.platform}\\RprLoadStore64.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\RprSupport\\bin\\%{cfg.platform}\\RprSupport64.dll" "%{cfg.buildtarget.directory}"'
        }
    end
