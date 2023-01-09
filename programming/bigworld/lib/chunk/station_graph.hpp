#ifndef STATION_GRAPH_HPP
#define STATION_GRAPH_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/unique_id.hpp"
#include "math/vector3.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkStationNode;
class GeometryMapping;
class SimpleMutex;

/**
 *  A graph of nodes
 */
class StationGraph
{
public:
	static StationGraph* getGraph( const UniqueID& graphName );
	static BW::vector<StationGraph*> getAllGraphs();

	const UniqueID &    name() const;
	bool                isReady() const;

	bool   canTraverseFrom( const UniqueID& src, const UniqueID& dst );
	uint32 traversableNodes( const UniqueID& src, BW::vector<UniqueID>& retNodeIDs );
	bool   worldPosition( const UniqueID& node, Vector3& retWorldPos );
	const  UniqueID& nearestNode( const Vector3& worldPos );	

	void   registerNode( ChunkStationNode* node, Chunk* pChunk );
	void   deregisterNode( ChunkStationNode* node );
#ifdef EDITOR_ENABLED
	bool   isRegistered( const UniqueID& id );
	bool   isRegistered( ChunkStationNode* node );
#endif

	/**
	 *	EDITOR interface.  The editor can create/delete nodes interactively.
	 *	There is one ChunkStationNode for every StationGraph::Node, whereby
	 *	the ChunkStationNodes are ChunkItems that can be moved around etc.
	 *	The StationGraph::Nodes contain information that will be used at
	 *	runtime by the server, and possibly the client.
	 */
#ifdef EDITOR_ENABLED
	ChunkStationNode* getNode( const UniqueID& id );

	BW::vector<ChunkStationNode*> getAllNodes();

	static ChunkStationNode* getNode( const UniqueID& graphName, 
        const UniqueID& id );

    static void mergeGraphs( UniqueID const &graph1Id, 
        UniqueID const &graph2Id, GeometryMapping * pMapping );

    void loadAllChunks( GeometryMapping * pMapping );

	bool save();

	static void saveAll();

    virtual bool isValid(BW::string &failureMsg) const;

	bool updateNodeIds(const UniqueID &nodeId, const BW::vector<UniqueID> &links);
#endif	//EDITOR_ENABLED

private:
	class Node
	{
	public:
		Node();
		bool						load( DataSectionPtr pSect, const Matrix& mapping );
#ifdef EDITOR_ENABLED
		///only the editor needs the code to save StationGraph::Nodes
		bool						save( DataSectionPtr pSect );
#endif
		bool						hasTraversableLinkTo( const UniqueID& nodeId ) const;

		void						addLink( const UniqueID& id );
		void						delLink( const UniqueID& id );
		BW::vector<UniqueID>&		links();

		Vector3						worldPosition_;
		UniqueID					id_;

        BW::string                 userString_;

	private:
		BW::vector<UniqueID>		links_;
	};

	StationGraph( const UniqueID& name );

	void                        addNode ( const StationGraph::Node& n );
	StationGraph::Node*         node( const UniqueID& id );
	void                        constructFilename( const BW::string& spacePath );
    bool                        load( const Matrix& mapping );

	BW::string			                        filename_;
	UniqueID			                        name_;
	DataSectionPtr		                        pSect_;
	bool                                        isReady_;	    /** false until loaded */
	BW::map<UniqueID, StationGraph::Node>      nodes_;         /** client / server nodes list */
#ifdef EDITOR_ENABLED	
	BW::map<UniqueID, ChunkStationNode*>       edNodes_;       /** Maps loaded node ids to editor nodes */
#endif
	static BW::map<UniqueID, StationGraph*>    graphs_;        /** Cache of StationGraphs */
};

BW_END_NAMESPACE

#endif	//STATION_GRAPH_HPP
