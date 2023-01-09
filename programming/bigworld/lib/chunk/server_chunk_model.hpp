#ifndef SERVER_CHUNK_MODEL_HPP
#define SERVER_CHUNK_MODEL_HPP

#include "chunk_item.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class SuperModel;

/**
 *	This class defines your standard stocky solid SceneUnit-like model.
 */
class ServerChunkModel : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ServerChunkModel )
	DECLARE_CHUNK_ITEM_ALIAS( ServerChunkModel, shell )

public:
	ServerChunkModel();
	~ServerChunkModel();
	BoundingBox localBB() const;

	static const BW::string & getSectionName();

	bool load( DataSectionPtr pSection,BW::string* pErrorString = NULL );

	void transform( Matrix newTransform ) { transform_ = newTransform; }
	const Matrix & transform() const { return transform_; }

	SuperModel * getSuperModel() { return pSuperModel_; }

protected:
	virtual void toss( Chunk * pChunk );
	virtual const char * label() const;

protected:
	SuperModel *				pSuperModel_;
	BW::string					label_;
	Matrix						transform_;

	// virtual void lend( Chunk * pLender );
};

BW_END_NAMESPACE

#endif // SERVER_CHUNK_MODEL_HPP

