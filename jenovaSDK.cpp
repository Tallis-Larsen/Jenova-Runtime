
/*-------------------------------------------------------------+
|                                                              |
|                   _________   ______ _    _____              |
|                  / / ____/ | / / __ \ |  / /   |             |
|             __  / / __/ /  |/ / / / / | / / /| |             |
|            / /_/ / /___/ /|  / /_/ /| |/ / ___ |             |
|            \____/_____/_/ |_/\____/ |___/_/  |_|             |
|                                                              |
|							Jenova SDK                         |
|                   Developed by Hamid.Memar                   |
|                                                              |
+-------------------------------------------------------------*/

// Windows SDK
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

// C++ SDK
#include <stdarg.h>
#include <iostream>
#include <functional>

// Godot SDK
#include <gdextension_interface.h>
#include <godot.hpp>
#include <core/defs.hpp>
#include <core/class_db.hpp>
#include <core/object.hpp>
#include <classes/os.hpp>
#include <classes/time.hpp>
#include <classes/engine.hpp>
#include <classes/editor_interface.hpp>
#include <classes/editor_selection.hpp>
#include <classes/theme.hpp>
#include <classes/window.hpp>
#include <classes/scene_tree.hpp>
#include <classes/resource_saver.hpp>
#include <classes/resource_loader.hpp>
#include <classes/texture2d.hpp>
#include <templates/vector.hpp>
#include <variant/string.hpp>
#include <variant/node_path.hpp>
#include <variant/array.hpp>
#include <variant/utility_functions.hpp>

// GodotSDK/LithiumSDK
#ifdef LITHIUM_EDITION
#include <classes/component.hpp>
#include <classes/element.hpp>
#include <Misc/lithium-compatibility.hpp>
#else
#include <classes/packed_scene.hpp>
#endif

// Jenova System SDK
#include "JenovaSDK.h"

// Namespaces
using namespace std;

// Internal Structs
struct NodeBackup
{
	godot::String nodeName;
	godot::String nodeClass;
	godot::String scenePath;
	godot::PackedScene* nodeBackup = nullptr;
	godot::Node* dummyNode = nullptr;
	godot::Node* sceneRoot = nullptr;
};

// Internal Classes
class EventCallback : public godot::RefCounted
{
private:
	void* callback;

public:
	void OnEventCall()
	{
		if (!this->callback) return;
		typedef void(*callbackFunc)(void);
		callbackFunc callbackFunction = (callbackFunc)this->callback;
		if (!callbackFunction) return;
		callbackFunction();
		memdelete(this);
	}
	EventCallback(void* callbackPtr) : callback(callbackPtr) {}
};

// Storages
godot::Vector<NodeBackup> nodeBackups;
std::unordered_map<std::string, void*> globalMemoryMap;
std::unordered_map<std::string, godot::Variant> globalVariables;

// Internal Helpers
static void CollectNodesByClassName(godot::Node* node, const godot::String& class_name, godot::Vector<godot::Node*>& result)
{
	if (node->is_class(class_name))
	{
		result.push_back(node);
	}
	for (int i = 0; i < node->get_child_count(); ++i) 
	{
		CollectNodesByClassName(node->get_child(i), class_name, result);
	}
}

