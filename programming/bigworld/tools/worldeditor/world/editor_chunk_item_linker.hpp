#ifndef EDITOR_CHUNK_ITEM_LINKER_HPP
#define EDITOR_CHUNK_ITEM_LINKER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_link.hpp"
#include "cstdmf/unique_id.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Used to make an object linkable in the editor, e.g. EditorChunkEntity
 *	and EditorChunkUserDataObject.  The class has been designed for use by
 *	composition.  Chunk point links are owned wholly by each instance of this
 *	class, while standard links have shared ownership with the opposing
 *	EditorChunkItemLinkable instance.
 */
class EditorChunkItemLinkable
{
public:
	// Link structure
	struct Link
	{
		Link(UniqueID UID, BW::string CID);

		bool operator<( const Link& rhs ) const;

		UniqueID	UID_;
		BW::string	CID_;
	};

	// Linking typedefs
	typedef BW::set<Link>				Links;
	typedef Links::iterator				LinksIter;
	typedef Links::const_iterator		LinksConstIter;

    typedef BW::map<UniqueID, bool>    LinkInfo;
    typedef LinkInfo::const_iterator    LinkInfoConstIter;

private:
	// The owner of this linker
	ChunkItem*							chunkItem_;

	// Linking variables
	BW::vector<ChunkLinkPtr>			links_;
	BW::map<BW::string,ChunkLinkPtr>	chunkPointLinks_;  // ( chunkId, ChunkLink )
    LinkInfo							preloadLinks_;
	Links								backLinks_;

	// References to custom data
	UniqueID							guid_;
	PropertiesHelper*					propHelper_;


public:
	// Constructor
	EditorChunkItemLinkable(
		ChunkItem* chunkItem, UniqueID guid, PropertiesHelper* propHelper);

	// Destructor
	~EditorChunkItemLinkable();

	ChunkItem* chunkItem() const{ return chunkItem_; };

	void tossAdd();
	void tossRemove();
	void deleted();

	void clearAllLinks();

	BW::string getOutsideChunkId() const;

	UniqueID guid() const;
	void guid(UniqueID guid);

	PropertiesHelper* propHelper() const{ return propHelper_; };
	void propHelper(PropertiesHelper* propHelper) { propHelper_ = propHelper; };

	bool linkedChunksWriteable();

	// Chunk link methods
	void createChunkLink( const BW::string& chunkId );
	void removeChunkLink(const BW::string& chunkId);
	void removeChunkLinks();
	void updateChunkLinks();

	// Back links
	void loadBackLinks(DataSectionPtr pSection);
	void saveBackLinks(DataSectionPtr pSection);
	void addBackLink(EditorChunkItemLinkable* ecudo);
	void addBackLink(const UniqueID& guid, const BW::string& chunkId);
	void removeBackLink(EditorChunkItemLinkable* ecudo);
	void removeBackLink(const UniqueID& guid, const BW::string& chunkId);
	void removeAllBackLinks();
	LinksConstIter getBackLinksBegin();
	LinksConstIter getBackLinksEnd();
	size_t getBackLinksCount() const;
	bool hasLinksTo(const UniqueID& guid, const BW::string& chunkId);

	// Standard link methods
	ChunkLinkPtr createLink(
		ChunkLink::Direction, EditorChunkItemLinkable* other);
	void unlink();
	bool isLinkedTo(const UniqueID& other) const;
	void removeLink(EditorChunkItemLinkable* other);
	ChunkLinkPtr setLink(EditorChunkItemLinkable* other, bool canTraverse);
	void delLink(ChunkLinkPtr link);
	ChunkLinkPtr findLink(EditorChunkItemLinkable const *other) const;
	virtual void removeLink(ChunkLinkPtr link);
	virtual ChunkLinkPtr createLink() const;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_ITEM_LINKER_HPP
