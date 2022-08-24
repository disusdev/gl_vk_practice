printf ("Premake generation started...")

LANG = "C"
BIN_DIR = path.join(_MAIN_SCRIPT_DIR, ".bin/%{cfg.buildcfg}")
LIB_DIR = path.join(_MAIN_SCRIPT_DIR, ".lib/%{cfg.buildcfg}")
PROJECTS_DIR = path.join(_MAIN_SCRIPT_DIR, ".projects/")

function configuration_settings()
  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
  
  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
end

function system_settings()
  filter {"system:windows", "action:vs*"}
    systemversion("latest")
end

function static_lib_header()
  kind "StaticLib"
  language (LANG)
  targetdir (LIB_DIR)
  defines { "LIBRARY_EXPORTS" }
  configuration_settings()
  system_settings()
end

function shared_lib_header()
  kind "SharedLib"
  language (LANG)
  targetdir (BIN_DIR)
  defines { "LIBRARY_EXPORTS" }
  configuration_settings()
  system_settings()
end

function console_app_header(project_name)
  printf ("[" .. project_name .. "] generating...")
  location(path.join(PROJECTS_DIR, project_name))
  kind "ConsoleApp"
  language (LANG)
  targetdir (BIN_DIR)
  libdirs {LIB_DIR}
  configuration_settings()
  system_settings()
end

workspace "project"
  objdir ".obj/%{cfg.buildcfg}"
  
  configurations { "Debug", "Release" }

  architecture "x86_64"

include("0_tutorial_cubemap/0_tutorial_cubemap.lua")

printf ("Generation finished.")