// System SDK Implementation
namespace jenova::sdk
{
	// Helpers Utilities
	bool JenovaSDK::IsEditor()
	{
		return godot::Engine::get_singleton()->is_editor_hint();
	}
	bool JenovaSDK::IsGame()
	{
		if (IsEditor()) return false;
		if (godot::OS::get_singleton()->is_debug_build()) return false;
		return true;
	}
	godot::Node* JenovaSDK::GetNodeByPath(const godot::String& path)
	{
		godot::SceneTree* scene_tree = dynamic_cast<godot::SceneTree*>(godot::Engine::get_singleton()->get_main_loop());
		if (scene_tree) return scene_tree->get_root()->get_node<godot::Node>(godot::NodePath(path));
		return nullptr;
	}
	godot::Node* JenovaSDK::FindNodeByName(godot::Node* parent, const godot::String& name)
	{
		if (!parent) return nullptr;
		if (parent->get_name() == name) return parent;
		for (int i = 0; i < parent->get_child_count(); ++i)
		{
			godot::Node* child = parent->get_child(i);
			godot::Node* result = FindNodeByName(child, name);
			if (result) return result;
		}
		return nullptr;
	}
	StringPtr JenovaSDK::GetNodeUniqueID(godot::Node* node)
	{
		return GetCStr(godot::String(node->get_path()).md5_text());
	}
	godot::SceneTree* JenovaSDK::GetTree()
	{
		godot::SceneTree* scene_tree = dynamic_cast<godot::SceneTree*>(godot::Engine::get_singleton()->get_main_loop());
		return scene_tree;
	}
	double JenovaSDK::GetTime()
	{
		int64_t time_msec = godot::Time::get_singleton()->get_ticks_msec();
		return static_cast<double>(time_msec) / 1000.0f;
	}
	godot::String JenovaSDK::Format(StringPtr format, va_list args)
	{
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), format, args);
		return godot::String(buffer);
	}
	godot::String JenovaSDK::Format(WideStringPtr format, va_list args)
	{
		wchar_t buffer[1024];
		vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
		return godot::String(buffer);
	}
	void JenovaSDK::Output(StringPtr format, va_list args)
	{
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), format, args);
		godot::UtilityFunctions::print(godot::String("[JENOVA-SDK] > ") + godot::String(buffer));
	}
	void JenovaSDK::Output(WideStringPtr format, va_list args)
	{
		wchar_t buffer[1024];
		vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
		godot::UtilityFunctions::print(godot::String(L"[JENOVA-SDK] > ") + godot::String(buffer));
	}
	void JenovaSDK::DebugOutput(StringPtr format, va_list args)
	{
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), format, args);
		std::string debugMessage = "[JENOVA-SDK] ::> ";
		debugMessage += buffer;

		// Debug Print
		#if defined(_WIN32) || defined(_WIN64)
			OutputDebugStringA(debugMessage.c_str());
		#else
			std::clog << debugMessage << std::endl;
		#endif
	}
	void JenovaSDK::DebugOutput(WideStringPtr format, va_list args)
	{
		wchar_t buffer[1024];
		vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
		std::wstring debugMessage = L"[JENOVA-SDK] ::> ";
		debugMessage += buffer;

		// Debug Print
		#if defined(_WIN32) || defined(_WIN64)
			OutputDebugStringW(debugMessage.c_str());
		#else
			std::wclog << debugMessage << std::endl;
		#endif
	}
	StringPtr JenovaSDK::GetCStr(const godot::String& godotStr)
	{
		std::string str((char*)godotStr.utf8().ptr(), godotStr.utf8().size());
		if (!str.empty() && str.back() == '\0') str.pop_back();

		// Bad Approach, Needs Improvement
		#if defined(_WIN32) || defined(_WIN64)
			return _strdup(str.c_str());
		#else
			return strdup(str.c_str());
		#endif
	}
	WideStringPtr JenovaSDK::GetWCStr(const godot::String& godotStr)
	{
		godot::PackedByteArray wchar_buffer = godotStr.to_wchar_buffer();
		size_t length = wchar_buffer.size() / sizeof(wchar_t);
		std::wstring str((wchar_t*)wchar_buffer.ptr(), length);
		if (!str.empty() && str.back() == L'\0') str.pop_back();

		// Bad Approach, Needs Improvement
		#if defined(_WIN32) || defined(_WIN64)
				return _wcsdup(str.c_str());
		#else
				return wcsdup(str.c_str());
		#endif
	}
	bool JenovaSDK::SetClassIcon(const godot::String& className, const godot::Ref<godot::Texture2D> iconImage)
	{
		if (!godot::ClassDB::class_exists(className)) return false;
		godot::Ref<godot::Theme> editor_theme = godot::EditorInterface::get_singleton()->get_editor_theme();
		if (editor_theme->has_icon(className, "EditorIcons")) return false;
		editor_theme->set_icon(className, "EditorIcons", iconImage);
		return true;
	}
	double JenovaSDK::MatchScaleFactor(double inputSize)
	{
		if (IsEditor())
		{
			double scaleFactor = godot::EditorInterface::get_singleton()->get_editor_scale();
			return inputSize * scaleFactor;
		}
		else
		{
			return inputSize;
		}
	}
	godot::Error JenovaSDK::CreateSignalCallback(godot::Object* object, const godot::String& signalName, FunctionPtr callbackPtr)
	{
		return object->connect(signalName, callable_mp(memnew(EventCallback(callbackPtr)), &EventCallback::OnEventCall));
	}

	// Hot-Reloading Utilities (Sakura)
	bool JenovaSDK::SupportsReload()
	{
		if (IsEditor()) return godot::EditorInterface::get_singleton()->has_method("get_open_scenes_roots");
		return true;
	}
	void JenovaSDK::PrepareReload(const godot::String& className)
	{
		// Disable Hot-Reloading In Static SDK
		#ifdef JENOVA_SDK_STATIC
			return;
		#endif

		// Validate
		if (!godot::ClassDB::class_exists(className)) return;
	
		// Validate Scene Tree [Required!]
		if (!GetTree()) return;

		// Deselect Nodes
		if (IsEditor()) godot::EditorInterface::get_singleton()->get_selection()->clear();

		// Get Opened Scenes
		godot::Array openedScenes;
		if (IsEditor()) openedScenes = godot::EditorInterface::get_singleton()->get_open_scenes_roots();
		else openedScenes.push_back(GetTree()->get_root());
		for (size_t i = 0; i < openedScenes.size(); i++)
		{
			// Get Scene Root
			godot::Node* sceneRoot = (godot::Node*)openedScenes[i]._native_ptr();

			// Validate Scene Root
			if (!sceneRoot) continue;

			// Collect Nodes With Class Name
			godot::Vector<godot::Node*> classNodes;
			CollectNodesByClassName(sceneRoot, className, classNodes);

			// Backup Nodes
			for (const auto& classNode : classNodes)
			{
				// Create Node Backup
				NodeBackup nodebackup;
				nodebackup.nodeName = classNode->get_name();
				nodebackup.nodeClass = godot::String(className);
				nodebackup.sceneRoot = sceneRoot;
				nodebackup.scenePath = sceneRoot->get_scene_file_path();

				// Duplicate Node to Backup [Due to Issue #81982]
				godot::Node* classNodeClone = classNode->duplicate();

				// Pack Current Scene
				nodebackup.nodeBackup = memnew(godot::PackedScene);
				nodebackup.nodeBackup->pack(classNodeClone);
				memdelete(classNodeClone); // classNodeClone->queue_free();

				// Replace With Dummy Node
				nodebackup.dummyNode = memnew(godot::Node);
				classNode->replace_by(nodebackup.dummyNode, true);
				memdelete(classNode); // classNode->queue_free();

				// Add to Dummy Nodes
				nodeBackups.push_back(nodebackup);
			}
		}
	}
	void JenovaSDK::FinishReload(const godot::String& className)
	{
		// Disable Hot-Reloading In Static SDK
		#ifdef JENOVA_SDK_STATIC
			return;
		#endif

		// Validate
		if (!godot::ClassDB::class_exists(className)) return;

		// Create Class Name
		godot::String backupClassName(className);

		// Restore Nodes
		for (int i = 0; i < nodeBackups.size(); ++i)
		{
			// Get Node Backup
			NodeBackup nodeBackup = nodeBackups[i];

			// Validate Node Class
			if (nodeBackup.nodeClass != backupClassName) return;

			// Validate Backup Data
			if (!nodeBackup.sceneRoot) return;

			// Check for Feature
			if (!IsEditor())
			{
				godot::Dictionary versionInfo = godot::Engine::get_singleton()->get_version_info();
				if (godot::String(versionInfo["build"]) != "jenova")
				{
					godot::UtilityFunctions::push_error("[Jenova::Sakura] Runtime Hot-Reloading for GDExtension Classes Only is Supported in Jenova Editions of Godot.");
					auto* placeholderNode = memnew(godot::Node);
					nodeBackup.dummyNode->replace_by(placeholderNode, true);
					memdelete(nodeBackup.dummyNode);
					memdelete(nodeBackup.nodeBackup);
					nodeBackups.remove_at(i);
					--i;
					continue;
				}
			}

			// Restore Nodes From Backup
			godot::Node* originalNode = nodeBackup.nodeBackup->instantiate(godot::PackedScene::GenEditState::GEN_EDIT_STATE_DISABLED);
			if (originalNode)
			{
				nodeBackup.dummyNode->replace_by(originalNode, true);
				memdelete(nodeBackup.dummyNode);
				memdelete(nodeBackup.nodeBackup);
				nodeBackups.remove_at(i);
				--i;
			}
		}
	}
	void JenovaSDK::Dispose(const godot::String& className)
	{
		godot::StringName classNameStr(className);
		if (!godot::ClassDB::class_exists(classNameStr)) return;
		godot::internal::gdextension_interface_classdb_unregister_extension_class(godot::internal::library, classNameStr._native_ptr());
	}
}

