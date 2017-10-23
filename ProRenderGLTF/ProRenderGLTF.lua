project "ProRenderGLTF"
    kind "SharedLib"
    location "../ProRenderGLTF"

    if os.is("windows") then
        vpaths { ["Extensions"] = "./Extensions/*.h" }
        vpaths { ["Extensions"] = "./Extensions/*.cpp" }
        vpaths { [""] = "./*.h" }        
        vpaths { [""] = "./*.cpp" }
        vpaths { [""] = "./*.def" }
        
        files {
            "./Extensions/*.h", "./Extensions/*.cpp",
            "./*.h", "./*.cpp",
            "./*.def"
        }
        
        vpaths { ["/"] = "*.*" }
    end
  
    configuration {}
    
    linkoptions { '/DEF:"../ProRenderGLTF/ProRenderGLTF.def"' }
    
    defines { "RPR_GLTF_EXPORT" }
    
    includedirs {
        "./",
        "../3rdParty/json/include",
        "../3rdParty/FreeImage/include",
        "../Rpr/",
        "../RprSupport/"
    }
                  
    libdirs {
        "../3rdParty/FreeImage/lib",
        "../3rdParty/RadeonProRenderSDK/RadeonProRender/libWin64"
    }
    
    links {
        "FreeImage",
        -- "RadeonProRender64",
        "Rpr",
        "RprSupport"
    }    
