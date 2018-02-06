project "RprSupport"
    kind "SharedLib"
    location "../RprSupport"
    links {"RadeonProRender", "RadeonRays", "CLW", "Calc"}
    files { "../RprSupport/**.h", "../RprSupport/**.cpp"}
    includedirs{ "../Rpr", "../RadeonRays/RadeonRays/include", "../RadeonRays/CLW", "../3rdparty/RprSupport/include", "../Baikal", "../BaikalStandalone", "." }

    defines {"RPR_EXPORT_API"}

    if os.is("macosx") then
        sysincludedirs {"/usr/local/include"}
        libdirs {"/usr/local/lib"}
        linkoptions{ "-framework OpenGL", "-framework GLUT" }
        buildoptions "-std=c++14 -stdlib=libc++"
    end

    if os.is("windows") then
        linkoptions { '/DEF:"RprSupport.def"' }

        links {"RadeonProRender",}
        links {"glew", "OpenGL32"}
        libdirs {   "../3rdparty/glew/lib/%{cfg.platform}",
                    "../3rdparty/freeglut/lib/%{cfg.platform}",
                    "../3rdparty/embree/lib/%{cfg.platform}",
                    "../3rdparty/oiio/lib/%{cfg.platform}"}
    end

    if os.is("linux") then
        buildoptions "-std=c++14"
        links {"pthread",}
        os.execute("rm -rf obj");
    end

    if _OPTIONS["use_vulkan"] then
        local vulkanSDKPath = os.getenv( "VK_SDK_PATH" );
        if vulkanSDKPath == nil then
            vulkanSDKPath = os.getenv( "VULKAN_SDK" );
        end
        if vulkanSDKPath ~= nil then
            configuration {"x32"}
            libdirs { vulkanSDKPath .. "/Bin32" }
            configuration {"x64"}
            libdirs { vulkanSDKPath .. "/Bin" }
            configuration {}
        end
        if os.is("macosx") then
            --no Vulkan on macOs need to error out TODO
        elseif os.is("linux") then
            libdirs { vulkanSDKPath .. "/lib" }
            links { "vulkan"}
        elseif os.is("windows") then
            links {"Anvil"}
            links{"vulkan-1"}
        end
    end

    configuration {"x64", "Debug"}
        targetsuffix "64"
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
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\embree.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\embree\\bin\\%{cfg.platform}\\tbb.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIO.dll" "%{cfg.buildtarget.directory}"',
          'copy "..\\3rdparty\\oiio\\bin\\%{cfg.platform}\\OpenImageIOD.dll" "%{cfg.buildtarget.directory}"'
        }
    end
