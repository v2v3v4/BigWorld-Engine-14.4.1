#ifndef WORLD_EDITOR_APP_HPP
#define WORLD_EDITOR_APP_HPP

#include "editor_shared/app/i_editor_app.hpp"
#include "guimanager/gui_forward.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"

BW_BEGIN_NAMESPACE

BEGIN_GUI_NAMESPACE
class MenuHelper;
END_GUI_NAMESPACE

class AppFunctor;
typedef SmartPointer<AppFunctor> AppFunctorPtr;
class WorldManager;


class WorldEditorApp
	: public CWinApp
	, public IEditorApp
{
public:
	static WorldEditorApp & instance() { return *s_instance_; }

	WorldEditorApp();
	~WorldEditorApp();

	CWnd* mainWnd() { return m_pMainWnd; }
	App* mfApp() { return s_mfApp; }

	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual int Run();

	virtual BOOL OnIdle(LONG lCount);

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	WEPythonAdapter * pythonAdapter() const;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL parseCommandLineMF();

private:
	BOOL InternalInitInstance();
	int InternalExitInstance();
	int InternalRun();

	static WorldEditorApp * s_instance_;
	App					  * s_mfApp;
	WEPythonAdapter		  * pPythonAdapter_;
	HANDLE					updateMailSlot_;
	AppFunctorPtr			pAppFunctor_;
	LPCTSTR					oldAppName_;

	typedef std::auto_ptr< WorldManager > WorldManagerPtr;
	WorldManagerPtr		    pWorldManager_;

	std::auto_ptr< GUI::MenuHelper > menuHelper_;
};


extern WorldEditorApp theApp;

BW_END_NAMESPACE

#endif // WORLD_EDITOR_APP_HPP
