#ifndef CHUNK_ITEM_HPP
#define CHUNK_ITEM_HPP

#include "scene/scene_object.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/collision_scene_provider.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/guard.hpp"

#include "resmgr/datasection.hpp"

#include "umbra_config.hpp"

#include "cstdmf/bw_set.hpp"

#if UMBRA_ENABLE
#include "umbra_proxies.hpp"

BW_BEGIN_NAMESPACE

class UmbraDrawItem;
class UmbraDrawShadowItem;

BW_END_NAMESPACE

namespace Umbra
{
	class Cell;
};

#endif

#ifndef MF_SERVER
enum SHADOW_TYPE {
	SHADOW_TYPE_NONE = 0,
	SHADOW_TYPE_STATIC = 1,
	SHADOW_TYPE_DYNAMIC = 2
};
#endif // MS_SERVER

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkSpace;
class Vector3;
class BoundingBox;

namespace Moo
{
	class DrawContext;
}
/**
 *	This class is the base class of all the things that can live in a chunk.
 *	It declares only the methods common to the client and the editor for the
 *	management of a chunk.
 *
 *	@see EditorChunkItem
 *	@see ClientChunkItem
 */

// This class reimplements the intersection of {Safe}ReferenceCount
// and PyObjectPlus with virtual functions to ensure subclasses can
// also inherit from PyObjectPlus without breaking SmartPointers
// to this (super)class. {Safe}ReferenceCount is private so that no
// other class can call other {Safe}ReferenceCount methods directly,
// which will not represent the correct state in a class also derived
// from PyObjectPlus.

// To avoid ambiguity, such child-classes must also specifically
// override the relevant methods to use the PyObjectPlus versions:
/*
	virtual void incRef() const		{ this->PyObjectPlus::incRef(); }
	virtual void decRef() const		{ this->PyObjectPlus::decRef(); }
	virtual int refCount() const
	{
		return static_cast< int >( this->PyObjectPlus::refCount() );
	}
*/

// WARNING: This class can still compile calls to {Safe}ReferenceCount
// methods, but child classes will suffer problems at run-time if this
// is done.
class ChunkItemBase : private SafeReferenceCount
{
public:
	enum WantFlags
	{
		WANTS_NOTHING		= 0,
		WANTS_DRAW			= 1 <<  0,
		WANTS_TICK			= 1 <<  1,
		WANTS_UPDATE		= 1 <<  2,
		WANTS_SWAY			= 1 <<  3,
		WANTS_NEST			= 1 <<  4,

		FORCE_32_BIT		= 1 << 31
	};
	static const uint32 USER_FLAG_SHIFT  =  8;
	static const uint32 CHUNK_FLAG_SHIFT = 24;

	enum
	{
		TYPE_DEPTH_ONLY		= ( 1 << 0 ),
	};

	typedef BW::set<Chunk*> Borrowers;	

	explicit ChunkItemBase( WantFlags wantFlags = WANTS_NOTHING );
	ChunkItemBase( const ChunkItemBase & oth );
	virtual ~ChunkItemBase();

	/**
	 *	Pass out the triangle lists in collision scene.
	 *	For item not in the collision scene, the result is empty
	 */
	virtual CollisionSceneProviderPtr getCollisionSceneProvider( Chunk* ) const { return NULL; }

	virtual void toss( Chunk * pChunk ) { this->chunk( pChunk ); }
	virtual void updateAnimations() {}
	virtual void draw( Moo::DrawContext& drawContext ) { }
	virtual void tick( float /*dTime*/ ) { }
	virtual void sway( const Vector3 & /*src*/, const Vector3 & /*dst*/, const float /*radius*/ ) { }
	virtual void lend( Chunk * /*pLender*/ ) { }
	virtual void nest( ChunkSpace * /*pSpace*/ ) { }

	virtual const char * label() const				{ return ""; }
	virtual uint32 typeFlags() const				{ return 0; }

	virtual void incRef() const		{ this->SafeReferenceCount::incRef(); }
	virtual void decRef() const		{ this->SafeReferenceCount::decRef(); }
	virtual int refCount() const
	{
		return this->SafeReferenceCount::refCount();
	}

    Chunk * chunk() const			{ return pChunk_; }
	void chunk( Chunk * pChunk )	{ pChunk_ = pChunk; }

