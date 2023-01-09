#ifndef USER_DATA_OBJECT_LINK_PROXY_HPP
#define USER_DATA_OBJECT_LINK_PROXY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/editor/link_proxy.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE
/**
 *  This is a proxy that implements linking and testing of linking between two
 *  EditorUserDataObjects
 */
class UserDataObjectLinkProxy : public LinkProxy
{
public:
    explicit UserDataObjectLinkProxy
    (
	    const BW::string&		linkName,
		EditorChunkItemLinkable*	linker
    );

    /*virtual*/ ~UserDataObjectLinkProxy();

    /*virtual*/ LinkType linkType() const;

    /*virtual*/ MatrixProxyPtr createCopyForLink();

    /*virtual*/ TargetState canLinkAtPos(ToolLocatorPtr locator) const;

    /*virtual*/ void createLinkAtPos(ToolLocatorPtr locator);

    /*virtual*/ ToolLocatorPtr createLocator() const;
	

private:
    UserDataObjectLinkProxy( UserDataObjectLinkProxy const & );
    UserDataObjectLinkProxy &operator=( UserDataObjectLinkProxy const & );

private:
    EditorChunkItemLinkable*		linker_;
	BW::string					linkName_;
};

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_LINK_PROXY_HPP
