#ifndef EDITOR_CHUNK_USER_DATA_OBJECT_LINK
#define EDITOR_CHUNK_USER_DATA_OBJECT_LINK


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkUserDataObjectLink : public EditorChunkLink
{
	DECLARE_EDITOR_CHUNK_ITEM(EditorChunkUserDataObjectLink)

public:
	virtual BW::vector<BW::string> edCommand(
		BW::string const &path ) const;

    virtual bool edExecuteCommand(
		BW::string const &path, BW::vector<BW::string>::size_type index );

	virtual float collide(
		const Vector3& source, const Vector3& dir, WorldTriangle& wt ) const;

private:
	void deleteCommand();
};


typedef SmartPointer<EditorChunkUserDataObjectLink>   EditorChunkUserDataObjectLinkPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_USER_DATA_OBJECT_LINK