	bool wantsDraw() const { return !!(wantFlags_ & WANTS_DRAW); }
	bool wantsTick() const { return !!(wantFlags_ & WANTS_TICK); }
	bool wantsUpdate() const { return !!(wantFlags_ & WANTS_UPDATE); }
	bool wantsSway() const { return !!(wantFlags_ & WANTS_SWAY); }
	bool wantsNest() const { return !!(wantFlags_ & WANTS_NEST); }

	virtual bool reflectionVisible() { return false; }
	virtual bool affectShadow() const { return false; }

	uint32 drawMark() const { return drawMark_; }
	void drawMark( uint32 val )
		{ drawMark_ = val; }
	uint32 depthMark() const { return depthMark_; }
	void depthMark( uint32 val )
		{ depthMark_ = val; }

	uint8 userFlags() const { return uint8(wantFlags_ >> USER_FLAG_SHIFT); }

	virtual bool addYBounds( BoundingBox& bb ) const	{	return false;	}

	/*** 
	 * The syncInit method is used to construct any requisite Umbra 
	 * objects and update them as needed.
	 */
	virtual void syncInit() {}

	void addBorrower( Chunk* pChunk );
	void delBorrower( Chunk* pChunk );
	void clearBorrowers();

#if UMBRA_ENABLE
	UmbraDrawItem* pUmbraDrawItem() const { return pUmbraDrawItem_; }	
	UmbraDrawShadowItem* pUmbraDrawShadowItem() const { return pUmbraShadowCasterTestItem_; }
#endif
private:
	ChunkItemBase & operator=( const ChunkItemBase & other );	// undefined

	uint32 drawMark_;
	uint32 depthMark_;

protected:
	WantFlags wantFlags_;
	Chunk * pChunk_;

	Borrowers	borrowers_;

#if UMBRA_ENABLE
	void updateUmbraLenders();
	UmbraDrawItem*	pUmbraDrawItem_;
	UmbraDrawShadowItem*  pUmbraShadowCasterTestItem_;
#endif

	void lendByBoundingBox( Chunk * pLender, const BoundingBox & worldbb );

public: static	uint32	s_instanceCount_;
public: static	uint32	s_instanceCountPeak_;

	const SceneObject& sceneObject() { return sceneObject_; }

protected:
	SceneObject sceneObject_;
};


#ifdef EDITOR_ENABLED

BW_END_NAMESPACE

#include "editor_chunk_item.hpp"

BW_BEGIN_NAMESPACE

#else

	/**
	 *	This class declares the extra data and methods that the client
	 *	requires all its chunk items to have. Currently it is empty.
	 */
	class ClientChunkItem : public ChunkItemBase
	{
	public:
		explicit ClientChunkItem( WantFlags wantFlags = WANTS_NOTHING ) : 
			ChunkItemBase( wantFlags ) { }
	};

	typedef ClientChunkItem SpecialChunkItem;
#endif

/**
 *	The base class for all items resides in a chunk
 *	It inherited from SpecialChunkItem to get virtual methods
 *	specified to different usage, like client or editor.
 */
class ChunkItem : public SpecialChunkItem
{
public:
	explicit ChunkItem( WantFlags wantFlags = WANTS_NOTHING ) :
		SpecialChunkItem( wantFlags ) 
	{
		sceneObject_.handleAndType( this );
	}
	virtual void syncInit() {}
#ifndef MF_SERVER
	void	toss( Chunk* pChunk );
#endif
};

/**
 *	the smartpointer of ChunkItem
 */
typedef SmartPointer<ChunkItem>	ChunkItemPtr;



/**
 *	This class is a factory for chunk items. It is up to the
 *	factory's 'create' method to actually add the item to the
 *	chunk (as a dynamic or static item - or not at all).
 *
 *	This class may be used either by deriving from it and overriding
 *	the 'create' method, or by passing a non-NULL creator function
 *	into the constructor.
 */
class ChunkItemFactory
{
public:

	/**
	 *	This class is used to return a more descriptive result
	 *	it is used by chunk item factory methods
	 */
	class Result
	{
		bool succeeded_;
		bool onePerChunk_;
		ChunkItemPtr item_;
		BW::string errorString_;
	protected:
		/**
		 *	This protected Constructor is used by derived class
		 *	to create an object indicates a success
		 */
		Result() :
			succeeded_( true ),
			onePerChunk_()
		{}

