
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

// Archive SDK
#define LIBARCHIVE_STATIC
#include "Archive/archive.h"
#include "Archive/archive_entry.h"

// Curl SDK
#define CURL_STATICLIB
#include "Curl/curl.h"

// Resources
#include "IconDatabase.h"

// Helper Macros
#define BIND_TOOL_ACTION(toolBtn, toolID)	toolBtn->connect("pressed", callable_mp(pacmanEventManager, &PackmanEventManager::OnToolPressed).bind(String(toolID)));

// Configuration
constexpr const char* packageDatabaseFileURL = "/Jenova-Framework/Jenova-Packages/refs/heads/main/Jenova.Package.Database.json";

// Storages
static jenova::PackageList	onlinePackages;
static jenova::PackageList	installedPackages;

// Singleton Instance
JenovaPackageManager* jnvpm_singleton = nullptr;

// Initializer/Deinitializer
void JenovaPackageManager::init()
{
    // Register Class
    ClassDB::register_internal_class<JenovaPackageManager>();

    // Initialize Singleton
	jnvpm_singleton = memnew(JenovaPackageManager);

    // Verbose
    jenova::Output("Jenova Package Manager Initialized.");
}
void JenovaPackageManager::deinit()
{
	// Release Packages
	onlinePackages.clear();
	installedPackages.clear();

    // Release Singleton
    if (jnvpm_singleton) memdelete(jnvpm_singleton);
}

// Bindings
void JenovaPackageManager::_bind_methods()
{
    ClassDB::bind_static_method("JenovaPackageManager", D_METHOD("GetInstance"), &JenovaPackageManager::get_singleton);
    ClassDB::bind_method(D_METHOD("OpenPackageManager"), &JenovaPackageManager::OpenPackageManager);
	ClassDB::bind_method(D_METHOD("UpdateStatus"), &JenovaPackageManager::UpdateStatus);
	ClassDB::bind_method(D_METHOD("ForceUpdatePackageList"), &JenovaPackageManager::ForceUpdatePackageList);
}

// Singleton Handling
JenovaPackageManager* JenovaPackageManager::get_singleton()
{
    return jnvpm_singleton;
}

