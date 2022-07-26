#include <string>
#include <sstream>
#include <fbxsdk.h>
#include <exception>
#include <stdio.h>

//help doc: https://help.autodesk.com/cloudhelp/2018/ENU/FBX-Developer-Help/getting_started/installing_and_configuring/configuring_the_fbx_sdk_for_wind.html
//this plugin is setup for window dev environment so probably not supported on other platforms

#define VertexColorGenDebugFBXTest 0
#define EXPORT_API __declspec(dllexport)

extern "C"
{
	//Create a callback delegate to Unity's debug log
	typedef void(*DebugLogCallback)(const char* message);

	//Create a callback delegate to MeshList reimport
	typedef void(*OnCloneNodeFinish)(bool reimport);

	static DebugLogCallback callbackInstance = nullptr;

	//standard printf implementation to string/const char* so can pass back to unity debug log function
	void DebugLog(const char* format, ...)
	{
		char buffer[1024 * 10];

		va_list ap;
		va_start(ap, format);
		auto size = _vsnprintf_s(buffer, 1024 * 10, format, ap);
		va_end(ap);

		if (callbackInstance != nullptr)
		{
			callbackInstance(std::string(buffer).c_str());
		}
	}

	//function to set callback to unity's debug log - very useful
	EXPORT_API void RegisterDebugLogCallback(DebugLogCallback cb)
	{
		callbackInstance = cb;

		DebugLog("Attached Unity's debug log for FBXVertexColorGenTool");
	}
}

//unique pointer deleter for FBXObjects
struct FbxObjectDeleter
{
	void operator()(FbxObject* p)
	{
		if (p)
		{
			//DebugLog("Deleted: %s", p->GetName());
			p->Destroy();
		}
	}
};

//unique pointer deleter for FbxManager
struct FbxManagerDeleter
{
	void operator()(FbxManager* p)
	{
		if (p)
		{
			//DebugLog("Deleted fbx manager");
			p->Destroy();
		}
	}
};

bool Export(FbxManager* gSdkManager, FbxScene* fbxScene, const char* path/*, int fileFormat*/, bool doAscii = false)
{
	std::unique_ptr<FbxExporter, FbxObjectDeleter>exporter(FbxExporter::Create(gSdkManager, "FbxExporter"));

	int fileFormat = gSdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");

	bool exporterStatus = doAscii ? exporter->Initialize(path, fileFormat, gSdkManager->GetIOSettings()) : exporter->Initialize(path);
	
	if (!exporterStatus && !exporter->GetStatus())
	{
		DebugLog("failed to initialize FbxExporter, error returned %s", exporter->GetStatus().GetErrorString());
		return false;
	}
	
	// Export the scene to the file.
	exporterStatus = exporter->Export(fbxScene);
	if (!exporterStatus && !exporter->GetStatus())
	{
		DebugLog("failed to export from FbxExporter, error returned %s", exporter->GetStatus().GetErrorString());
		return false;
	}

	return true;
}

