#ifndef EDITOR_CHUNK_MODEL_BASE_HPP
#define EDITOR_CHUNK_MODEL_BASE_HPP

#include "cstdmf/bw_string.hpp"

#include "chunk/chunk_model.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkModel
 */
class EditorChunkModelBase : public ChunkModel
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkModelBase )
	DECLARE_CHUNK_ITEM_ALIAS( EditorChunkModelBase, shell )

public:
	EditorChunkModelBase();

	virtual bool load( DataSectionPtr pSection, Chunk * pChunk = NULL,
		BW::string* pErrorString = NULL );

	void tossWithExtra( Chunk * pChunk, SuperModel* extraModel );

	virtual bool edSave( DataSectionPtr pSection ) { return true; };

	virtual const Matrix & edTransform() { return transform_; }

	virtual DataSectionPtr pOwnSect()	{ return pOwnSect_; }
	virtual const DataSectionPtr pOwnSect()	const { return pOwnSect_; }

protected:
	/**
	 * As extractVisuals(), but returns the names of the visuals rather than
	 * a Ptr to them.
	 */
	BW::vector<BW::string> extractVisualNames() const;

	// check if the model file is a new date then the binary data file
	bool isVisualFileNewer() const;

	DataSectionPtr	pOwnSect_;
	bool			firstToss_;

private:
	EditorChunkModelBase( const EditorChunkModelBase& );
	EditorChunkModelBase& operator=( const EditorChunkModelBase& );
};


typedef SmartPointer<EditorChunkModelBase> EditorChunkModelBasePtr;


BW_END_NAMESPACE
#endif // EDITOR_CHUNK_MODEL_BASE_HPP