// Jenova Runtime SDK
#include "Jenova.hpp"

// Runtime SDK Implementation
namespace jenova::sdk
{
	// Helpers Utilities
	void JenovaSDK::Alert(StringPtr fmt, va_list args)
	{
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		ShowMessageBox(buffer, "[JENOVA-SDK]", 0);
	}
	jenova::sdk::EngineMode JenovaSDK::GetEngineMode()
	{
		return jenova::sdk::EngineMode(jenova::GlobalStorage::CurrentEngineMode);
		jenova::sdk::EngineMode engineMode;
		if (OS::get_singleton()->is_debug_build()) engineMode = jenova::sdk::EngineMode::Debug;
		else engineMode = jenova::sdk::EngineMode::Runtime;
		if (Engine::get_singleton()->is_editor_hint()) engineMode = jenova::sdk::EngineMode::Editor;
		return engineMode;
	}
	bool JenovaSDK::CreateDirectoryMonitor(const String& directoryPath)
	{
		if (!JenovaAssetMonitor::get_singleton()) return false;
		if (!JenovaAssetMonitor::get_singleton()->AddDirectory(directoryPath)) return false;
		return true;
	}
	bool JenovaSDK::CreateFileMonitor(const String& filePath)
	{
		if (!JenovaAssetMonitor::get_singleton()) return false;
		if (!JenovaAssetMonitor::get_singleton()->AddFile(filePath)) return false;
		return true;
	}
	bool JenovaSDK::RegisterFileMonitorCallback(FileSystemCallback callbackPtr)
	{
		if (!JenovaAssetMonitor::get_singleton()) return false;
		if (!JenovaAssetMonitor::get_singleton()->RegisterCallback(jenova::AssetMonitor::AssetMonitorCallback(callbackPtr))) return false;
		return true;
	}
	bool JenovaSDK::UnregisterFileMonitorCallback(FileSystemCallback callbackPtr)
	{
		if (!JenovaAssetMonitor::get_singleton()) return false;
		if (!JenovaAssetMonitor::get_singleton()->UnregisterCallback(jenova::AssetMonitor::AssetMonitorCallback(callbackPtr))) return false;
		return true;
	}
	bool JenovaSDK::ReloadJenovaRuntime(RuntimeReloadMode reloadMode)
	{
		jenova::sdk::Output("ReloadJenovaRuntime -> Not Implemented Yet");
		return false;
	}
	void JenovaSDK::CreateCheckpoint(const godot::String& checkPointName)
	{
		JenovaTinyProfiler::CreateCheckpoint(AS_STD_STRING(checkPointName));
	}
	double JenovaSDK::GetCheckpointTime(const godot::String& checkPointName)
	{
		return JenovaTinyProfiler::GetCheckpointTime(AS_STD_STRING(checkPointName));
	}
	void JenovaSDK::DeleteCheckpoint(const godot::String& checkPointName)
	{
		JenovaTinyProfiler::DeleteCheckpoint(AS_STD_STRING(checkPointName));
	}
	double JenovaSDK::GetCheckpointTimeAndDispose(const godot::String& checkPointName)
	{
		return JenovaTinyProfiler::GetCheckpointTimeAndDispose(AS_STD_STRING(checkPointName));
	}
	bool JenovaSDK::RegisterRuntimeCallback(RuntimeCallback callbackPtr)
	{
		return jenova::RegisterRuntimeEventCallback((jenova::FunctionPointer)callbackPtr);
	}
	bool JenovaSDK::UnregisterRuntimeCallback(RuntimeCallback callbackPtr)
	{
		return jenova::UnregisterRuntimeEventCallback((jenova::FunctionPointer)callbackPtr);
	}

