workspace "REV"
    architecture   "x64"
    preferredtoolarchitecture "x86_64"
    configurations { "debug", "release", "nsight" }
    location       "../.."
    startproject   "sandbox"

include "premake_rev.lua"
include "premake_sandbox.lua"
