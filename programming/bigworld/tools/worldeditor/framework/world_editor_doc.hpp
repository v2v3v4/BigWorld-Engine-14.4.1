#ifndef WORLD_EDITOR_DOC_HPP
#define WORLD_EDITOR_DOC_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class WorldEditorDoc : public CDocument
{
protected: 
	DECLARE_DYNCREATE(WorldEditorDoc)

	WorldEditorDoc();
	
public:
	static WorldEditorDoc& instance();

	virtual ~WorldEditorDoc();

	virtual BOOL OnNewDocument();
	virtual void OnCloseDocument();

protected:
	DECLARE_MESSAGE_MAP()

private:
	static WorldEditorDoc	*s_instance_;
};

BW_END_NAMESPACE

#endif // WORLD_EDITOR_DOC_HPP
