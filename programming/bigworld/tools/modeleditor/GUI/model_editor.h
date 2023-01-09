// ModelEditor.h : main header file for the ModelEditor application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "me_shell.hpp"
#include "me_app.hpp"
#include "mru.hpp"
#include "me_python_adapter.hpp"

#include "appmgr/app.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"

BW_BEGIN_NAMESPACE

BEGIN_GUI_NAMESPACE
class MenuHelper;
END_GUI_NAMESPACE

class UalItemInfo;
class PageLights;

class CModelEditorApp
	: public CWinApp
	, GUI::OptionMap
	, public IModelEditorApp
{
public:
	static DogWatch s_updateWatch;
	
	CModelEditorApp();
	virtual ~CModelEditorApp();

	static UINT loadErrorMsg( LPVOID lpvParam );

	CWnd* mainWnd() { return m_pMainWnd; }
	App* mfApp() { return mfApp_; }
	bool initDone() { return initDone_; };

	bool loadFile( UalItemInfo* item );

	const BW::string& modelToLoad() { return modelToLoad_; }
	void modelToLoad( const BW::string& modelName ) { modelToLoad_ = modelName; }

	void loadLights( const char* lightsName );

	//The menu commands
	void OnFileOpen();
	void OnFileAdd();
	void OnFileReloadTextures();
	void OnFileRegenBoundingBox();
	void OnAppPrefs();
	void exit( bool ignoreChanges = false );

	void updateRecentList( const BW::string& kind );

	void updateLanguageList();

	MEPythonAdapter* pythonAdapter() const;

// Overrides
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual int ExitInstance(); 
	virtual int Run();

private:
	App* mfApp_;
	bool initDone_;

	MeShell*	meShell_;
	MeApp*		meApp_;

	PageLights* lightPage_;

	MEPythonAdapter* pPythonAdapter_;

	std::auto_ptr< GUI::MenuHelper > menuHelper_;

	BOOL parseCommandLineMF();
		
	virtual BW::string get( const BW::string& key ) const;
	virtual bool exist( const BW::string& key ) const;
	virtual void set( const BW::string& key, const BW::string& value );

	BOOL InternalInitInstance();
	int InternalExitInstance();
	int InternalRun();

	IMainFrame * getMainFrame();

	DECLARE_MESSAGE_MAP()
};

extern CModelEditorApp theApp;
BW_END_NAMESPACE