	// Graphic Utilities
	NativePtr JenovaSDK::GetGameWindowHandle()
	{
		return NativePtr(jenova::GetMainWindowNativeHandle());
	}
	StringPtr JenovaSDK::GetRenderingDriverName()
	{
		auto projectSetting = ProjectSettings::get_singleton();
		#if defined(_WIN32) || defined(_WIN64)
			return GetCStr(String(projectSetting->get_setting("rendering/rendering_device/driver.windows")));
		#else
			return GetCStr(String(projectSetting->get_setting("rendering/rendering_device/driver")));
		#endif
	}
	NativePtr JenovaSDK::GetRenderingDriverResource(DriverResourceID resourceType)
	{
		godot::RenderingDevice* device = godot::RenderingServer::get_singleton()->get_rendering_device();
		if (device) return reinterpret_cast<NativePtr>(device->get_driver_resource(godot::RenderingDevice::DriverResource(resourceType), RID(), 0));
		return nullptr;
	}

	// Memory Management Utilities (Anzen)
	NativePtr JenovaSDK::GetGlobalPointer(MemoryID id)
	{
		auto it = globalMemoryMap.find(id);
		if (it != globalMemoryMap.end()) return it->second;
		return nullptr;
	}
	NativePtr JenovaSDK::SetGlobalPointer(MemoryID id, NativePtr ptr)
	{
		auto it = globalMemoryMap.find(id);
		if (it != globalMemoryMap.end())
		{
			NativePtr oldPtr = it->second;
			it->second = ptr;
			return oldPtr;
		}
		else {
			globalMemoryMap[id] = ptr;
			return ptr;
		}
	}
	void JenovaSDK::DeleteGlobalPointer(MemoryID id)
	{
		globalMemoryMap.erase(id);
	}
	NativePtr JenovaSDK::AllocateGlobalMemory(MemoryID id, size_t size)
	{
		NativePtr mem = jenova::AllocateMemory(size);
		if (!mem) return nullptr;
		globalMemoryMap[id] = mem;
		return mem;
	}
	void JenovaSDK::FreeGlobalMemory(MemoryID id)
	{
		auto it = globalMemoryMap.find(id);
		if (it != globalMemoryMap.end())
		{
			jenova::FreeMemory(it->second);
			globalMemoryMap.erase(it);
		}
	}

