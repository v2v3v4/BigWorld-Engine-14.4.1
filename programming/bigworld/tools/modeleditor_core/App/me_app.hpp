#ifndef ME_APP_HPP
#define ME_APP_HPP

#include "tools/common/floor.hpp"

#include "moo/light_container.hpp"

#include "tools/common/tools_camera.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"
#include "cstdmf/singleton_manager.hpp"

BW_BEGIN_NAMESPACE
class IMainFrame;
class Lights;
class Mutant;

class MeApp
{
public:

	MeApp( IMainFrame * mainFrame, IModelEditorApp * editorApp );
	~MeApp();

	void initCamera();
	
	static MeApp & instance() { SINGLETON_MANAGER_WRAPPER( MeApp ) MF_ASSERT(s_instance_); return *s_instance_; }
	
	Floor*	floor();
	Mutant*	mutant();
	Lights*	lights();
	ToolsCameraPtr camera();

	Moo::LightContainerPtr blackLight() { return blackLight_; }
	Moo::LightContainerPtr whiteLight() { return whiteLight_; }

	void saveModel();
	void saveModelAs();
	bool canExit( bool quitting );
	void forceClean();
	bool isDirty() const;
private:

	static MeApp *		s_instance_;

	Floor*	floor_;
	Mutant*	mutant_;
	Lights*  lights_;

	Moo::LightContainerPtr blackLight_;
	Moo::LightContainerPtr whiteLight_;

	ToolsCameraPtr		camera_;	
	IModelEditorApp *	editorApp_;
	IMainFrame *		mainFrame_;
};

BW_END_NAMESPACE


#endif // ME_APP_HPP