#ifndef EDITOR_CHUNK_LINK_MANAGER
#define EDITOR_CHUNK_LINK_MANAGER


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Used to control when links regenerate their geometry so as not to
 *	penetrate through terrain unless passing through a terrain hole.
 *	Users have the ability to control how often link geometry is regenerated
 *	by altering the family of render/links options in the WorldEditor
 *	options.xml.  See file_grammar_guide.pdf in the documents folder for
 *	details.
 */
class EditorChunkLinkManager
{
public:
	typedef BW::set<EditorChunkLink*>::iterator	RegLinksIt;

private:
	bool											valid_;

	BW::set<EditorChunkLink*>						registeredLinks_;
	SimpleMutex										regLinksMutex_;

	bool											recalcComplete_;
	bool											recalcTimerStarted_;
	int												recalcsRemaining_;
    double											totalRecalcWaitTime_;
    double											recalcWaitTime_;

public:
	~EditorChunkLinkManager();

	// Accessors to singleton instance
	static EditorChunkLinkManager* instancePtr();
	static EditorChunkLinkManager& instance();

	bool isValid();
	void setValid(bool valid);

	void registerLink(EditorChunkLink* pEcl);
	void unregisterLink(EditorChunkLink* pEcl);

	void chunkLoaded(Chunk* pChunk);
	void chunkTossed(Chunk* pChunk);

	void coveringLoadedChunk();
	void update( float dTime );
	bool canRecalc();

private:
	// Private constructor
	EditorChunkLinkManager();

	// Private copy constructor and assignment operator
	EditorChunkLinkManager(const EditorChunkLinkManager& eclm);
	EditorChunkLinkManager& operator=(const EditorChunkLinkManager& eclm);
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_LINK_MANAGER