	// Global Variable Storage Utilities
	godot::Variant JenovaSDK::GetGlobalVariable(VariableID id)
	{
		return globalVariables[id];
	}
	void JenovaSDK::SetGlobalVariable(VariableID id, godot::Variant var)
	{
		globalVariables[id] = var;
	}
	void JenovaSDK::ClearGlobalVariables()
	{
		globalVariables.clear();
	}

	// Task System Utilities
	TaskID JenovaSDK::InitiateTask(TaskFunction function)
	{
		return JenovaTaskSystem::InitiateTask(function);
	}
	bool JenovaSDK::IsTaskComplete(TaskID taskID)
	{
		return JenovaTaskSystem::IsTaskComplete(taskID);
	}
	void JenovaSDK::ClearTask(TaskID taskID)
	{
		JenovaTaskSystem::ClearTask(taskID);
	}
}

// SDK Management
namespace jenova
{
	// JenovaSDK Interface Singleton
	namespace sdk { JenovaSDK* bridge = nullptr; }

	// Create/Release SDK Interface
	JenovaSDKInterface CreateJenovaSDKInterface()
	{
		JenovaSDKInterface sdkInterface = new sdk::JenovaSDK();
		sdk::bridge = (sdk::JenovaSDK*)sdkInterface;
		return sdkInterface;
	}
	bool ReleaseJenovaSDKInterface(JenovaSDKInterface sdkInterface)
	{
		if (!sdkInterface) return false;
		delete sdkInterface;
		sdk::bridge = nullptr;
		return true;
	}

