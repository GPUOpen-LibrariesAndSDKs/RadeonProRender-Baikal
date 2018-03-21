function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption {
    trigger = "enable_raymask",
    description = "Enable visibility flags for shapes (slows down an intersector)"
}

newoption {
    trigger = "denoiser",
    description = "Use denoising on output"
}

newoption {
    trigger     = "fbx",
    description = "Enable FBX import"
}

newoption {
    trigger     = "gltf",
    description = "Enable glTF import"
}

newoption {
    trigger     = "rpr",
    description = "Enable RadeonProRender API lib"
}

newoption {
    trigger     = "embed_kernels",
    description = "Embed CL kernels into binary module"
}

newoption {
    trigger     = "uberv2",
    description = "Enable UberV2 support"
}

solution "Baikal"
platforms {"x64"}
configurations { "Debug", "Release" }
language "C++"
flags { "NoMinimalRebuild", "EnableSSE", "EnableSSE2" }

-- define common includes
includedirs { ".","./3rdParty/include" }


dofile ("./OpenCLSearch.lua" )
defines {"USE_OPENCL"}

-- perform OS specific initializations
local targetName;
if os.is("macosx") then
    targetName = "osx"
end

if os.is("windows") then
    targetName = "win"
    defines{ "WIN32" }
    buildoptions { "/MP"  } --multiprocessor build
    defines {"_CRT_SECURE_NO_WARNINGS"}
elseif os.is("linux") then
    buildoptions {"-fvisibility=hidden -Wno-ignored-attributes"}
end

--make configuration specific definitions
configuration "Debug"
    defines { "_DEBUG" }
    flags { "Symbols" }
configuration "Release"
    defines { "NDEBUG" }
    flags { "Optimize" }

configuration {"x64", "Debug"}
    targetsuffix "64D"
configuration {"x64", "Release"}
    targetsuffix "64"

configuration {} -- back to all configurations

if _OPTIONS["uberv2"] then
	defines { "ENABLE_UBERV2" }
end

if _OPTIONS["rpr"] then
    if fileExists("./Rpr/Rpr.lua") then
        dofile("./Rpr/Rpr.lua")
    end
    if fileExists("./RprTest/RprTest.lua") then
        dofile("./RprTest/RprTest.lua")
    end
    if fileExists("./RprSupport/RprSupport.lua") then
        dofile("./RprSupport/RprSupport.lua")
    end
end

if fileExists("./Baikal/Baikal.lua") then
    dofile("./Baikal/Baikal.lua")
end

if fileExists("./BaikalStandalone/BaikalStandalone.lua") then
    dofile("./BaikalStandalone/BaikalStandalone.lua")
end

if fileExists("./Gtest/gtest.lua") then
    dofile("./Gtest/gtest.lua")
end

if fileExists("./BaikalTest/BaikalTest.lua") then
    dofile("./BaikalTest/BaikalTest.lua")
end

if fileExists("./RadeonRaysPremakeAdapter/RadeonRays.lua") then
    dofile("./RadeonRaysPremakeAdapter/RadeonRays.lua")
end

if fileExists("./RadeonRaysPremakeAdapter/CLW.lua") then
    dofile("./RadeonRaysPremakeAdapter/CLW.lua")
end

if fileExists("./RadeonRaysPremakeAdapter/Calc.lua") then
    dofile("./RadeonRaysPremakeAdapter/Calc.lua")
end
