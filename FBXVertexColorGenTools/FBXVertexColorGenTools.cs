//inspiration: https://answers.unity.com/questions/1127916/reloading-native-plugins.html

//VertexColorGenLoadPluginForTesting is used for plugin rapid prototyping which implements manual load, unload and func pointer.
//#define VertexColorGenLoadPluginForTesting

using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Security;
using AOT;
using System.Collections.Generic;
using System.Reflection;

//Probably can put on separate file so other plugins can use it. Currently window only 
class FunctionLoader
{
    public static Delegate LoadFunction<T>(string dllPath, string functionName)
    {
        var hModule = DLLLibraryLoader.LoadLibrary(dllPath);
        handles.Add(hModule);
        var functionAddress = DLLLibraryLoader.GetProcAddress(hModule, functionName);
        return Marshal.GetDelegateForFunctionPointer(functionAddress, typeof(T));
    }

    // tracking what we loaded for easier cleanup
    static List<IntPtr> handles = new List<IntPtr>();

    public static void FreeLibraries()
    {
        foreach (var ptr in handles)
        {
            Debug.Log("Cleaning up module " + ptr);
            // todo: check if ptr was -1 or something bad like that...
            DLLLibraryLoader.FreeLibrary(ptr);
        }
    }

    ////////////////////////////////////////////////////////////

}

//Probably can put on separate file so other plugins can use it. Currently window only 
class DLLLibraryLoader
{
    [DllImport("kernel32", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr LoadLibrary(string lpFileName);

    [DllImport("kernel32")]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);
}

public static class FBXVertexColorGenTools
{
    // The callback signature (shared by C++). string is marshalled as const char* automatically
    public delegate void DebugLogCallback(string message);

    public delegate void OnCloneNodeFinish(bool reimport);

#if UNITY_EDITOR

#if VertexColorGenLoadPluginForTesting

	//Needed to pass in the path for dynamic DLL loading in Unity
    static string dllPath = "Plugins/FBXVertexColorGenTools/build/output/x64/Release/FBXVertexColorGenTools.dll";
    
    static string projectPath
    {
        get
        {
            var path = Application.dataPath;
            path = path.Remove(path.Length - 6, 6);
            return path;
        }
            
    }

    static string loadPath
    {
        get
        {
            return projectPath + dllPath;
        }

    }

    [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
    private delegate void RegisterDebugLogCallback(DebugLogCallback cb);
    static RegisterDebugLogCallback RegisterDebugLogCallbackFunc;

    [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
    public delegate bool CloneNodeImpl(string filePath, string nodeToDuplicate, string newNodeName, OnCloneNodeFinish onCloneNodeFinishFunc);
    static CloneNodeImpl CloneNodeImplFunc;

#else

    //CallingConvention = CallingConvention.Cdecl is used for function that cleans its own stack - so suitable for debug log / printf callback?
    [DllImport("FBXVertexColorGenTools", EntryPoint = "RegisterDebugLogCallback", CallingConvention = CallingConvention.Cdecl)]
    static extern void RegisterDebugLogCallbackFunc(DebugLogCallback cb);

    [DllImport("FBXVertexColorGenTools", EntryPoint = "CloneNode")]
    public static extern bool CloneNodeImplFunc(string filePath, string nodeToDuplicate, string newNodeName, OnCloneNodeFinish onCloneNodeFinishFunc);

#endif

    public static bool CloneNode(string filePath, string nodeToDuplicate, string newNodeName, OnCloneNodeFinish onCloneNodeFinishFunc)
    {
        ClearLog();

#if VertexColorGenLoadPluginForTesting
        
        RegisterDebugLogCallbackFunc = (RegisterDebugLogCallback)FunctionLoader.LoadFunction<RegisterDebugLogCallback>(loadPath, "RegisterDebugLogCallback");

        CloneNodeImplFunc = (CloneNodeImpl)FunctionLoader.LoadFunction<CloneNodeImpl>(loadPath, "CloneNode");

        Debug.Log("Loaded functions from " + loadPath);

#endif

        RegisterDebugLogCallbackFunc(DebugLog);

        var result = CloneNodeImplFunc(filePath, nodeToDuplicate, newNodeName, onCloneNodeFinishFunc);

#if VertexColorGenLoadPluginForTesting
        FunctionLoader.FreeLibraries();
#endif
        return result;
    }
    
    public static void ClearLog()
    {
        var assembly = Assembly.GetAssembly(typeof(UnityEditor.Editor));
        var type = assembly.GetType("UnityEditor.LogEntries");
        var method = type.GetMethod("Clear");
        method.Invoke(new object(), null);
    }

    [MonoPInvokeCallback(typeof(DebugLogCallback))]
    public static void DebugLog(string message)
    {
        Debug.Log(message);
    }

#endif
}