// Jenova Package Manager Implementation
bool JenovaPackageManager::OpenPackageManager(const String& packageDatabaseURL)
{
	// Prepare Package Manager
	if (!PreparePackageManager())
	{
		jenova::Error("Jenova Package Manager", "Failed to Prepare Package Manager.");
		return false;
	};

	// Get Scale Factor
	double scaleFactor = EditorInterface::get_singleton()->get_editor_scale();

	// Get Editor Theme
	Ref<Theme> editor_theme = EditorInterface::get_singleton()->get_editor_theme();
	if (!editor_theme.is_valid())
	{
		jenova::Error("Jenova Package Manager", "Failed to Obtain Engine Theme.");
		return false;
	}

	// Create Window
	currentWindow = memnew(Window);
	currentWindow->set_title("Package Manager");
	currentWindow->set_name("PackageManagerWindow");
	currentWindow->set_size(Vector2i(SCALED(950), SCALED(600)));
	currentWindow->set_min_size(Vector2i(SCALED(950), SCALED(600)));
	currentWindow->set_flag(Window::Flags::FLAG_POPUP, true);

	// Show Window [Must Be Here]
	currentWindow->hide();
	currentWindow->popup_exclusive_centered(EditorInterface::get_singleton()->get_base_control());
	if (!currentWindow->get_flag(Window::Flags::FLAG_POPUP)) EditorInterface::get_singleton()->get_base_control()->get_tree()->set_pause(true);
	currentWindow->set_owner(EditorInterface::get_singleton()->get_base_control()->get_window());

	// Add Background
	ColorRect* background = memnew(ColorRect);
	background->set_name("Background");
	background->set_anchors_preset(Control::PRESET_FULL_RECT);
	background->set_color(editor_theme->get_color("dark_color_1", "Editor"));
	currentWindow->add_child(background);

	// Add Status Label
	statusLabel = memnew(RichTextLabel);
	statusLabel->set_name("Status");
	statusLabel->set_text("Status");
	statusLabel->set_anchors_preset(Control::PRESET_TOP_WIDE);
	statusLabel->set_anchor(Side::SIDE_LEFT, 1.0);
	statusLabel->set_anchor(Side::SIDE_RIGHT, 1.0);
	statusLabel->set_offset(Side::SIDE_LEFT, SCALED(-70.0));
	statusLabel->set_offset(Side::SIDE_TOP, SCALED(20.0));
	statusLabel->set_offset(Side::SIDE_RIGHT, SCALED(-20.0));
	statusLabel->set_offset(Side::SIDE_BOTTOM, SCALED(40.0));
	statusLabel->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_OFF);
	statusLabel->set_fit_content(true);
	statusLabel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	statusLabel->set_theme(nullptr);
	statusLabel->add_theme_stylebox_override("normal", memnew(StyleBoxEmpty));
	currentWindow->add_child(statusLabel);

	// Add Toolbar
	HBoxContainer* toolbar = memnew(HBoxContainer);
	toolbar->set_name("Toolbar");
	toolbar->set_anchors_preset(Control::PRESET_TOP_WIDE);
	toolbar->set_custom_minimum_size(Vector2(0, SCALED(32)));
	toolbar->set_anchor(Side::SIDE_RIGHT, 1.0);
	toolbar->set_offset(Side::SIDE_LEFT, SCALED(20.0));
	toolbar->set_offset(Side::SIDE_TOP, SCALED(15.0));
	toolbar->set_offset(Side::SIDE_RIGHT, SCALED(-330.0));
	toolbar->set_offset(Side::SIDE_BOTTOM, SCALED(40.0));
	toolbar->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	toolbar->add_theme_constant_override("separation", SCALED(10));
	currentWindow->add_child(toolbar);

	// Create Toolbar Icons
	auto githubIcon = CREATE_SVG_MENU_ICON(JENOVA_RESOURCE(SVG_GITHUB_ICON));
	auto downloadIcon = CREATE_SVG_MENU_ICON(JENOVA_RESOURCE(SVG_DOWNLOAD_ICON));

	// Add Toolbar Items
	Button* open_repository_tool = CreateToolbarItem("OpenRepositoryTool", githubIcon, "Open Packages Database GitHub Repository", toolbar);
	Button* reload_database_tool = CreateToolbarItem("ReloadDatabaseTool", jenova::GetEditorIcon("PreviewRotate"), "Refresh Online Package Database", toolbar);
	Button* stop_downloads_tool = CreateToolbarItem("StopDownloadTool", jenova::GetEditorIcon("Stop"), "Stop All Downloads & Tasks", toolbar);
	Button* download_pkg_database_tool = CreateToolbarItem("DownloadDatabaseTool", downloadIcon, "Download Package Database", toolbar);

	// Add Browser Block
	Panel* browser_block = memnew(Panel);
	browser_block->set_name("BrowserBlock");
	browser_block->set_anchors_preset(Control::PRESET_FULL_RECT);
	browser_block->set_anchor(Side::SIDE_RIGHT, 1.0);
	browser_block->set_anchor(Side::SIDE_BOTTOM, 1.0);
	browser_block->set_offset(Side::SIDE_LEFT, SCALED(20.0));
	browser_block->set_offset(Side::SIDE_TOP, SCALED(60.0));
	browser_block->set_offset(Side::SIDE_RIGHT, SCALED(-20.0));
	browser_block->set_offset(Side::SIDE_BOTTOM, SCALED(-20.0));
	browser_block->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	browser_block->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	currentWindow->add_child(browser_block);

	// Add Background
	ColorRect* browser_block_background = memnew(ColorRect);
	browser_block_background->set_name("BrowserBackground");
	browser_block_background->set_anchors_preset(Control::PRESET_FULL_RECT);
	browser_block_background->set_color(editor_theme->get_color("base_color", "Editor"));
	browser_block_background->set_anchors_preset(Control::PRESET_FULL_RECT);
	browser_block_background->set_offset(Side::SIDE_TOP, SCALED(40.0));
	browser_block_background->set_offset(Side::SIDE_LEFT, SCALED(1.0));
	browser_block->add_child(browser_block_background);

	// Add Browser Tab
	TabBar* browser_tab = memnew(TabBar);
	browser_tab->set_name("BrowserTab");
	browser_tab->set_anchors_preset(Control::PRESET_TOP_WIDE);
	browser_tab->set_anchor(Side::SIDE_RIGHT, 1.0);
	browser_tab->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	browser_tab->set_custom_minimum_size(Vector2(0, SCALED(40)));
	browser_tab->add_tab("Compilers");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::Compiler), "Compilers Available for Jenova Build System");
	browser_tab->add_tab("Godot Kits");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::GodotKit), "Godot Software Development Kits");
	browser_tab->add_tab("Libraries");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::Library), "Prepared Libraries for Jenova Framework");
	browser_tab->add_tab("Sample Projects");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::SampleProject), "Sample Projects Made With Jenova Framework");
	browser_tab->add_tab("Code Templates");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::CodeTemplate), "Code Templates for Easier Prototyping In Jenova");
	browser_tab->add_tab("Addons");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::Addon), "Plug-Ins Developed For Jenova");
	browser_tab->add_tab("Tools");
	browser_tab->set_tab_tooltip(int32_t(jenova::PackageType::Tool), "Third-Party Tools Made for Jenova Framework");
	browser_tab->set_current_tab(0);
	browser_tab->add_theme_stylebox_override("tab_focus", memnew(StyleBoxEmpty));
	browser_block->add_child(browser_tab);

	// Add Browser Tab Scroll Block
	ScrollContainer* browser_tab_scroll_block = memnew(ScrollContainer);
	browser_tab_scroll_block->set_name("BrowserTabScrollBlock");
	browser_tab_scroll_block->set_anchors_preset(Control::PRESET_FULL_RECT);
	browser_tab_scroll_block->set_offset(Side::SIDE_LEFT, SCALED(10.0));
	browser_tab_scroll_block->set_offset(Side::SIDE_TOP, SCALED(50.0));
	browser_tab_scroll_block->set_offset(Side::SIDE_RIGHT, SCALED(-10.0));
	browser_tab_scroll_block->set_offset(Side::SIDE_BOTTOM, SCALED(-10.0));
	browser_tab_scroll_block->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	browser_tab_scroll_block->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	browser_block->add_child(browser_tab_scroll_block);

	// Add Browser Tab Content
	GridContainer* browser_tab_content = memnew(GridContainer);
	browser_tab_content->set_name("BrowserTabContent");
	browser_tab_content->set_anchors_preset(Control::PRESET_TOP_WIDE);
	browser_tab_content->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	browser_tab_content->add_theme_constant_override("v_separation", SCALED(10));
	browser_tab_scroll_block->add_child(browser_tab_content);
	browserContent = browser_tab_content;

	// Define Internal UI Callback Handler
	class PackmanEventManager : public Object
	{
	private:
		JenovaPackageManager* pkgManagerInstance = nullptr;
		Window* window = nullptr;

	public:
		PackmanEventManager(JenovaPackageManager* _pkgman, Window* _window) : pkgManagerInstance(_pkgman), window(_window) { }
		void OnToolPressed(const String& toolID)
		{
			// Open GitHub Repository
			if (toolID == "OpenRepository")
			{
				jenova::OpenURL("https://github.com/Jenova-Framework/Jenova-Packages");
			}
			// Download Package Database
			if (toolID == "DownloadDatabase")
			{
				jenova::OpenURL("https://raw.githubusercontent.com/Jenova-Framework/Jenova-Packages/refs/heads/main/Jenova.Package.Database.json");
			}
			// Open GitHub Repository
			if (toolID == "ReloadDatabase")
			{
				pkgManagerInstance->ReloadEntireDatabase();
			}
		}
		void OnTabChanged(int32_t tabIndex)
		{
			pkgManagerInstance->UpdatePackageList(jenova::PackageType(tabIndex));
		}
		void OnWindowClose()
		{
			if (!pkgManagerInstance->CanClose()) return;
			jenova::ReleasePopUpWindow(window);
			pkgManagerInstance->PrepareForClose();
			window->queue_free();
			memdelete(this);
		}
	};

	// Create Internal UI Callback Handler
	PackmanEventManager* pacmanEventManager = memnew(PackmanEventManager(this, currentWindow));

	// Create & Assign Callbacks
	currentWindow->connect("close_requested", callable_mp(pacmanEventManager, &PackmanEventManager::OnWindowClose));
	browser_tab->connect("tab_changed", callable_mp(pacmanEventManager, &PackmanEventManager::OnTabChanged));

	// Create & Assign Tool Button Actions
	BIND_TOOL_ACTION(open_repository_tool, "OpenRepository");
	BIND_TOOL_ACTION(download_pkg_database_tool, "DownloadDatabase");
	BIND_TOOL_ACTION(reload_database_tool, "ReloadDatabase");
	BIND_TOOL_ACTION(stop_downloads_tool, "StopAllTasks");

	// Create Timer
	taskTimer = memnew(Timer);
	taskTimer->set_autostart(false);
	taskTimer->set_wait_time(0.5);
	taskTimer->connect("timeout", callable_mp(this, &JenovaPackageManager::OnTaskTimerTick));
	currentWindow->add_child(taskTimer);

	// Prepare Pop Up Window
	if (!jenova::AssignPopUpWindow(currentWindow))
	{
		Window* window = currentWindow;
		PrepareForClose();
		window->hide();
		window->queue_free();
		return false;
	}

	// Set Current Package Database URL
	currentDatabaseURL = AS_STD_STRING(packageDatabaseURL);

	// Update Packages Database
	this->FormatStatus("#ababab", "Obtaining Installed Packages...");
	if (!ObtainInstalledPackages())
	{
		jenova::Error("Jenova Package Manager", "Unable to Update Installed Packages Database.");
		return false;
	}
	this->FormatStatus("#ababab", "Fetching Online Packages...");
	if (!FetchOnlinePackages(packageDatabaseURL))
	{
		jenova::Error("Jenova Package Manager", "Unable to Fetch Online Packages. Verify Your Internet Connection.");
		return false;
	}

	// Update Package List
	this->UpdatePackageList(jenova::PackageType(browser_tab->get_current_tab()));

	// Set Status
	this->FormatStatus("#ababab", "%lld Online Packages Available", onlinePackages.size());

    // All Good
    return true;
}
Button* JenovaPackageManager::CreateToolbarItem(const String& toolName, const Ref<Texture2D>& toolIcon, const String& toolTip, Control* toolbar)
{
	// Get Scale Factor
	double scaleFactor = EditorInterface::get_singleton()->get_editor_scale();

	// Create New Toolbar Item
	Button* toolbar_item = memnew(Button);
	toolbar_item->set_name(toolName);
	toolbar_item->set_custom_minimum_size(Vector2(SCALED(32), SCALED(32)));
	toolbar_item->set_flat(false);
	toolbar_item->set_tooltip_text(toolTip);
	toolbar_item->set_theme_type_variation("RunBarButton");
	toolbar_item->add_theme_stylebox_override("focus", memnew(StyleBoxEmpty));
	toolbar_item->set_button_icon(toolIcon);
	toolbar_item->set_icon_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	toolbar_item->add_theme_constant_override("icon_max_width", SCALED(20));
	
	// Add to Toolbar
	toolbar->add_child(toolbar_item);

	// Return New Toolbar Item
	return toolbar_item;
}
Panel* JenovaPackageManager::CreatePackageItem(const jenova::JenovaPackage& jenovaPackage, const Ref<Theme> itemTheme)
{
	// Get Scale Factor
	double scaleFactor = EditorInterface::get_singleton()->get_editor_scale();

	// Get Color Hashes 
	String accentColorHash = itemTheme->get_color("accent_color", "Editor").to_html();

	// Add Package Item
	Panel* package_item = memnew(Panel);
	package_item->set_name("PackageItem");
	package_item->set_custom_minimum_size(Vector2(SCALED(250), SCALED(100)));
	package_item->set_anchors_preset(Control::PRESET_FULL_RECT);
	package_item->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	package_item->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);

	// Add Package Item Background
	ColorRect* package_item_background = memnew(ColorRect);
	package_item_background->set_name("PackageItemBackground");
	package_item_background->set_anchors_preset(Control::PRESET_FULL_RECT);
	package_item_background->set_color(itemTheme->get_color("dark_color_1", "Editor").darkened(0.15f));
	package_item_background->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item_background->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item->add_child(package_item_background);

	// Add Package Item Icon
	TextureRect* package_item_icon = memnew(TextureRect);
	package_item_icon->set_name("PackageItemIcon");
	package_item_icon->set_custom_minimum_size(Vector2(SCALED(64), SCALED(64)));
	package_item_icon->set_anchors_preset(Control::PRESET_CENTER_LEFT);
	package_item_icon->set_offset(Side::SIDE_LEFT, SCALED(20.0));
	package_item_icon->set_offset(Side::SIDE_TOP, SCALED(-32.0));
	package_item_icon->set_offset(Side::SIDE_RIGHT, SCALED(84.0));
	package_item_icon->set_offset(Side::SIDE_BOTTOM, SCALED(32.0));
	package_item_icon->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item_icon->set_texture(jenovaPackage.pkgImage);
	package_item_icon->set_expand_mode(TextureRect::ExpandMode::EXPAND_IGNORE_SIZE);
	package_item_icon->set_tooltip_text(jenovaPackage.pkgHash);
	package_item->add_child(package_item_icon);

	// Add Package Item Name
	Label* package_item_name = memnew(Label);
	package_item_name->set_name("PackageItemName");
	package_item_name->set_anchors_preset(Control::PRESET_CENTER_LEFT);
	package_item_name->set_offset(Side::SIDE_LEFT, SCALED(100.0));
	package_item_name->set_offset(Side::SIDE_TOP, SCALED(-35.0));
	package_item_name->set_offset(Side::SIDE_RIGHT, SCALED(600.0));
	package_item_name->set_offset(Side::SIDE_BOTTOM, SCALED(-10.0));
	package_item_name->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item_name->add_theme_color_override("font_color", itemTheme->get_color("accent_color", "Editor"));
	package_item_name->add_theme_font_size_override("font_size", SCALED(20));
	package_item_name->set_text(jenovaPackage.pkgName);
	package_item_name->set_tooltip_text(jenovaPackage.pkgHash);
	package_item->add_child(package_item_name);

	// Add Package Item Description
	RichTextLabel* package_item_description = memnew(RichTextLabel);
	package_item_description->set_name("PackageItemDescription");
	package_item_description->set_anchors_preset(Control::PRESET_CENTER_LEFT);
	package_item_description->set_offset(Side::SIDE_LEFT, SCALED(105.0));
	package_item_description->set_offset(Side::SIDE_TOP, SCALED(-5.0));
	package_item_description->set_offset(Side::SIDE_RIGHT, SCALED(900.0));
	package_item_description->set_offset(Side::SIDE_BOTTOM, SCALED(40.0));
	package_item_description->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item_description->add_theme_font_size_override("font_size", SCALED(14));
	package_item_description->set_theme(nullptr);
	package_item_description->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_OFF);
	package_item_description->add_theme_stylebox_override("normal", memnew(StyleBoxEmpty));
	package_item_description->add_theme_constant_override("line_separation", 2.0f);
	package_item_description->parse_bbcode(vformat("%s\nSize : [color=%s]%s[/color]  Date Uploaded : [color=%s]%s[/color]  Platform : [color=%s]%s[/color]",
		jenovaPackage.pkgDescription, accentColorHash, jenova::FormatBytesSize(jenovaPackage.pkgSize), accentColorHash, jenovaPackage.pkgDate,
		accentColorHash, AS_C_STRING(JenovaPackageManager::GetPackagePlatformName(jenovaPackage.pkgPlatform))));
	package_item->add_child(package_item_description);

	// Add Package Item Install Button
	Button* package_item_install_button = memnew(Button);
	package_item_install_button->set_name("PackageItemInstallButton");
	package_item_install_button->set_anchors_preset(Control::PRESET_CENTER_RIGHT);
	package_item_install_button->set_offset(Side::SIDE_LEFT, SCALED(-170.0));
	package_item_install_button->set_offset(Side::SIDE_TOP, SCALED(-22.0));
	package_item_install_button->set_offset(Side::SIDE_RIGHT, SCALED(-30.0));
	package_item_install_button->set_offset(Side::SIDE_BOTTOM, SCALED(22.0));
	package_item_install_button->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
	package_item_install_button->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	package_item_install_button->add_theme_color_override("font_color", itemTheme->get_color("accent_color", "Editor"));
	package_item->add_child(package_item_install_button);

	// Check If Package Installed
	bool isPackageInstalled = false;
	for (const auto& installedPackage : installedPackages)
	{
		if (jenovaPackage.pkgHash == installedPackage.pkgHash) isPackageInstalled = true;
	}

	// Adjust Button Based On Package State
	if (isPackageInstalled)
	{
		// Assign Remove Action
		package_item_install_button->set_text("  Uninstall Package  ");
		package_item_install_button->connect("pressed", callable_mp(this, &JenovaPackageManager::UtilizeNewPackageTask)
			.bind(String("UninstallPackagePrepare"), jenovaPackage.pkgHash, package_item_install_button));
	}
	else
	{
		// Assign Download Action
		package_item_install_button->set_text("  Install Package  ");
		package_item_install_button->connect("pressed", callable_mp(this, &JenovaPackageManager::UtilizeNewPackageTask)
			.bind(String("InstallPackagePrepare"), jenovaPackage.pkgHash, package_item_install_button));
	}

	// Return New Package Item
	return package_item;
}
void JenovaPackageManager::ReloadEntireDatabase()
{
	// Update Packages Database
	this->FormatStatus("#ababab", "Obtaining Installed Packages...");
	if (!ObtainInstalledPackages())
	{
		FormatStatus("#ff1717", "Failed to Reload Database.");
		return;
	}
	this->FormatStatus("#ababab", "Fetching Online Packages...");
	if (!FetchOnlinePackages(String(currentDatabaseURL.c_str())))
	{
		FormatStatus("#ff1717", "Failed to Reload Database.");
		return;
	}
	ForceUpdatePackageList();

	// Set Status
	this->FormatStatus("#ababab", "%lld Online Packages Available", onlinePackages.size());
}
bool JenovaPackageManager::FetchOnlinePackages(const String& packageDatabaseURL)
{
	// Clear Current List
	onlinePackages.clear();

	// Verbose
	jenova::VerboseByID(__LINE__, "Downloading [%s%s]", AS_C_STRING(packageDatabaseURL), packageDatabaseFileURL);

	// Initialize cURL
	CURL* curl = curl_easy_init();
	if (!curl) { return false; }

	// Prepare URLs
	std::string fullURL = AS_C_STRING(packageDatabaseURL) + std::string(packageDatabaseFileURL);

	// Response Buffer
	std::string responseData;

	// Configure cURL
	curl_easy_setopt(curl, CURLOPT_URL, fullURL.c_str());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "JenovaPackageManager/1.0");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, JenovaPackageManager::OnFetchBufferWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

	// Perform the Request
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) 
	{
		jenova::Error("Jenova Package Manager", "Package List Fetch Request Failed : %s", curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		return false;
	}

	// Cleanup cURL
	curl_easy_cleanup(curl);

	// Parse Package Database
	try 
	{
		// Parse JSON Database
		nlohmann::json packageDatabase = nlohmann::json::parse(responseData);

		// Iterate Over Packages
		for (const auto& packageItem : packageDatabase["packages"]) 
		{
			// Create Package
			jenova::JenovaPackage newPackage;
			newPackage.pkgName = String(packageItem["pkgName"].get<std::string>().c_str());
			newPackage.pkgVersion = String(packageItem["pkgVersion"].get<std::string>().c_str());
			newPackage.pkgDescription = String(packageItem["pkgDescription"].get<std::string>().c_str());
			newPackage.pkgHash = String(packageItem["pkgHash"].get<std::string>().c_str());
			newPackage.pkgType = packageItem["pkgType"].get<jenova::PackageType>();
			newPackage.pkgPlatform = packageItem["pkgPlatform"].get<jenova::PackagePlatform>();
			newPackage.pkgSize = packageItem["pkgSize"].get<uint32_t>();
			newPackage.pkgDate = String(packageItem["pkgDate"].get<std::string>().c_str());
			newPackage.pkgURL = String(packageItem["pkgURL"].get<std::string>().c_str());
			newPackage.pkgDestination = String(packageItem["pkgDestination"].get<std::string>().c_str());
			newPackage.pkgInstallScript = packageItem["pkgInstallScript"].get<bool>();
			newPackage.pkgUninstallScript = packageItem["pkgUninstallScript"].get<bool>();

			// Create Package Icon
			double scaleFactor = EditorInterface::get_singleton()->get_editor_scale();
			Vector2 packageIconSize(SCALED(64), SCALED(64));
			std::string packageIconBuffer = base64::base64_decode(packageItem["pkgImage"].get<std::string>());
			newPackage.pkgImage = jenova::CreateImageTextureFromByteArrayEx((uint8_t*)packageIconBuffer.data(), packageIconBuffer.size(), packageIconSize);

			// Add Package
			onlinePackages.push_back(newPackage);
		}

		// All Good
		return true;
	}
	catch (const std::exception& e) 
	{
		jenova::VerboseByID(__LINE__, "JSON Parsing failed: %s", e.what());
		return false;
	}
}
bool JenovaPackageManager::ObtainInstalledPackages()
{
	// Clear Current List
	installedPackages.clear();

	// Create Installed Package File Path
	String projectPath = jenova::GetJenovaProjectDirectory();
	String installedPackageFile = projectPath + "Jenova/" + jenova::GlobalSettings::JenovaInstalledPackagesFile;

	// Validate Package Database File
	if (!std::filesystem::exists(AS_STD_STRING(installedPackageFile)))
	{
		if (!CacheInstalledPackages()) return false;
	}

	// Read Pacakge Database
	std::string packageDatabaseContent = jenova::ReadStdStringFromFile(AS_STD_STRING(installedPackageFile));

	// Validate Package Database Content
	if (packageDatabaseContent.empty()) packageDatabaseContent = "{\"packages\" : []}";

	// Parse Package Database
	try
	{
		// Parse JSON Database
		nlohmann::json packageDatabase = nlohmann::json::parse(packageDatabaseContent);

		// Iterate Over Packages
		for (const auto& packageItem : packageDatabase["packages"])
		{
			// Create Package
			jenova::JenovaPackage newPackage;
			newPackage.pkgName = String(packageItem["pkgName"].get<std::string>().c_str());
			newPackage.pkgVersion = String(packageItem["pkgVersion"].get<std::string>().c_str());
			newPackage.pkgHash = String(packageItem["pkgHash"].get<std::string>().c_str());
			newPackage.pkgType = packageItem["pkgType"].get<jenova::PackageType>();
			newPackage.pkgPlatform = packageItem["pkgPlatform"].get<jenova::PackagePlatform>();
			newPackage.pkgDestination = String(packageItem["pkgDestination"].get<std::string>().c_str());
			newPackage.pkgInstallScript = packageItem["pkgInstallScript"].get<bool>();
			newPackage.pkgUninstallScript = packageItem["pkgUninstallScript"].get<bool>();

			// Add Package
			installedPackages.push_back(newPackage);
		}

		// All Good
		return true;
	}
	catch (const std::exception&)
	{
		// Failed
		return false;
	}

	// All Good
	return true;
}
bool JenovaPackageManager::CacheInstalledPackages()
{
	// Create Installed Package File Path
	String projectPath = jenova::GetJenovaProjectDirectory();
	String installedPackageFile = projectPath + "Jenova/" + jenova::GlobalSettings::JenovaInstalledPackagesFile;

	// Serialize Package Database
	try
	{
		// Create Serializer
		nlohmann::json packageDatabase;

		// Create Packages Array
		packageDatabase["packages"] = nlohmann::json::array();

		// Iterate Over Packages
		for (const auto& installedPackage : installedPackages)
		{
			// Create JSON object for each package
			nlohmann::json packageItem;
			packageItem["pkgName"] = AS_STD_STRING(installedPackage.pkgName);
			packageItem["pkgVersion"] = AS_STD_STRING(installedPackage.pkgVersion);
			packageItem["pkgHash"] = AS_STD_STRING(installedPackage.pkgHash);
			packageItem["pkgType"] = installedPackage.pkgType;
			packageItem["pkgPlatform"] = installedPackage.pkgPlatform;
			packageItem["pkgDestination"] = AS_STD_STRING(installedPackage.pkgDestination);
			packageItem["pkgInstallScript"] = installedPackage.pkgInstallScript;
			packageItem["pkgUninstallScript"] = installedPackage.pkgUninstallScript;

			// Add package item to the packages array
			packageDatabase["packages"].push_back(packageItem);
		}

		// Write Package Database On Disk
		if (!jenova::WriteStdStringToFile(AS_STD_STRING(installedPackageFile), packageDatabase.dump(2))) return false;

		// All Good
		return true;
	}
	catch (const std::exception&)
	{
		// Failed
		return false;
	}
}
jenova::JenovaPackage JenovaPackageManager::GetOnlinePackage(const String& packageHash)
{
	// Search Packages
	for (const auto& onlinePackage : onlinePackages)
	{
		if (onlinePackage.pkgHash == packageHash) return onlinePackage;
	}

	// Not Found
	jenova::JenovaPackage nullPkg;
	nullPkg.pkgName = "[NotFound]";
	return nullPkg;
}
String JenovaPackageManager::GetPackagePlatformName(jenova::PackagePlatform pkgPlatform)
{
	switch (pkgPlatform)
	{
	case jenova::PackagePlatform::WindowsAMD64:
		return "Windows (x64)";
	case jenova::PackagePlatform::LinuxAMD64:
		return "Linux (x64)";
	case jenova::PackagePlatform::WindowsARM64:
		return "Windows (arm64)";
	case jenova::PackagePlatform::LinuxARM64:
		return "Linux (arm64)";
	case jenova::PackagePlatform::AndroidARM64:
		return "Android (arm64)";
	case jenova::PackagePlatform::iOSARM64:
		return "iOS (arm64)";
	case jenova::PackagePlatform::MacOSARM64:
		return "MacOS (arm64)";
	case jenova::PackagePlatform::Universal:
		return "Universal";
	case jenova::PackagePlatform::Unknown:
	default:
		return "Unknown";
	}
}
jenova::PackageList JenovaPackageManager::GetInstalledPackages(const jenova::PackageType& packageType)
{
	// Update Installed Packages
	jenova::PackageList collectedPackages;
	if (!ObtainInstalledPackages()) return collectedPackages;
	
	// Collect Installed Packages
	for (const auto& installedPackage : installedPackages)
	{
		// Collect All Packages
		if (packageType == jenova::PackageType::All)
		{
			collectedPackages.push_back(installedPackage);
			continue;
		}

		// Only Collect If Type Match
		if (packageType == installedPackage.pkgType) collectedPackages.push_back(installedPackage);
	}

	// Return Installed Packages
	return collectedPackages;
}
bool JenovaPackageManager::ExtractPackage(const String& packageFile, const String& destinationPath)
{
	// Create Destination If Not Exist
	if (!std::filesystem::exists(AS_STD_STRING(destinationPath)))
	{
		if (!std::filesystem::create_directories(AS_STD_STRING(destinationPath)))
		{
			jenova::Error("Jenova Package Extractor", "Failed to Create Destination Directory.");
			return false;
		};
	}

	// Create Archvie Extractor
	struct archive* a;
	struct archive* ext;
	struct archive_entry* entry;
	int flags;
	int r;

	// Helper Function
	auto copyData = [&](struct archive* ar, struct archive* aw)
	{
		int r;
		const void* buff;
		size_t size;
		la_int64_t offset;
		for (;;) 
		{
			r = archive_read_data_block(ar, &buff, &size, &offset);
			if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
			if (r < ARCHIVE_OK) return (r);
			r = archive_write_data_block(aw, buff, size, offset);
			if (r < ARCHIVE_OK) return (r);
		}
	};

	// Configuration
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	
	// Extract Package
	this->FormatStatus("#e6256f", "Extracting Archive Using LZMA %s", archive_liblzma_version());
	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);
	if ((r = archive_read_open_filename(a, AS_C_STRING(packageFile), 10240))) return false;
	for (;;)
	{
		// Read Next Header
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) break;
		if (r < ARCHIVE_OK) jenova::Error("Jenova Package Extractor", "Failed to Extract Package, Reason [%d] : %s", __LINE__, archive_error_string(a));
		if (r < ARCHIVE_WARN) return false;

		// Set Output Path
		String newFilePath = destinationPath + String("/") + String(archive_entry_pathname(entry));
		archive_entry_set_pathname(entry, AS_C_STRING(newFilePath));

		// Extract Entity
		this->FormatStatus("#4287f5", "Extracting File %s...", std::filesystem::path(archive_entry_pathname(entry)).filename().string().c_str());
		r = archive_write_header(ext, entry);
		if (r < ARCHIVE_OK) jenova::Error("Jenova Package Extractor", "Failed to Extract Package, Reason [%d] : %s", __LINE__, archive_error_string(a));
		else if (archive_entry_size(entry) > 0)
		{
			r = copyData(a, ext);
			if (r < ARCHIVE_OK) jenova::Error("Jenova Package Extractor", "Failed to Extract Package, Reason [%d] : %s", __LINE__, archive_error_string(a));
			if (r < ARCHIVE_WARN) return false;
		}
		r = archive_write_finish_entry(ext);
		if (r < ARCHIVE_OK) jenova::Error("Jenova Package Extractor", "Failed to Extract Package, Reason [%d] : %s", __LINE__, archive_error_string(a));
		if (r < ARCHIVE_WARN) return false;
	}
	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	// All Good
	return true;
}
bool JenovaPackageManager::DownloadPackage(const String& packageFileURL, const String& downloadFilePath)
{
	// Initialize CURL
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	curlHandle = curl_easy_init();
	if (!curlHandle)
	{
		jenova::Error("Jenova Package Downloader", "Failed to initialize CURL");
		return false;
	}

	// Create & Open Destination
	std::fstream outFile(AS_STD_STRING(downloadFilePath), std::ios::binary | std::ios::out);
	if (!outFile.is_open())
	{
		curl_easy_cleanup(curlHandle);
		curlHandle = nullptr;
		jenova::Error("Jenova Package Downloader", "Failed to Open Destination File For Writing");
		return false;
	}

	// Set Download URL
	curl_easy_setopt(curlHandle, CURLOPT_URL, AS_C_STRING(packageFileURL));

	// Enable SSL
	curl_easy_setopt(curlHandle, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 2L);

	// Set Write Function
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, JenovaPackageManager::OnDownloadBufferWrite);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &outFile);

	// Configuring Curl
	curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPROXYTUNNEL, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "Jenova Package Downloader/1.0 (Microsoft Windows) (x64) English");
	curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curlHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	// curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 60L);
	// curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10L);

	// Set Progress Function
	curl_easy_setopt(curlHandle, CURLOPT_XFERINFOFUNCTION, JenovaPackageManager::OnDownloadProgress);
	curl_easy_setopt(curlHandle, CURLOPT_XFERINFODATA, this);
	curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 0L);

	// Perform Download
	res = curl_easy_perform(curlHandle);

	// Check For Errors During Download
	if (res != CURLE_OK)
	{
		outFile.close();
		curl_easy_cleanup(curlHandle);
		curlHandle = nullptr;
		jenova::Error("Jenova Package Downloader", curl_easy_strerror(res));
		return false;
	}

	// Clean Up CURL
	outFile.close();
	curl_easy_cleanup(curlHandle);
	curl_global_cleanup();
	curlHandle = nullptr;

	// All Good
	return true;
}
bool JenovaPackageManager::InstallPackage(const String& packageHash)
{
	// Get Package
	FormatStatus("#eb3458", "Getting Package Information...");
	jenova::JenovaPackage package = GetOnlinePackage(packageHash);

	// Validate Package
	if (package.pkgName == "[NotFound]") return false;

	// Get Package URL
	String packageFileURL = package.pkgURL;

	// Get Package Install Path
	String installPath = ProjectSettings::get_singleton()->globalize_path(package.pkgDestination);

	// Get Package Download File Path
	String downloadFilePath = installPath + "/" + packageFileURL.get_file();

	// Create Destination If Not Exist
	if (!std::filesystem::exists(AS_STD_STRING(installPath)))
	{
		if (!std::filesystem::create_directories(AS_STD_STRING(installPath)))
		{
			jenova::Error("Jenova Package Installer", "Failed to Create Destination Directory.");
			return false;
		};
	}

	// Verbose
	jenova::VerboseByID(__LINE__, "Downloading Package from [%s] to [%s]", AS_C_STRING(packageFileURL), AS_C_STRING(downloadFilePath));

	// Download Package
	FormatStatus("#eb3458", "Downloading Package %s...", AS_C_STRING(package.pkgName));
	std::this_thread::sleep_for(std::chrono::seconds(1));
	if (!DownloadPackage(packageFileURL, downloadFilePath))
	{
		jenova::Error("Jenova Package Installer", "Failed to Download Requested Package.");
		return false;
	}

	// Verfity Package [MD5 Check]
	FormatStatus("#9d49de", "Verifying Package %s...", AS_C_STRING(packageFileURL.get_file()));
	std::this_thread::sleep_for(std::chrono::seconds(1));
	if (jenova::GenerateMD5HashFromFile(downloadFilePath) != package.pkgHash)
	{
		jenova::Warning("Jenova Package Installer", "Failed to Verify Package %s", AS_C_STRING(packageFileURL.get_file()));
	}

	// Extract Package
	FormatStatus("#eb3458", "Installing Package %s...", AS_C_STRING(packageFileURL.get_file()));
	if (!ExtractPackage(downloadFilePath, installPath))
	{
		jenova::Error("Jenova Package Installer", "Failed to Extract Requested Package, Make Sure You Have Enough Space.");
		return false;
	}

	// Delete Package
	if (!std::filesystem::remove(AS_STD_STRING(downloadFilePath)))
	{
		jenova::Error("Jenova Package Installer", "Failed to Remove Cached Package.");
		return false;
	};

	// Execute Install Script if Exists
	if (package.pkgInstallScript)
	{
		FormatStatus("#fa205a", "Executing Package Installer Script...");
		std::string installerScriptPath = AS_STD_STRING(installPath) + "/" + "Install.jnvscript";
		if (!jenova::ExecutePackageScript(installerScriptPath))
		{
			jenova::Error("Jenova Package Installer", "Install Script Failed to Execute.");
			return false;
		}
	}

	// Update Installed Package Database
	installedPackages.push_back(package);

	// Cache Installed Package Database
	if (!CacheInstalledPackages())
	{
		jenova::Error("Jenova Package Installer", "Failed to Cache Installed Packages Database.");
		return false;
	}

	// Reload Installed Packages
	if (!ObtainInstalledPackages())
	{
		jenova::Error("Jenova Package Installer", "Failed to Reload Installed Packages Database.");
		return false;
	}

	// Refresh Package List
	this->call_deferred("ForceUpdatePackageList");

	// Update Status
	this->FormatStatus("#2edb76", "Package '%s' Installed.", AS_C_STRING(package.pkgName));

	// All Good
	return true;
}
bool JenovaPackageManager::UninstallPackage(const String& packageHash)
{
	// Get Package
	FormatStatus("#eb3458", "Getting Package Information...");
	jenova::JenovaPackage package = GetOnlinePackage(packageHash);

	// Validate Package
	if (package.pkgName == "[NotFound]") return false;

	// Get Package URL
	std::string installPath = AS_STD_STRING(ProjectSettings::get_singleton()->globalize_path(package.pkgDestination));

	// Execute Uninstall Script if Exists
	if (package.pkgUninstallScript)
	{
		FormatStatus("#fa205a", "Executing Package Uninstaller Script...");
		std::string uninstallerScriptPath = installPath + "/" + "Uninstall.jnvscript";
		if (!jenova::ExecutePackageScript(uninstallerScriptPath))
		{
			jenova::Error("Jenova Package Uninstaller", "Uninstall Script Failed to Execute.");
			return false;
		}
	}

	// Delete Install Path Directory If Exists
	if (std::filesystem::exists(installPath))
	{
		if (!std::filesystem::is_empty(installPath))
		{
			// Delete Files And Directories
			try
			{
				for (const auto& entry : std::filesystem::directory_iterator(installPath))
				{
					if (entry.is_regular_file())
					{
						std::filesystem::remove(entry.path());
						FormatStatus("#e09334", "File ([color=#e04234]%s[/color]) Deleted.", entry.path().filename().string().c_str());
					}
					else if (entry.is_directory())
					{
						std::filesystem::remove_all(entry.path());
						FormatStatus("#e09334","Directory ([color=#e04234]%s[/color]) Deleted.", entry.path().filename().string().c_str());
					}
				}
				std::filesystem::remove(installPath);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				jenova::Error("Jenova Pacakage Uninstaller", "Failed to Clean Package Data Directory.");
				return false;
			}
		}
		else
		{
			std::filesystem::remove(installPath);
		}
	}

	// Update Installed Package Database
	installedPackages.erase(std::find(installedPackages.begin(), installedPackages.end(), package));

	// Cache Installed Package Database
	if (!CacheInstalledPackages())
	{
		jenova::Error("Jenova Package Uninstaller", "Failed to Cache Installed Packages Database.");
		return false;
	}

	// Reload Installed Packages
	if (!ObtainInstalledPackages())
	{
		jenova::Error("Jenova Package Uninstaller", "Failed to Reload Installed Packages Database.");
		return false;
	}

	// Refresh Package List
	this->call_deferred("ForceUpdatePackageList");

	// Update Status
	this->FormatStatus("#2edb76", "Package '%s' Uninstalled.", AS_C_STRING(package.pkgName));

	// All Good
	return true;
}
void JenovaPackageManager::UtilizeNewPackageTask(const String& taskName, const String& packageHash, Button* utilizerButton)
{
	// Start Package Installation On New Thread
	if (taskName == "InstallPackagePrepare")
	{
		utilizerButton->set_text("    Installing    ");
		this->SetBusy(true, JenovaTaskSystem::InitiateTask([=]() { UtilizeNewPackageTask("InstallPackage", packageHash);}));
	}
	if (taskName == "UninstallPackagePrepare")
	{
		utilizerButton->set_text("    Unnstalling    ");
		this->SetBusy(true, JenovaTaskSystem::InitiateTask([=]() { UtilizeNewPackageTask("UninstallPackage", packageHash);}));
	}

	// Actual Task Operation On New Thread
	if (taskName == "InstallPackage")
	{
		if (InstallPackage(packageHash))
		{
			jenova::Output("Package [[color=#f55142]%s[/color]] Installed Successfully!", AS_C_STRING(packageHash));
		}
		else
		{
			FormatStatus("#ff1717", "Failed to Install Requested Package [%s]", AS_C_STRING(packageHash));
			jenova::Error("Jenova Package Installer", "Failed to Install Requested Package [%s]", AS_C_STRING(packageHash));
		}
	}
	if (taskName == "UninstallPackage")
	{
		if (UninstallPackage(packageHash))
		{
			jenova::Output("Package [[color=#f55142]%s[/color]] Uninstalled Successfully!", AS_C_STRING(packageHash));
		}
		else
		{
			FormatStatus("#ff1717", "Failed to Install Requested Package [%s]", AS_C_STRING(packageHash));
			jenova::Error("Jenova Package Installer", "Failed to Uninstall Requested Package [%s]", AS_C_STRING(packageHash));
		}
	}
}
bool JenovaPackageManager::UpdateStatus(const String& newStatus)
{
	if (!currentWindow || !statusLabel) return false;
	statusLabel->parse_bbcode(newStatus);
	return true;
}
void JenovaPackageManager::FormatStatus(const String& colorHash, const char* newStatus, ...)
{
	char buffer[jenova::GlobalSettings::FormatBufferSize];
	va_list args;
	va_start(args, newStatus);
	vsnprintf(buffer, sizeof(buffer), newStatus, args);
	va_end(args);
	this->call_deferred("UpdateStatus", "[color=" + String(colorHash) + "]" + String(buffer) + "[/color]");
}
void JenovaPackageManager::UpdatePackageList(const jenova::PackageType& packageType)
{
	// Validate Objects
	if (!browserContent) return;

	// Get Editor Theme
	Ref<Theme> editor_theme = EditorInterface::get_singleton()->get_editor_theme();
	if (!editor_theme.is_valid())
	{
		jenova::Error("Jenova Package Manager", "Failed to Obtain Engine Theme.");
		return;
	}

	// Clear Current Items
	for (size_t i = 0; i < browserContent->get_child_count(); i++) browserContent->get_child(i)->queue_free();

	// Create New Packages
	for (const auto& onlinePackage : onlinePackages)
	{
		if (onlinePackage.pkgType != packageType) continue;
		Panel* newPackageItem = CreatePackageItem(onlinePackage, editor_theme);
		browserContent->add_child(newPackageItem);
	}

	// Update Current Tab
	this->currentTabID = int32_t(packageType);
}
void JenovaPackageManager::ForceUpdatePackageList()
{
	this->UpdatePackageList(jenova::PackageType(currentTabID));
}
void JenovaPackageManager::SetBusy(bool busyState, jenova::TaskID taskID)
{
	this->isBusy = busyState;
	if (this->isBusy)
	{
		this->currentTaskID = taskID;
		jenova::SetWindowState(jenova::GetMainWindowNativeHandle(), false);
		taskTimer->start();
	}
	if (!this->isBusy)
	{
		this->currentTaskID = 0;
		jenova::SetWindowState(jenova::GetMainWindowNativeHandle(), true);
		taskTimer->stop();
	}
	if (this->isBusy && this->currentWindow) this->currentWindow->set_disable_input(true);
	if (!this->isBusy && this->currentWindow) this->currentWindow->set_disable_input(false);
}
bool JenovaPackageManager::CanClose() const
{
	return !this->isBusy;
}
bool JenovaPackageManager::PreparePackageManager()
{
	// Get Project Path
	String projectPath = jenova::GetJenovaProjectDirectory();

	// Create Package Directory If Not Exists
	std::string packageRepositoryPath = AS_STD_STRING(projectPath + jenova::GlobalSettings::JenovaPackageRepositoryPath);
	if (!packageRepositoryPath.empty() && (packageRepositoryPath.back() == '\\' || packageRepositoryPath.back() == '/')) packageRepositoryPath.pop_back();
	if (!std::filesystem::exists(packageRepositoryPath))
	{
		if (!std::filesystem::create_directories(packageRepositoryPath)) return false;
	}
	
	// Create Package Directory .gdignore If Not Exists
	String gdIgnoreFilePath = projectPath + jenova::GlobalSettings::JenovaPackageRepositoryPath + ".gdignore";
	if (!std::filesystem::exists(AS_STD_STRING(gdIgnoreFilePath)))
	{
		if (!jenova::WriteStringToFile(gdIgnoreFilePath, " ")) return false;
	}

	// All Good
	return true;
}
void JenovaPackageManager::PrepareForClose()
{
	currentWindow = nullptr;
	statusLabel = nullptr;
	browserContent = nullptr;
}
void JenovaPackageManager::OnTaskTimerTick()
{
	if (JenovaTaskSystem::IsTaskComplete(this->currentTaskID))
	{
		JenovaTaskSystem::ClearTask(this->currentTaskID);
		this->SetBusy(false);
	}
}

