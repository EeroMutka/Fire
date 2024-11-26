
function specify_warnings()
	flags "FatalWarnings" -- treat all warnings as errors
	buildoptions "/w14062" -- error on unhandled enum members in switch cases
	buildoptions "/w14456" -- error on shadowed locals
	buildoptions "/wd4101" -- allow unused locals
	linkoptions "-IGNORE:4099" -- disable linker warning: "PDB was not found ...; linking object as if no debug info"
end

workspace "examples"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "build_projects"

project "example_dx11"
	kind "ConsoleApp"
	language "C++"
	targetdir "build"
	specify_warnings()
	
	includedirs "../"
	
	defines "UI_DEMO_DX11"
	files "source/**"
	
	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

project "example_dx12"
	kind "ConsoleApp"
	language "C++"
	targetdir "build"
	specify_warnings()
	
	includedirs "../"
	
	defines "UI_DEMO_DX12"
	files "source/**"
	
	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
