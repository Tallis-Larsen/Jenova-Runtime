
/*-------------------------------------------------------------+
|                                                              |
|                   _________   ______ _    _____              |
|                  / / ____/ | / / __ \ |  / /   |             |
|             __  / / __/ /  |/ / / / / | / / /| |             |
|            / /_/ / /___/ /|  / /_/ /| |/ / ___ |             |
|            \____/_____/_/ |_/\____/ |___/_/  |_|             |
|                                                              |
|                        Jenova Runtime                        |
|                   Developed by Hamid.Memar                   |
|                                                              |
+-------------------------------------------------------------*/

// Jenova SDK
#include "Jenova.hpp"

// Macros
#define BUILT_IN_TEMPLATE 0

// Global Storage
Vector<Dictionary> globalTemplates;
Vector<Dictionary> classTemplates;

// Singleton Instance
JenovaTemplateManager* jnvtm_singleton = nullptr;

// Initializer/Deinitializer
void JenovaTemplateManager::init()
{
	// Register Class
	ClassDB::register_internal_class<JenovaTemplateManager>();

	// Initialize Singleton
	jnvtm_singleton = memnew(JenovaTemplateManager);

	// Verbose
	jenova::Output("Jenova Template Manager Initialized.");
}
void JenovaTemplateManager::deinit()
{
	// Release Singleton
	if (jnvtm_singleton) memdelete(jnvtm_singleton);
}

// Bindings
void JenovaTemplateManager::_bind_methods()
{
}

// Singleton Handling
JenovaTemplateManager* JenovaTemplateManager::get_singleton()
{
	return jnvtm_singleton;
}

// Jenova Script Template Implementation
bool JenovaTemplateManager::RegisterNewGlobalScriptTemplate(const String& templateName, const String& templateCode, const String& templateDescription)
{
	Dictionary newTemplateData;
	newTemplateData["inherit"] = "";
	newTemplateData["name"] = templateName;
	newTemplateData["description"] = templateDescription;
	newTemplateData["content"] = templateCode;
	newTemplateData["id"] = globalTemplates.size();
	newTemplateData["origin"] = BUILT_IN_TEMPLATE;
	globalTemplates.push_back(newTemplateData);
	return true;
}
bool JenovaTemplateManager::RegisterNewClassScriptTemplate(const String& templateName, const String& className, const String& templateCode, const String& templateDescription)
{
	Dictionary newTemplateData;
	newTemplateData["inherit"] = className;
	newTemplateData["name"] = templateName;
	newTemplateData["description"] = templateDescription;
	newTemplateData["content"] = templateCode;
	newTemplateData["id"] = classTemplates.size();
	newTemplateData["origin"] = BUILT_IN_TEMPLATE;
	classTemplates.push_back(newTemplateData);
	return true;
}
TypedArray<Dictionary> JenovaTemplateManager::GetGlobalScriptTemplates()
{
	TypedArray<Dictionary> scriptTemplates;
	for (const auto& globalTemplate : globalTemplates) scriptTemplates.push_back(globalTemplate);
	return scriptTemplates;
}
String JenovaTemplateManager::GetGlobalScriptTemplateSource(const String& templateName)
{
	for (const auto& globalTemplate : globalTemplates) 
		if (String(globalTemplate["name"]) == templateName) return globalTemplate["content"];
	return String("/* Global Template Not Found */");
}
TypedArray<Dictionary> JenovaTemplateManager::GetClassScriptTemplates(const String& className)
{
	TypedArray<Dictionary> scriptTemplates;
	for (const auto& classTemplate : classTemplates)
		if (String(classTemplate["inherit"]) == className) scriptTemplates.push_back(classTemplate);
	return scriptTemplates;
}
String JenovaTemplateManager::GetClassScriptTemplateSource(const String& templateName, const String& className)
{
	for (const auto& classTemplate : classTemplates)
		if (String(classTemplate["name"]) == templateName && String(classTemplate["inherit"]) == className) return classTemplate["content"];
	return String("/* Class Template Not Found */");
}