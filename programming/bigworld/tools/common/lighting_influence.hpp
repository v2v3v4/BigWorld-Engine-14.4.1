#ifndef STATIC_LIGHTING_HPP
#define STATIC_LIGHTING_HPP


#include "space_editor.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace LightingInfluence
{


	/**
	* Base class for chunk visitor
	*/
	class ChunkVisitor
	{
	public:
		ChunkVisitor( uint32 maxDepth );

		// This method determine if the visitedChunk should be taken into account
		virtual bool considerChunk( Chunk* visitedChunk )	{ return true; }

		// This method execute the action on the visited chunk
		virtual void action( Chunk* visitedChunk )=0;

		uint32 maxDepth()	{ return maxDepth_; }
	private:
		uint32 maxDepth_;
	};


	/**
	* Mark the chunks as dirty for dynamic light
	* LightType must be a pointer to a class with an intersects( BoundingBox )
	* method, which both SpotLight and OmniLight have
	*/
	template<class LightType>
	class DynamicLightMarkChunksVisitor :public ChunkVisitor
	{
	public:
		DynamicLightMarkChunksVisitor( LightType light, uint32 maxDepth = 2 ):
		  light_( light ),
			ChunkVisitor( maxDepth )
		  {
			  BW_GUARD;
		  }

		  virtual bool considerChunk( Chunk* visitedChunk )
		  {
			  BW_GUARD;
			  return light_->intersects( visitedChunk->boundingBox() );
		  }

		  virtual void action( Chunk* visitedChunk )
		  {
			  BW_GUARD;
			  SpaceEditor::instance().changedChunk( 
				  visitedChunk, 
				  InvalidateFlags::FLAG_THUMBNAIL );
		  }
	private:
		LightType light_;


	};


	void visitChunks( Chunk* srcChunk,ChunkVisitor* pVisitor,
		BW::set<Chunk*>& searchedChunks = BW::set<Chunk*>(),
		uint32 currentDepth = 0 );
}


BW_END_NAMESPACE
#endif // STATIC_LIGHTING_HPP
