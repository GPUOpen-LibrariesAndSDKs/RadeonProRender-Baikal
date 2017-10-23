--
-- Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
--
-- lua premake4 script file .
project "RprSupport"
    kind "SharedLib"
    location "../RprSupport"
    links {"Rpr"}
    includedirs {"../Rpr"}
    files { "../RprSupport/**.h", "../RprSupport/**.cpp", "../RprSupport/**.hpp" , "../RprSupport/**.cl" ,"../RprSupport/**.def" }

    if( os.is("linux") ) then
        buildoptions { '-std=c++0x' }
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
       
