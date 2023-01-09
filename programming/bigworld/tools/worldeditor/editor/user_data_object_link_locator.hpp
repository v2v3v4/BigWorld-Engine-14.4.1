#ifndef USER_DATA_OBJECT_LINK_LOCATOR_HPP
#define USER_DATA_OBJECT_LINK_LOCATOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool_locator.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class UserDataObjectLinkLocator : public ToolLocator
{
public:
    enum Type
    {
        LOCATE_USER_DATA_OBJECTS   = 1,
        LOCATE_ENTITIES = 2,
        LOCATE_BOTH     = LOCATE_USER_DATA_OBJECTS | LOCATE_ENTITIES
    };

	explicit UserDataObjectLinkLocator(const BW::string& linkName, Type type = LOCATE_BOTH);

    /*virtual*/ ~UserDataObjectLinkLocator();

    /*virtual*/ void calculatePosition(Vector3 const &worldRay, Tool &tool);

    ChunkItemPtr chunkItem();

private:
    UserDataObjectLinkLocator(UserDataObjectLinkLocator const &);
    UserDataObjectLinkLocator &operator=(UserDataObjectLinkLocator const &);

private:
    ChunkItemPtr        chunkItem_;
    bool                initialPos_;
    Vector3             lastLocatorPos_;
    Vector3             totalLocatorOffset_;
    ToolLocatorPtr      subLocator_;
    Type                type_;
	BW::string			linkName_;
};

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_LINK_LOCATOR_HPP
