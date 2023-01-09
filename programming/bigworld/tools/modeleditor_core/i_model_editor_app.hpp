#ifndef I_MODEL_EDITOR_APP_HPP
#define I_MODEL_EDITOR_APP_HPP

#include "appmgr/app.hpp"
#include "editor_shared/app/i_editor_app.hpp"

class CWnd;

BW_BEGIN_NAMESPACE

class MEPythonAdapter;

class IModelEditorApp
	: public IEditorApp
{
public:
	IModelEditorApp();
	virtual bool initDone() = 0;
	virtual App * mfApp() = 0;
	virtual CWnd * mainWnd() = 0;
	virtual const BW::string & modelToLoad() = 0;
	virtual void modelToLoad( const BW::string & modelName ) = 0;
	virtual void updateRecentList( const BW::string& kind ) = 0;
	virtual MEPythonAdapter* pythonAdapter() const = 0;
	virtual void OnFileRegenBoundingBox() = 0;

	//Interface for python calls
	void loadModel( const char* modelName );
	void addModel( const char* modelName );

	void OnFileReloadTextures();

	virtual void loadLights( const char* lightName ) = 0;
	virtual void OnAppPrefs() = 0;
	void OnFileOpen();
	virtual void OnFileAdd() = 0;
	virtual void exit( bool ignoreChanges = false ) = 0;

protected:
	BW::string modelToLoad_;
	BW::string modelToAdd_;
};

BW_END_NAMESPACE

#endif //I_MODEL_EDITOR_APP_HPP