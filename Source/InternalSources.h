#pragma once
namespace jenova::resources {
static constexpr char JenovaModuleInitializerCPP[] = R"(

	// Godot SDK
	#include <Godot/godot.hpp>

	// Jenova SDK
	#include <JenovaSDK.h>

	// Namespaces
	using namespace godot;

	// Dummy Callbacks
	void DummyCallback(ModuleInitializationLevel initLevel) {}

	// Jenova SDK Interface
	namespace jenova::sdk
	{
		/* Singleton Accessed From Scripts */
		JenovaSDK* bridge = nullptr;
	}

	// Extension Initialization Data
	struct InitializerData
	{
		GDExtensionInterfaceGetProcAddress		godotGetProcAddress;
		GDExtensionClassLibraryPtr				godotExtensionClassLibraryPtr;
		GDExtensionInitialization*				godotExtensionInitialization;
		jenova::sdk::JenovaSDKInterface			jenovaSDKInterface;
	};

	// Extension Initializer
	JENOVA_EXPORT GDExtensionBool InitializeJenovaModule(InitializerData* initializerData)
	{
		// Initialize Jenova SDK Interface
		jenova::sdk::bridge = (jenova::sdk::JenovaSDK*)initializerData->jenovaSDKInterface;

		// Initailize Object & Solve API Functions
		GDExtensionBinding::InitObject init_obj(initializerData->godotGetProcAddress, 
												initializerData->godotExtensionClassLibraryPtr, 
												initializerData->godotExtensionInitialization);
		init_obj.register_initializer(DummyCallback);
		init_obj.register_terminator(DummyCallback);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
		return init_obj.init();
	}

)";
}