	public:
		/**
		 *	Constructor
		 *	@param	item	The result chunk item, it could be NULL
		 *					a newly created chunk item or an existing one
		 *	@param	errorString	A string to describe any error occurred
		 *  @param  onePerChunk A boolean indicating whether only one of these items
		 *                      may exist per Chunk.
		 */
		explicit Result( ChunkItemPtr item,
				const BW::string& errorString = "",
				bool onePerChunk = false ) :
			succeeded_( item ),
			onePerChunk_( onePerChunk ),
			item_( item ),
			errorString_( errorString )
		{}
		/**
		 *	conversion operator to bool
		 */
		operator bool() const
		{
			return succeeded_;
		}
		/**
		 *	smart pointer like operator->
		 */
		ChunkItemPtr operator->()
		{
			return item_;
		}
		ChunkItemPtr item()
		{
			return item_;
		}
		bool onePerChunk() const 
		{ 
			return onePerChunk_; 
		}
		const BW::string& errorString() const
		{
			return errorString_;
		}
	};


	/**
	 *	This class is used when you return a value which indicates
	 *	a successful loading while no item is returned
	 */
	class SucceededWithoutItem : public Result
	{};

	typedef Result (*Creator)( Chunk * pChunk, DataSectionPtr pSection );

	ChunkItemFactory( const BW::string & section,
		int priority = 0,
		Creator creator = NULL );
	virtual ~ChunkItemFactory() {};

	virtual Result create( Chunk * pChunk, DataSectionPtr pSection ) const;

	int priority() const							{ return priority_; }
private:
	int		priority_;
	Creator creator_;
};


/**
 *	This macro is used to declare a class as a chunk item. It is used to
 *	implement the factory functionality. It should appear in the declaration of
 *	the class.
 *
 *	Classes using this macro should implement the load method and also use
 *	the IMPLEMENT_CHUNK_ITEM macro.
 */
#define DECLARE_CHUNK_ITEM( CLASS )											\
	static ChunkItemFactory::Result create( Chunk * pChunk, 				\
		DataSectionPtr pSection );											\
	static ChunkItemFactory factory_;										\

/// declare an alias of a existing class
#define DECLARE_CHUNK_ITEM_ALIAS( CLASS, ALIAS )							\
	static ChunkItemFactory ALIAS##factory_;

/**
 *	This macro is used to implement some of the chunk item functionality of a
 *	class that has used the DECLARE_CHUNK_ITEM macro.
 *  NOTE: the load method should set the 'errorString' when it fails by passing
 *	a reference to the errorString variable in IMPLEMENT_CHUNK_ITEM_ARGS
 */
#define IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD_BASE( CLASS, PRIORITY, ADD )		\
	ChunkItemFactory::Result CLASS::create( Chunk * pChunk, 				\
			DataSectionPtr pSection )										\
	{																		\
		SmartPointer< CLASS > pItem = new CLASS();							\
		PROFILE_FILE_SCOPED( CLASS ); 										\
																			\
		BW::string errorString; 											\
		if (pItem->load IMPLEMENT_CHUNK_ITEM_ARGS)							\
		{																	\
			if (ADD &&!pChunk->addStaticItem( pItem ))						\
			{																\
				ERROR_MSG( #CLASS "::create: "								\
						"error in section %s of %s in mapping %s\n", 		\
					pSection->sectionName().c_str(), 						\
					pChunk->identifier().c_str(),							\
					pChunk->mapping()->path().c_str() );					\
			}																\
			return ChunkItemFactory::Result( pItem );						\
		}																	\
																			\
		return ChunkItemFactory::Result( NULL, errorString );				\
	}																		\


#define IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD( CLASS, LABEL, PRIORITY, ADD )	\
	ChunkItemFactory CLASS::factory_( #LABEL, PRIORITY, CLASS::create );	\
	IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD_BASE( CLASS, PRIORITY, ADD )

#define IMPLEMENT_CHUNK_ITEM( CLASS, LABEL, PRIORITY )						\
	IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD( CLASS, LABEL, PRIORITY, true )		\

#define IMPLEMENT_CHUNK_ITEM_STR( CLASS, LABEL, PRIORITY )						\
	IMPLEMENT_CHUNK_ITEM_OPTIONAL_ADD( CLASS, LABEL, PRIORITY, true )		\

/// This macro can be redefined if your load method takes different arguments
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection)

/// implement an alias of a existing class
#define IMPLEMENT_CHUNK_ITEM_ALIAS( CLASS, LABEL, PRIORITY )							\
	ChunkItemFactory CLASS::LABEL##factory_( #LABEL, PRIORITY, CLASS::create );

BW_END_NAMESPACE

#endif // CHUNK_ITEM_HPP
