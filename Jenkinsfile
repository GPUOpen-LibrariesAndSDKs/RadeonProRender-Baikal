
/*4*/
pipeline {
    agent {
        node {
            label 'Windows && VS2015 && BUILDER'
        } 
    }
    stages {
        stage('Build') {
            steps {
                bat '''
                set msbuild="C:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe"
                if not exist %msbuild% (
                    set msbuild="C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MSBuild.exe"
                )

                set target=build
                set maxcpucount=/maxcpucount 
                set PATH=C:\\Python27\\;%PATH%
                rem .\\Tools\\premake\\win\\premake5 --rpr vs2015
                .\\Tools\\premake\\win\\premake5 vs2015
                set solution=.\\Baikal.sln
                
                %msbuild% /target:%target% %maxcpucount% /property:Configuration=Debug;Platform=x64 %parameters% %solution%
                %msbuild% /target:%target% %maxcpucount% /property:Configuration=Release;Platform=x64 %parameters% %solution%
                '''
            }
        }
    }
}

