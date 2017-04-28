function fileExists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

newoption {
    trigger = "denoiser",
    description = "Use denoising on output"
}

newoption {
    trigger = "benchmark",
    description = "Benchmark with command line interface instead of App."
}

newoption {
    trigger     = "rpr",
    description = "Enable RadeonProRender API lib"
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
	buildoptions {"-fvisibility=hidden"}
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

if _OPTIONS["rpr"] then
	if fileExists("./Rpr/Rpr.lua") then
		dofile("./Rpr/Rpr.lua")
	end
	if fileExists("./RprTest/RprTest.lua") then
		dofile("./RprTest/RprTest.lua")
	end
	if fileExists("./RprLoadStore/RprLoadStore.lua") then
		dofile("./RprLoadStore/RprLoadStore.lua")
	end
end

if fileExists("./Baikal/Baikal.lua") then
	dofile("./Baikal/Baikal.lua")
end

if fileExists("./Gtest/gtest.lua") then
	dofile("./Gtest/gtest.lua")
end

if fileExists("./UnitTest/UnitTest.lua") then
	dofile("./UnitTest/UnitTest.lua")
end

if fileExists("./RadeonRays/RadeonRays/RadeonRays.lua") then
	dofile("./RadeonRays/RadeonRays/RadeonRays.lua")
end

if fileExists("./RadeonRays/CLW/CLW.lua") then
	dofile("./RadeonRays/CLW/CLW.lua")
end

if fileExists("./RadeonRays/Calc/Calc.lua") then
	dofile("./RadeonRays/Calc/Calc.lua")
end