	// Exported API
	JenovaSDKInterface sdk::GetSDKInterface()
	{
		return bridge;
	}
	sdk::FunctionPtr sdk::GetSDKFunction(StringPtr sdkFunctionName)
	{
		// Validate Bridge
		if (bridge == nullptr) return nullptr;

		// Solve Helpers Utilities Functions
		if (std::string(sdkFunctionName) == "IsEditor") return &sdk::IsEditor;
		if (std::string(sdkFunctionName) == "IsGame") return &sdk::IsGame;
		if (std::string(sdkFunctionName) == "GetEngineMode") return &sdk::GetEngineMode;
		if (std::string(sdkFunctionName) == "GetNodeByPath") return &sdk::GetNodeByPath;
		if (std::string(sdkFunctionName) == "FindNodeByName") return &sdk::FindNodeByName;
		if (std::string(sdkFunctionName) == "GetNodeUniqueID") return &sdk::GetNodeUniqueID;
		if (std::string(sdkFunctionName) == "GetTree") return &sdk::GetTree;
		if (std::string(sdkFunctionName) == "GetTime") return &sdk::GetTime;
		if (std::string(sdkFunctionName) == "Alert") return &sdk::Alert;
		if (std::string(sdkFunctionName) == "FormatA") return (godot::String(*)(StringPtr, ...))(sdk::Format);
		if (std::string(sdkFunctionName) == "FormatW") return (godot::String(*)(WideStringPtr, ...))(sdk::Format);
		if (std::string(sdkFunctionName) == "OutputA") return (void(*)(StringPtr, ...))(sdk::Output);
		if (std::string(sdkFunctionName) == "OutputW") return (void(*)(WideStringPtr, ...))(sdk::Output);
		if (std::string(sdkFunctionName) == "DebugOutputA") return (void(*)(StringPtr, ...))(sdk::DebugOutput);
		if (std::string(sdkFunctionName) == "DebugOutputW") return (void(*)(WideStringPtr, ...))(sdk::DebugOutput);
		if (std::string(sdkFunctionName) == "GetCStr") return &sdk::GetCStr;
		if (std::string(sdkFunctionName) == "GetWCStr") return &sdk::GetWCStr;
		if (std::string(sdkFunctionName) == "SetClassIcon") return &sdk::SetClassIcon;
		if (std::string(sdkFunctionName) == "MatchScaleFactor") return &sdk::MatchScaleFactor;
		if (std::string(sdkFunctionName) == "CreateSignalCallback") return &sdk::CreateSignalCallback;
		if (std::string(sdkFunctionName) == "CreateDirectoryMonitor") return &sdk::CreateDirectoryMonitor;
		if (std::string(sdkFunctionName) == "CreateFileMonitor") return &sdk::CreateFileMonitor;
		if (std::string(sdkFunctionName) == "RegisterFileMonitorCallback") return &sdk::RegisterFileMonitorCallback;
		if (std::string(sdkFunctionName) == "UnregisterFileMonitorCallback") return &sdk::UnregisterFileMonitorCallback;
		if (std::string(sdkFunctionName) == "ReloadJenovaRuntime") return &sdk::ReloadJenovaRuntime;
		if (std::string(sdkFunctionName) == "CreateCheckpoint") return &sdk::CreateCheckpoint;
		if (std::string(sdkFunctionName) == "GetCheckpointTime") return &sdk::GetCheckpointTime;
		if (std::string(sdkFunctionName) == "DeleteCheckpoint") return &sdk::DeleteCheckpoint;
		if (std::string(sdkFunctionName) == "GetCheckpointTimeAndDispose") return &sdk::GetCheckpointTimeAndDispose;
		if (std::string(sdkFunctionName) == "RegisterRuntimeCallback") return &sdk::RegisterRuntimeCallback;
		if (std::string(sdkFunctionName) == "UnregisterRuntimeCallback") return &sdk::UnregisterRuntimeCallback;

		// Solve Graphic Utilities Functions
		if (std::string(sdkFunctionName) == "GetGameWindowHandle") return &sdk::GetGameWindowHandle;
		if (std::string(sdkFunctionName) == "GetRenderingDriverName") return &sdk::GetRenderingDriverName;
		if (std::string(sdkFunctionName) == "GetRenderingDriverResource") return &sdk::GetRenderingDriverResource;

		// Solve Hot-Reloading Utilities (Sakura) Functions
		if (std::string(sdkFunctionName) == "SupportsReload") return &sdk::sakura::SupportsReload;
		if (std::string(sdkFunctionName) == "PrepareReload") return &sdk::sakura::PrepareReload;
		if (std::string(sdkFunctionName) == "FinishReload") return &sdk::sakura::FinishReload;
		if (std::string(sdkFunctionName) == "Dispose") return &sdk::sakura::Dispose;

		// Solve Memory Management Utilities (Anzen) Functions
		if (std::string(sdkFunctionName) == "GetGlobalPointer") return &sdk::GetGlobalPointer;
		if (std::string(sdkFunctionName) == "SetGlobalPointer") return &sdk::SetGlobalPointer;
		if (std::string(sdkFunctionName) == "DeleteGlobalPointer") return &sdk::DeleteGlobalPointer;
		if (std::string(sdkFunctionName) == "AllocateGlobalMemory") return &sdk::AllocateGlobalMemory;
		if (std::string(sdkFunctionName) == "FreeGlobalMemory") return &sdk::FreeGlobalMemory;

		// Solve Global Variable Storage Utilities Functions
		if (std::string(sdkFunctionName) == "GetGlobalVariable") return &sdk::GetGlobalVariable;
		if (std::string(sdkFunctionName) == "SetGlobalVariable") return &sdk::SetGlobalVariable;
		if (std::string(sdkFunctionName) == "ClearGlobalVariables") return &sdk::ClearGlobalVariables;

		// Solve Task System Utilities Functions
		if (std::string(sdkFunctionName) == "InitiateTask") return &sdk::InitiateTask;
		if (std::string(sdkFunctionName) == "IsTaskComplete") return &sdk::IsTaskComplete;
		if (std::string(sdkFunctionName) == "ClearTask") return &sdk::ClearTask;

		// Invalid Function
		return nullptr;
	}
}

