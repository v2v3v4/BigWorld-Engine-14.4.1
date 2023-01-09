#ifndef EDITOR_TICKABLE_HPP
#define EDITOR_TICKABLE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class EditorTickable: public ReferenceCount
{
public:
	virtual ~EditorTickable() {};

	virtual void tick() = 0;
};


typedef SmartPointer<EditorTickable> EditorTickablePtr;

BW_END_NAMESPACE

#endif // EDITOR_TICKABLE_HPP
