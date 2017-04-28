project "CLW"
    kind "StaticLib"
    location "../RadeonRays/CLW"
    includedirs { "../RadeonRays/CLW", "../RadeonRays" }
    files { "../RadeonRays/CLW/**.h", "../RadeonRays/CLW/**.cpp", "../RadeonRays/CLW/CL/CLW.cl" }

    if os.is("macosx") then
        buildoptions "-std=c++11 -stdlib=libc++"
    else if os.is("linux") then
        buildoptions "-std=c++11 -fPIC"
        os.execute("rm -rf obj");
        end
    end

    configuration {}

    defines {"RR_EMBED_KERNELS=1"}
    os.execute( "python ../Tools/scripts/stringify.py " ..
                                os.getcwd() .. "/../RadeonRays/CLW/CL/ "  .. 
                                ".cl " ..
                                "opencl " ..
                                 "> ../RadeonRays/CLW/kernelcache/clwkernels_cl.h"
                                )
    print ">> CLW: CL kernels embedded"

    configuration {"x64", "Debug"}
        targetdir "../Bin/Debug/x64"
    configuration {"x64", "Release"}
        targetdir "../Bin/Release/x64"
    configuration {}