// Static Runtime SDK Implementation
#ifdef JENOVA_SDK_STATIC
namespace jenova
{
	/*
		Following APIs Are Replicates from jenova.hpp
		And Are Only Available in Static Build of SDK.
	*/
	void Output(sdk::StringPtr fmt, ...)
	{
		char buffer[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);
		godot::UtilityFunctions::print(godot::String("[JENOVA-SDK] > ") + godot::String(buffer));
	}
	std::string ConvertToStdString(const godot::String& gstr)
	{
		std::string str((char*)gstr.utf8().ptr(), gstr.utf8().size());
		if (!str.empty() && str.back() == '\0') str.pop_back();
		return str;
	}
	bool RegisterRuntimeEventCallback(jenova::FunctionPointer runtimeCallback)
	{
		UtilityFunctions::push_error(
			"API Function `RegisterRuntimeEventCallback` in JenovaSDK is not available when using SDK static linking.\n"
			"Please ensure dynamic linking is enabled or refer to the SDK documentation for alternative solutions."
		);
		return false;
	}
	bool UnregisterRuntimeEventCallback(jenova::FunctionPointer runtimeCallback)
	{
		UtilityFunctions::push_error(
			"API Function `UnregisterRuntimeEventCallback` in JenovaSDK is not available when using SDK static linking.\n"
			"Please ensure dynamic linking is enabled or refer to the SDK documentation for alternative solutions."
		);
		return false;
	}
	HWND GetMainWindowNativeHandle()
	{
		return HWND(DisplayServer::get_singleton()->window_get_native_handle(DisplayServer::HandleType::WINDOW_HANDLE));
	}
}
#endif