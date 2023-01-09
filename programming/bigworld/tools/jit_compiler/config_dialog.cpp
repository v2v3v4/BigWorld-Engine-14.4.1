#include "config_dialog.hpp"

#include "asset_pipeline/conversion/content_addressable_cache.hpp"
#include "asset_pipeline/compiler/asset_compiler.hpp"
#include "plugin_system/plugin_loader.hpp"

#include "resmgr/bwresource.hpp"
#include "cstdmf/bw_util.hpp"

BW_BEGIN_NAMESPACE

ConfigDialog::ConfigDialog( const AssetCompiler & compiler, const PluginLoader & loader )
	: compiler_(compiler)
	, loader_(loader)
{
}

void ConfigDialog::run()
{
	DoModal();
}

const BW::string & ConfigDialog::presentBoolean( bool value ) const
{
	static BW::string trueString = "true";
	static BW::string falseString = "false";
	return value ? trueString : falseString;
}

const BW::string & ConfigDialog::presentPath( const BW::string & path ) const
{
	static BW::string emptyString = "(empty)";
	if (path.empty())
	{
		return emptyString;
	}

	return path;
}

BW::string ConfigDialog::generateConfigString() const
{
	BW::string text;

	// Executable Path
	text.append("Executable:\r\n");
	text.append( presentPath(BWUtil::executablePath()) ).append("\r\n");
	text.append("\r\n");

	// Working Directory
	text.append("Current Working Directory:\r\n");
	text.append( presentPath(BWResource::getCurrentDirectory()) ).append("\r\n");
	text.append("\r\n");

	// Input Paths
	text.append("Input Paths:\r\n");
	for (int i = 0; i < BWResource::getPathNum(); ++i)
	{
		text.append( presentPath(BWResource::getPath(i)) ).append("\r\n");
	}
	text.append("\r\n");

	// Intermediate Path
	text.append("Intermediate Path:\r\n");
	text.append( presentPath(compiler_.getIntermediatePath()) ).append("\r\n");
	text.append("\r\n");

	// Cache Path
	text.append("Cache Path:\r\n");
	text.append( presentPath(ContentAddressableCache::getCachePath()) ).append("\r\n");
	if (ContentAddressableCache::getReadFromCache())
	{
		text.append("Readable\r\n");
	}
	if (ContentAddressableCache::getWriteToCache())
	{
		text.append("Writable\r\n");
	}
	text.append("\r\n");

	// Output Path
	text.append("Output Path:\r\n");
	text.append( presentPath(compiler_.getOutputPath()) ).append("\r\n");
	text.append("\r\n");

	// Thread Count
	text.append("Threads: ").append(std::to_string(compiler_.getThreadCount()).c_str()).append("\r\n");

	// Force Rebuild
	text.append("Force Rebuild: ").append(presentBoolean(compiler_.getForceRebuild())).append("\r\n");

	// Recursive
	text.append("Recursive: ").append(presentBoolean(compiler_.getRecursive())).append("\r\n");

	text.append("\r\n");

	// Plugins
	text.append("Plugins Loaded:\r\n");
	auto pluginNames = loader_.pluginNames();
	for (auto plugin : pluginNames)
	{
		text.append(plugin).append("\r\n");
	}

	return text;
}

int ConfigDialog::onInitDialog(ATL::CWindow handle, LPARAM data)
{
	DlgResize_Init();

	textbox_.Attach(GetDlgItem(IDC_CONFIG_TEXT));

	BW::wstring configText(bw_utf8tow(generateConfigString()));
	textbox_.SetWindowTextW(configText.c_str());

	return FALSE;
}

void ConfigDialog::onClose()
{
	EndDialog(IDOK);
}

void ConfigDialog::onOk(UINT notifyCode, int controlID, CWindow handle)
{
	EndDialog(IDOK);
}

BW_END_NAMESPACE
