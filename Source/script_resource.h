#pragma once

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
#include "Jenova.hpp""

// Jenova Script Resource Loader Difinition
class CPPScriptResourceLoader : public ResourceFormatLoader
{
	GDCLASS(CPPScriptResourceLoader, ResourceFormatLoader);

protected:
	static void _bind_methods() {}

public:
	static void init();
	static void deinit();
	virtual Variant _load(const String& path, const String& original_path, bool use_sub_threads, int32_t cache_mode) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual bool _handles_type(const StringName& type) const override;
	virtual String _get_resource_type(const String& p_path) const override;
};

// Jenova Script Resource Saver Difinition
class CPPScriptResourceSaver : public ResourceFormatSaver {
	GDCLASS(CPPScriptResourceSaver, ResourceFormatSaver);

protected:
	static void _bind_methods() {}

public:
	static void init();
	static void deinit();
	virtual Error _save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
	virtual Error _set_uid(const String& p_path, int64_t p_uid) override;
	virtual bool _recognize(const Ref<Resource>& p_resource) const override;
	virtual PackedStringArray _get_recognized_extensions(const Ref<Resource>& p_resource) const override;
	virtual bool _recognize_path(const Ref<Resource>& p_resource, const String& p_path) const override;
};

// Jenova Header Resource Loader Difinition
class CPPHeaderResourceLoader : public ResourceFormatLoader
{
	GDCLASS(CPPHeaderResourceLoader, ResourceFormatLoader);

protected:
	static void _bind_methods() {}

public:
	static void init();
	static void deinit();
	virtual Variant _load(const String& path, const String& original_path, bool use_sub_threads, int32_t cache_mode) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual bool _handles_type(const StringName& type) const override;
	virtual String _get_resource_type(const String& p_path) const override;
};

// Jenova Header Resource Saver Difinition
class CPPHeaderResourceSaver : public ResourceFormatSaver {
	GDCLASS(CPPHeaderResourceSaver, ResourceFormatSaver);

protected:
	static void _bind_methods() {}

public:
	static void init();
	static void deinit();
	virtual Error _save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags) override;
	virtual Error _set_uid(const String& p_path, int64_t p_uid) override;
	virtual bool _recognize(const Ref<Resource>& p_resource) const override;
	virtual PackedStringArray _get_recognized_extensions(const Ref<Resource>& p_resource) const override;
	virtual bool _recognize_path(const Ref<Resource>& p_resource, const String& p_path) const override;
};