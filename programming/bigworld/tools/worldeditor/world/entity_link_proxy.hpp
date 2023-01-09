#ifndef ENTITY_LINK_PROXY_HPP
#define ENTITY_LINK_PROXY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is a proxy that implements linking and testing of linking between an
 *  EditorChunkEntity and EditorChunkStationNodes.
 */
class EntityLinkProxy : public LinkProxy
{
public:
    explicit EntityLinkProxy
    (
        EditorChunkEntity       *entity
    );

    /*virtual*/ ~EntityLinkProxy();

    /*virtual*/ LinkType linkType() const;

    /*virtual*/ MatrixProxyPtr createCopyForLink();

    /*virtual*/ TargetState canLinkAtPos(ToolLocatorPtr locator) const;

    /*virtual*/ void createLinkAtPos(ToolLocatorPtr locator);

    /*virtual*/ ToolLocatorPtr createLocator() const;

private:
    EntityLinkProxy(EntityLinkProxy const &);
    EntityLinkProxy &operator=(EntityLinkProxy const &);

private:
    EditorChunkEntityPtr        entity_;
};

BW_END_NAMESPACE

#endif // ENTITY_LINK_PROXY_HPP
