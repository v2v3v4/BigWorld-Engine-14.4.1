#ifndef GL_VIEW_H
#define GL_VIEW_H

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class IModelEditorApp;

class GLView
{
public:
	GLView();
	void setEditorApp( IModelEditorApp * editorApp );
	virtual bool underMouse() const { return false; }
	virtual void * getNativeId() { return 0; }

protected:
	bool paint( bool resize );
	virtual void resizeWindow();

	IModelEditorApp * editorApp_;

};

BW_END_NAMESPACE

#endif //GL_VIEW_H