solution "FbxToolsExperiments"

platforms{"x64"}
configurations{"Release"}
flags{"Symbols"}
flags{"Exceptions"}
configuration "Release"
	defines{"FBXSDK_SHARED"}
	flags{"Optimize"}
configuration {}

location "build"

project "FBXVertexColorGenTools"
	includedirs{"Dependency/FBXSDK/2019.5"}
	language "C++"
	kind "SharedLib"
	includedirs{"."}
	files{"*.cpp", "*.h", "premake4.lua"}
	
	configuration{"x64"}
		includedirs{"Dependency/FBXSDK/2019.5/include"}
		libdirs{"Dependency/FBXSDK/2019.5/lib/vs2017/x64/release"}
		links{"libfbxsdk","libxml2-md","zlib-md"}
	configuration {"x64","Release"}
		targetdir("build/output/x64/Release")

