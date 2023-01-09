#ifndef I_MAIN_FRAME_H
#define I_MAIN_FRAME_H

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class GLView;
class Vector2;
class Vector3;

namespace GUI
{
	class IMenuHelper;
}

class IMainFrame
{
public:
	virtual ~IMainFrame() {}

	virtual GLView * getEditorView() { return NULL; }
	virtual GUI::IMenuHelper * getMenuHelper() { return NULL;}

	virtual void setMessageText( const wchar_t * pText ) = 0;
	virtual void setStatusText( UINT id, const wchar_t * text ) = 0;
	virtual bool cursorOverGraphicsWnd() const = 0;
	virtual void updateGUI( bool force = false ) = 0;
	virtual Vector2 currentCursorPosition() const = 0;
	virtual Vector3 getWorldRay(int x, int y) const = 0;
	virtual void grabFocus() = 0;

	virtual void * getNativePointer() { return NULL; }

};

BW_END_NAMESPACE

#endif //I_MAIN_FRAME_H