#ifndef CONFIG_DIALOG_HPP
#define CONFIG_DIALOG_HPP

#include "wtl.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class AssetCompiler;
class PluginLoader;

class ConfigDialog : 
	public ATL::CDialogImpl<ConfigDialog>,
	public WTL::CDialogResize<ConfigDialog>
{
public:
	enum { IDD = IDR_CONFIG_DIALOG };

public:
	explicit ConfigDialog( const AssetCompiler & compiler, const PluginLoader & loader );

	void run();

	BEGIN_MSG_MAP_EX(ConfigDialog)
		MSG_WM_INITDIALOG(onInitDialog)
		MSG_WM_CLOSE(onClose)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, onOk)
		CHAIN_MSG_MAP(WTL::CDialogResize<ConfigDialog>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(ConfigDialog)
		DLGRESIZE_CONTROL(IDC_CONFIG_TEXT, DLSZ_SIZE_X|DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X|DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

private:

	// Message Map Functions
	int onInitDialog(ATL::CWindow handle, LPARAM data);
	void onClose();
	void onOk(UINT notifyCode, int controlID, CWindow handle);

	const BW::string & presentBoolean( bool value ) const;
	const BW::string & presentPath( const BW::string & path ) const;
	BW::string generateConfigString() const;

private:

	const AssetCompiler & compiler_;
	const PluginLoader & loader_;

	WTL::CEdit textbox_;
};

BW_END_NAMESPACE

#endif // CONFIG_DIALOG_HPP
