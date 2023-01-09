#ifndef I_EDITOR_APP_H
#define I_EDITOR_APP_H

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class IEditorApp
{
public:
	virtual bool isMinimized();
	virtual void onIdle() {};
};

BW_END_NAMESPACE

#endif //EDITOR_APP_H