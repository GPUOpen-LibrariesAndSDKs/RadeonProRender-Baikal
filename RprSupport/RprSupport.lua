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
