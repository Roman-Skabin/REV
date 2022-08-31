project "engine"
    targetname "%{prj.name}"

    location    "%{wks.location}/%{prj.name}"
    targetdir   "%{wks.location}/bin/%{wks.system}/$(Configuration)"
    objdir      "!%{prj.targetdir}/obj/%{prj.name}/!"
    debugdir    "%{prj.targetdir}"
    symbolspath "%{prj.targetdir}/%{prj.name}.pdb"
    includedirs "%{prj.location}"

    kind "SharedLib"

    language   "C++"
    cppdialect "C++17"
    compileas  "C++"

    pchheader "core/pch.h"
    pchsource "../../engine/core/pch.cpp" -- @NOTE(Roman): ${prj.location}/core/pch.cpp does not work for some reason.
    files
    {
        "%{prj.location}/**.h",
        "%{prj.location}/**.hpp",
        "%{prj.location}/**.inl",
        "%{prj.location}/**.ipp",
        "%{prj.location}/**.cpp"
    }

    staticruntime     "on"
    systemversion     "latest"
    inlining          "auto"
    intrinsics        "on"
    vectorextensions  "AVX2"
    floatingpoint     "fast"
    exceptionhandling "off"
    nativewchar       "off"
    flags             "NoManifest"
    defines           "_REV_DEV"

    filter "system:windows"
        characterset      "MBCS"
        prebuildcommands  "%{wks.location}/build/ctime/ctime.exe -begin %{wks.location}/build/ctime/%{prj.name}.ctm"
        postbuildcommands "%{wks.location}/build/ctime/ctime.exe -end   %{wks.location}/build/ctime/%{prj.name}.ctm"

    filter "configurations:debug"
        optimize        "off"
        symbols         "on"
        editAndContinue "off"
        defines         "_REV_CHECKS_BREAK"
        runtime         "debug"

    filter "configurations:release"
        optimize                   "speed"
        symbols                    "off"
        flags                      { "NoIncrementalLink", "LinkTimeOptimization", "NoBufferSecurityCheck" }
        removeunreferencedcodedata "on"
        defines                    "_REV_NO_CHECKS"
        runtime                    "release"
    
    filter "configurations:nsight"
        optimize        "off"
        symbols         "on"
        editAndContinue "off"
        defines         "_REV_NO_CHECKS"
        runtime         "release"
