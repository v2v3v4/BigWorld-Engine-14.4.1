#ifndef COMPILED_SPACE_ENTITY_EMBODIMENT_HPP
#define COMPILED_SPACE_ENTITY_EMBODIMENT_HPP

#include "forward_declarations.hpp"

#include "pyscript/stl_to_py.hpp"

#include "script_object_query_operation.hpp"


namespace BW
{

namespace Moo
{
	class DrawContext;
}

class Scene;
class ScriptObject;
class IEntityEmbodiment ;

typedef SmartPointer< IEntityEmbodiment > IEntityEmbodimentPtr;

class IEntityEmbodiment : public SafeReferenceCount
{
public:
	IEntityEmbodiment( const ScriptObject& object );
	virtual ~IEntityEmbodiment();

	void enterSpace( ClientSpacePtr pSpace, bool transient = false );
	void leaveSpace( bool transient = false );
	void move( float dTime );

	void worldTransform( const Matrix & transform );
	const Matrix & worldTransform() const;
	const AABB & localBoundingBox() const;

	void draw( Moo::DrawContext& drawContext );

	bool isOutside() const;
	bool isRegionLoaded( Vector3 testPos, float radius = 0.0f ) const;

	const ScriptObject& scriptObject();
	const ClientSpacePtr& space() const;

protected:

	virtual void doMove( float dTime ) = 0;

	virtual void doWorldTransform( const Matrix & transform ) = 0;
	virtual const Matrix & doWorldTransform() const = 0;
	virtual const AABB & doLocalBoundingBox() const = 0;

	virtual void doDraw( Moo::DrawContext & drawContext ) = 0;

	virtual bool doIsOutside() const = 0;
	virtual bool doIsRegionLoaded( Vector3 testPos, float radius = 0.0f ) 
		const = 0;

	virtual void doEnterSpace( ClientSpacePtr pSpace, bool transient = false ) = 0;
	virtual void doLeaveSpace( bool transient = false ) = 0;

private:
	ScriptObject object_;
	ClientSpacePtr space_;

protected:
	template<typename T>
	class ScriptObjectQuery : 
		public IScriptObjectQueryOperationTypeHandler
	{
	public:
		virtual ScriptObject doGetScriptObject( const SceneObject & object )
		{
			T* pEmb = object.getAs<T>();
			MF_ASSERT( pEmb );
			return pEmb->scriptObject();
		}
	};
};

typedef BW::vector<IEntityEmbodimentPtr> EntityEmbodiments;

/*
 *	Stuff to make a vector of embodiments work on its own in python
 */
namespace PySTLObjectAid
{
	/// Releaser is same for all PyObject's (but not for ChunkEmbodiment)
	template <> struct Releaser< IEntityEmbodimentPtr >
	{
		static void release( IEntityEmbodimentPtr & pCA );
	};

	/// Holder is special for our embodiment list
	template <> struct Holder< EntityEmbodiments >
	{
		static bool hold( IEntityEmbodimentPtr & it, PyObject * pOwner );
		static void drop( IEntityEmbodimentPtr & it, PyObject * pOwner );
	};
};


namespace Script
{
	PyObject * getData( const IEntityEmbodimentPtr pCA );
	int setData( PyObject * pObj, IEntityEmbodimentPtr & pCA,
		const char * varName = "" );
}

} // namespace BW

#endif // COMPILED_SPACE_ENTITY_EMBODIMENT_HPP
