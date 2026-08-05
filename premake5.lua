workspace "BigBaseV2"
  architecture "x64"
  startproject "BigBaseV2"

  configurations
  {
    "Debug",
    "Release",
    "Dist"
  }

  outputdir = "%{cfg.buildcfg}"

  IncludeDir = {}
  IncludeDir["fmtlib"] = "vendor/fmtlib/include"
  IncludeDir["json"] = "vendor/json/single_include"
  IncludeDir["MinHook"] = "vendor/MinHook/include"
  IncludeDir["ImGui"] = "vendor/ImGui"
  IncludeDir["ImGuiImpl"] = "vendor/ImGui/examples"
  IncludeDir["StackWalker"] = "vendor/StackWalker/Main/StackWalker"
  IncludeDir["Lua"] = "vendor/lua/src"
  IncludeDir["Sol2"] = "vendor/sol2/include"

  CppVersion = "C++17"
  MsvcToolset = "v143"
  WindowsSdkVersion = "latest"

  VehicleCatalogPath = "BigBaseV2/src/menu/pages/vehicle_catalog_generated.hpp"
  if not os.isfile(VehicleCatalogPath) then
    error("Missing generated vehicle catalog. Run: py scripts/generate_vehicle_catalog.py")
  end

  function DeclareMSVCOptions()
    filter "system:windows"
    staticruntime "Off"
    systemversion (WindowsSdkVersion)
    toolset (MsvcToolset)
    cppdialect (CppVersion)

    defines
    {
      "_CRT_SECURE_NO_WARNINGS",
      "_HAS_DEPRECATED_RESULT_OF=1",
      "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
      "NOMINMAX",
      "WIN32_LEAN_AND_MEAN",
      "_WIN32_WINNT=0x0A00"
    }

    disablewarnings
    {
      "4100",
      "4201",
      "4307"
    }
  end

  function DeclareDebugOptions()
    filter "configurations:Debug"
      defines { "_DEBUG" }
      symbols "On"
    filter "not configurations:Debug"
      defines { "NDEBUG" }
  end

  project "ImGui"
    location "vendor/%{prj.name}"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/lib/" .. outputdir)
    objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")

    files
    {
      "vendor/%{prj.name}/imgui.cpp",
      "vendor/%{prj.name}/imgui_demo.cpp",
      "vendor/%{prj.name}/imgui_draw.cpp",
      "vendor/%{prj.name}/imgui_widgets.cpp",
      "vendor/%{prj.name}/examples/imgui_impl_dx12.cpp",
      "vendor/%{prj.name}/examples/imgui_impl_win32.cpp"
    }

    includedirs
    {
      "vendor/%{prj.name}",
      "vendor/%{prj.name}/examples"
    }
    DeclareMSVCOptions()
    DeclareDebugOptions()

  project "fmtlib"
    location "vendor/%{prj.name}"
    kind "StaticLib"
    language "C++"
    targetdir ("bin/lib/" .. outputdir)
    objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")
    files { "vendor/%{prj.name}/include/**.h", "vendor/%{prj.name}/src/**.cc" }
    includedirs { "vendor/%{prj.name}/include" }
    DeclareMSVCOptions()
    DeclareDebugOptions()

  project "StackWalker"
    location "vendor/%{prj.name}"
    kind "StaticLib"
    language "C++"
    targetdir ("bin/lib/" .. outputdir)
    objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")
    files { "vendor/%{prj.name}/Main/StackWalker/StackWalker.cpp" }
    includedirs { "%{IncludeDir.StackWalker}" }
    DeclareMSVCOptions()
    DeclareDebugOptions()

  project "MinHook"
    location "vendor/%{prj.name}"
    kind "StaticLib"
    language "C"
    targetdir ("bin/lib/" .. outputdir)
    objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")
    files { "vendor/%{prj.name}/include/**.h", "vendor/%{prj.name}/src/**.h", "vendor/%{prj.name}/src/**.c" }
    includedirs { "%{IncludeDir.MinHook}" }
    DeclareMSVCOptions()
    DeclareDebugOptions()

  project "Lua"
    location "vendor/%{prj.name}"
    kind "StaticLib"
    language "C"
    targetdir ("bin/lib/" .. outputdir)
    objdir ("bin/lib/int/" .. outputdir .. "/%{prj.name}")
    files { "vendor/lua/src/**.h", "vendor/lua/src/**.c" }
    removefiles { "vendor/lua/src/lua.c", "vendor/lua/src/luac.c", "vendor/lua/src/onelua.c" }
    includedirs { "%{IncludeDir.Lua}" }
    defines { "LUA_COMPAT_5_3", "LUA_USE_WINDOWS" }
    DeclareMSVCOptions()
    DeclareDebugOptions()

  project "BigBaseV2"
    location "BigBaseV2"
    kind "SharedLib"
    language "C++"
    targetdir ("bin/" .. outputdir)
    objdir ("bin/int/" .. outputdir .. "/%{prj.name}")

    PrecompiledHeaderInclude = "common.hpp"
    PrecompiledHeaderSource = "%{prj.name}/src/common.cpp"

    files { "%{prj.name}/src/**.hpp", "%{prj.name}/src/**.cpp", "%{prj.name}/src/**.asm" }

    includedirs
    {
      "%{IncludeDir.fmtlib}", "%{IncludeDir.json}", "%{IncludeDir.MinHook}",
      "%{IncludeDir.ImGui}", "%{IncludeDir.ImGuiImpl}", "%{IncludeDir.StackWalker}",
      "%{IncludeDir.Lua}", "%{IncludeDir.Sol2}", "%{prj.name}/src"
    }

    links
    {
      "fmtlib",
      "MinHook",
      "ImGui",
      "StackWalker",
      "Lua",
      "dbghelp",
      "version",
      "windowscodecs",
      "ole32",
      "d3d12",
      "dxgi",
      "dxguid",
      "d3dcompiler"
    }

    pchheader "%{PrecompiledHeaderInclude}"
    pchsource "%{PrecompiledHeaderSource}"
    forceincludes { "%{PrecompiledHeaderInclude}" }

    DeclareMSVCOptions()
    DeclareDebugOptions()
    flags { "NoImportLib", "Maps" }

    filter "configurations:Debug"
      defines { "BIGBASEV2_DEBUG" }
    filter "configurations:Release"
      defines { "BIGBASEV2_RELEASE" }
      optimize "speed"
    filter "configurations:Dist"
      flags { "LinkTimeOptimization", "FatalCompileWarnings" }
      defines { "BIGBASEV2_DIST" }
      optimize "speed"