// Jenova Package Manager Download Events Implementation
size_t JenovaPackageManager::OnDownloadBufferWrite(void* bufferPtr, size_t bufferSize, size_t newBufferSize, void* streamPtr)
{
	std::fstream* outFile = static_cast<std::fstream*>(streamPtr);
	outFile->write(static_cast<char*>(bufferPtr), bufferSize * newBufferSize);
	return bufferSize * newBufferSize;
}
int JenovaPackageManager::OnDownloadProgress(void* clientPtr, size_t downloadTotal, size_t downloadNow, size_t uploadTotal, size_t uploadNow)
{
	if (downloadTotal > 0)
	{
		// Get Package Manager
		JenovaPackageManager* packageManager = static_cast<JenovaPackageManager*>(clientPtr);

		// Calculate Download Progress
		double percentage = (downloadNow * 100.0) / downloadTotal;

		// Calculate Download Speed
		std::string speedString;
		constexpr size_t sizeBase = 1000;
		curl_easy_getinfo(curlHandle, CURLINFO_SPEED_DOWNLOAD, &downloadSpeed);
		if (downloadSpeed < sizeBase) speedString += std::to_string(static_cast<int>(downloadSpeed)) + " B/s";
		else if (downloadSpeed < sizeBase * sizeBase) speedString += std::to_string(static_cast<int>(downloadSpeed / sizeBase)) + " KB/s";
		else speedString += std::to_string(static_cast<int>(downloadSpeed / (sizeBase * sizeBase))) + " MB/s";

		// Update Status
		packageManager->FormatStatus("#2688ff", "Downloading Package (%.2f%%) [%ld/%ld] At [color=#e0a134]%s[/color]", 
			percentage, downloadNow, downloadTotal, speedString.c_str());
	}
	return 0;
}
size_t JenovaPackageManager::OnFetchBufferWrite(void* contents, size_t size, size_t nmemb, std::string* buffer) 
{
	size_t totalSize = size * nmemb;
	buffer->append((char*)contents, totalSize);
	return totalSize;
}