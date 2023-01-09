#ifndef PARTICLE_EDITOR_H
#define PARTICLE_EDITOR_H

#include "resource.h"
#include "fwd.hpp"
#include "pyscript/script.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "editor_shared/app/i_editor_app.hpp"

BW_BEGIN_NAMESPACE

class App;

BEGIN_GUI_NAMESPACE
class MenuHelper;
END_GUI_NAMESPACE

class ParticleEditorApp
	: public CWinApp
	, public GUI::OptionMap
	, public IEditorApp
{
public:
    ParticleEditorApp();

    ~ParticleEditorApp();
    
	bool InitialiseMF( BW::string &openFile );
    /*virtual*/ BOOL InitInstance();

    /*virtual*/ int ExitInstance();

	/*virtual*/ int Run();

	bool setLanguage( GUI::ItemPtr item );
	unsigned int updateLanguage( GUI::ItemPtr item );
	void updateLanguageList();

    void openDirectory(BW::string const &dir, bool forceRefresh = false);

    void setFrameRate(float rate) { m_desiredFrameRate = rate; }    // No more frame limiting (Bug 4834)

    void OnDirectoryOpen();

    void OnFileSaveParticleSystem();

    enum State
    {
        PE_PLAYING,
        PE_PAUSED,
        PE_STOPPED
    };

    void setState(State state);

    State getState() const;

    void OnAppAbout();
		
    void OnAppShortcuts();

	void OnViewShowSide();

    void OnViewShowUAL();

    void OnViewShowMsgs();

    static ParticleEditorApp & instance() { return *s_instance; }

	App* mfApp() { return mfApp_; }

	CWnd* mainWnd() { return m_pMainWnd; }

protected:

    /*virtual*/ BOOL OnIdle(LONG lCount);
	BOOL InternalInitInstance();
	int InternalExitInstance();
	int InternalRun();

    DECLARE_MESSAGE_MAP()

	/*virtual*/ BW::string get(BW::string const &key) const;
	/*virtual*/ bool exist(BW::string const &key) const;
	/*virtual*/ void set(BW::string const &key, BW::string const &value);

    void update();

    typedef ParticleEditorApp This;

    PY_MODULE_STATIC_METHOD_DECLARE(py_update)
    PY_MODULE_STATIC_METHOD_DECLARE(py_particleSystem)   
    
private:

private:
    PeShell                     *m_appShell;
    PeApp                       *m_peApp;
    App                         *mfApp_;
    float                       m_desiredFrameRate;     // No more frame limiting (Bug 4834)
    State                       m_state;
    static ParticleEditorApp    *s_instance;
	std::auto_ptr< GUI::MenuHelper > menuHelper_;
};

extern ParticleEditorApp theApp;

BW_END_NAMESPACE

#endif // PARTICLE_EDITOR_H
