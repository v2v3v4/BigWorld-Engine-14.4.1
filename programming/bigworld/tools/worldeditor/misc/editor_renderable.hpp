#ifndef EDITOR_RENDERABLE_HPP
#define EDITOR_RENDERABLE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class EditorRenderable: public ReferenceCount
{
public:
	virtual ~EditorRenderable() {};

	virtual void render() = 0;
};

BW_END_NAMESPACE

#endif // EDITOR_RENDERABLE_HPP