extern "C" EXPORT_API bool CloneNode(const char* filePath, const char* nodeToDuplicate, const char* newNodeName, OnCloneNodeFinish onCloneNodeFinishFunc)
{
    try
	{
		
		if (strcmp(nodeToDuplicate, newNodeName) == 0)
		{
			DebugLog("Please enter unique name: %s", newNodeName);
			return false;
		}

		std::unique_ptr<FbxManager, FbxManagerDeleter>		gSdkManager(FbxManager::Create());
		std::unique_ptr<FbxImporter, FbxObjectDeleter>		importer(FbxImporter::Create(gSdkManager.get(), "FbxImporter"));
		std::unique_ptr<FbxScene, FbxObjectDeleter>			fbxScene(FbxScene::Create(gSdkManager.get(), "UnityFbxScene"));

        // Initialize
        if (importer->Initialize(filePath) == false)
        {
            DebugLog("Couldn't read file %s.", filePath);

			if (!importer->GetStatus())
			{
				DebugLog("failed to initialize FbxImporter, error returned %s", importer->GetStatus().GetErrorString());
			}

            return false;
        }

		//DebugLog("Current File Format ID: %i", importer->GetFileFormat());
		//DebugLog("Ascii File Format ID: %i", gSdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)"));

        if (importer->Import(fbxScene.get()))
        {
            if (importer->IsFBX())
            {
				auto tempNode = fbxScene->FindNodeByName(newNodeName);

				if (tempNode)
				{
					DebugLog("Please enter unique name: %s", newNodeName);
					return false;
				}

				auto node = fbxScene->FindNodeByName(nodeToDuplicate);

				if (node)
				{
					//auto newNode = cloneManager.Clone(node, fbxScene);
					auto newNode = node->Clone(FbxObject::eDeepClone);
					auto newNodeMesh = node->GetMesh()->Clone(FbxObject::eDeepClone);

#if VertexColorGenDebugFBXTest
					//std::string destPathSrc = "AssetsTest/";
					std::string destPathSrc = "AssetsTest/";
					std::string destPathMod = destPathSrc;
					
					destPathSrc += nodeToDuplicate;
					destPathSrc += "Src";
					destPathSrc += ".fbx";

					destPathMod += nodeToDuplicate;
					destPathMod += "Mod";
					destPathMod += ".fbx";

					Export(gSdkManager.get(), fbxScene.get(), destPathSrc.c_str(), true);

					DebugLog("Node Name %s", node->GetName());

					//node->SetName("OriTestLod");
					//node->GetMesh()->SetName("OriTestLodMesh");

					std::string newName = nodeToDuplicate;
					newName += "Dup";

					///////////////////////////////////////////////////////

					for (int i = 0; i < node->GetDstObjectCount(); i++)
					{
						auto t = node->GetDstObject(i);
						DebugLog("GetDstObject %i : %s", i, t->GetName());
					}

					for (int i = 0; i < node->GetSrcObjectCount(); i++)
					{
						auto t = node->GetSrcObject(i);
						DebugLog("GetSrcObject %i : %s", i, t->GetName());
					}

					///////////////////////////////////////////////////////

					for (int i = 0; i < node->GetDstPropertyCount(); i++)
					{
						auto t = node->GetDstProperty(i);
						DebugLog("GetDstProperty %i : %s", i, t.GetName());
					}

					for (int i = 0; i < node->GetSrcPropertyCount(); i++)
					{
						auto t = node->GetSrcProperty(i);
						DebugLog("GetSrcProperty %i : %s", i, t.GetName());
					}

					///////////////////////////////////////////////////////

					for (int i = 0; i < node->GetReferencedByCount(); i++)
					{
						auto t = node->GetReferencedBy(i);
						DebugLog("GetReferencedBy %i : %s", i, t->GetName());
					}

					newName = newNodeName;
					newNode->SetName(newName.c_str());
					newName += "Mesh";
					newNodeMesh->SetName(newName.c_str());
#else
					newNode->SetName(newNodeName);
#endif
					newNode->ConnectSrcObject(newNodeMesh);

					//DstObject in this case refers to "parent node". A node can have multiple "parent" connected to it
					for (int i = 0; i < node->GetDstObjectCount(); i++)
					{
						//Get the Parent Node. A node can have multiple "parent" connected to it
						auto dstNode = node->GetDstObject(i);

						if (dstNode)
						{
							//make dstNode a parent of newNode
							dstNode->ConnectSrcObject(newNode);
						}
					}

					DebugLog("Mesh successfully duplicated. New Mesh is %s", newNode->GetName());

#if VertexColorGenDebugFBXTest
					Export(gSdkManager.get(), fbxScene.get(), destPathMod.c_str(), true);
#else
					Export(gSdkManager.get(), fbxScene.get(), filePath, false);
#endif
					onCloneNodeFinishFunc(true);

					return true;
				}
				else
				{
					DebugLog("Node %s not found", nodeToDuplicate);
					return false;
				}
            }
        }
        else
        {
            DebugLog("Couldn't import file %s.", filePath);

			if (!importer->GetStatus())
			{
				DebugLog("%s", importer->GetStatus().GetErrorString());
			}

			return false;
        }

    }
    catch (...)
    {
        DebugLog("An error occurred while importing file %s. Try reimporting.", filePath);
		return false;
    }

	return false;
}