#ifndef STATION_NODE_LINK_PROXY_HPP
#define STATION_NODE_LINK_PROXY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is a proxy that implements linking and testing of linking between
 *  EditorChunkStationNode's.
 */
class StationNodeLinkProxy : public LinkProxy
{
public:
    explicit StationNodeLinkProxy
    (
        EditorChunkStationNode      &node
    );

    /*virtual*/ ~StationNodeLinkProxy();

    /*virtual*/ LinkType linkType() const;

    /*virtual*/ MatrixProxyPtr createCopyForLink();

    /*virtual*/ TargetState canLinkAtPos(ToolLocatorPtr locator) const;

    /*virtual*/ void createLinkAtPos(ToolLocatorPtr locator);

    /*virtual*/ ToolLocatorPtr createLocator() const;

private:
    StationNodeLinkProxy(StationNodeLinkProxy const &);
    StationNodeLinkProxy &operator=(StationNodeLinkProxy const &);

	void relinkEntities( StationGraph* oldGraph, StationGraph* newGraph );
	void relinkEntitiesInChunk( ChunkStationNode* node,
								StationGraph* graph,
								BW::string& chunkID );
private:
    EditorChunkStationNode          *node_;
};

BW_END_NAMESPACE

#endif // STATION_NODE_LINK_PROXY_HPP
