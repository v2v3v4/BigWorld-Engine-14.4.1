#ifndef LIGHT_CONTAINER_DEBUGGER_HPP
#define LIGHT_CONTAINER_DEBUGGER_HPP

#include "editor_renderable.hpp"
#include "chunk/chunk_item.hpp"

BW_BEGIN_NAMESPACE

class LightContainerDebugger : public EditorRenderable
{
public:
	static LightContainerDebugger* instance();
	static void fini();

	void	setItems( const BW::vector<ChunkItemPtr>& items ) { items_ = items; }
	size_t	numItems() const { return items_.size(); }
	void	clearItems() { items_.clear(); }

	void render();

private:
	static SmartPointer<LightContainerDebugger> s_instance_;
	LightContainerDebugger();	

	BW::vector<ChunkItemPtr> items_;
};

BW_END_NAMESPACE

#endif
