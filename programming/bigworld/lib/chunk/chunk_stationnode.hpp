#ifndef CHUNK_STATIONNODE_HPP
#define CHUNK_STATIONNODE_HPP

#include "cstdmf/unique_id.hpp"
#include "chunk_item.hpp"
#include "chunk/station_graph.hpp"
#include "chunk/chunk_link.hpp"


BW_BEGIN_NAMESPACE

/**
 *	A node in a station graph
 */
class ChunkStationNode : public ChunkItem
{
public:
    typedef BW::map<UniqueID, bool>    LinkInfo;
    typedef LinkInfo::const_iterator    LinkInfoConstIter;

	ChunkStationNode();
	~ChunkStationNode();

	bool load( DataSectionPtr pSection, Chunk* pChunk );

	UniqueID			id() const;
	StationGraph*		graph() const;
	Vector3				position() const;
    BW::string         const &userString() const;

    void                id(UniqueID const &newid);
    void                graph(StationGraph *g);
    void                position(Vector3 const &pos);
    void                userString(BW::string const &str);

    LinkInfoConstIter   beginLinks() const;
    LinkInfoConstIter   endLinks() const;
	LinkInfoConstIter	findLink(UniqueID const &id) const;
    size_t              numberLinks() const;

#ifdef EDITOR_ENABLED

    void                removeLink(UniqueID const &other);

#endif

    bool                canTraverse(UniqueID const &other) const;
    bool                isLinkedTo(UniqueID const &other) const;

    /*virtual*/ void    toss(Chunk *chunk);

	ChunkLinkPtr setLink(ChunkStationNode *other, bool canTraverse);

#ifdef EDITOR_ENABLED
    virtual bool isValid(BW::string &failureMsg) const;

    virtual void makeDirty();
#endif

protected:
	virtual bool loadName( DataSectionPtr pSection, Chunk * pChunk );

    virtual ChunkLinkPtr createLink() const;

#ifdef EDITOR_ENABLED

    void unlink();

#endif

    void removeLink(ChunkStationNode *other);    

    ChunkLinkPtr findLink(ChunkStationNode const *other) const;

    ChunkLinkPtr getChunkLink(size_t idx) const;

    void beginMove();

    void endMove();

    void updateRegistration( Chunk *chunk );


private:
    void removeLink(ChunkLinkPtr link);

    void delLink(ChunkLinkPtr link);

	static ChunkItemFactory::Result create( Chunk * pChunk, DataSectionPtr pSection );

	static ChunkItemFactory	factory_;

private:
	Vector3				        position_;
	UniqueID			        id_;
	StationGraph                *graph_;
    BW::string                 userString_;
    BW::vector<ChunkLinkPtr>   links_;
    LinkInfo                    preloadLinks_;
    size_t                      moveCnt_;
};

BW_END_NAMESPACE

#endif // CHUNK_STATIONNODE_HPP
