#ifndef ENTITY_EXTRA_HPP
#define ENTITY_EXTRA_HPP

#include "entity.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class allows arbitrary extension of the entity class.
 *	It can exist on both real and ghost entities, but is never ghosted.
 *	Use a controller and get it to register itself with your EntityExtra
 *	if you want to have ghosted data.
 *	EntityExtras are owned by their entity, so if the entity disappears
 *	you will be mercilessly deleted. You may also delete yourself at any time.
 *
 *	Derived classes of derived classes of EntityExtra are not supported,
 *	because of the ambiguity of resolving Python touch attributes,
 *	as well as other similar problems. If a good reason was found to change
 *	this stance then it could happen, but won't until something comes up.
 */
class EntityExtra
{
	PY_FAKE_PYOBJECTPLUS_BASE_DECLARE()

public:
	EntityExtra( Entity & e ) : entity_( e ) { }
	virtual ~EntityExtra() { }

	virtual void relocated() { }

	/**
	 *	Template helper class to manage the instance of a derived EntityExtra
	 *	class in an entity.
	 */
	template <class Derived> class Instance
	{
	public:
		typedef EntityExtra * (*FactoryFn)( Entity & e );

		Instance( PyMethodDef * pMethods = NULL, PyGetSetDef * pAttributes = NULL,
				FactoryFn ff = &construct ) :
			eeid_( Entity::registerEntityExtra( pMethods, pAttributes ) ),
			ff_( ff )
		{ }

		/**
		 *	Access the instance of this entity extra in the given entity
		 */
		Derived & operator()( Entity & e ) const
		{
			Derived * & rpd = (Derived*&)e.entityExtra( eeid_ );
			if (rpd == NULL) rpd = (Derived*)(*ff_)( e );
			return *rpd;
		}

		/**
		 *	Return whether or not there is an instance of this entity extra
		 *	in the given entity
		 */
		bool exists( const Entity & e ) const
		{
			return !!e.entityExtra( eeid_ );
		}

		/**
		 *	Clear the instance of this entity extra in the given entity
		 *	Safe to call even if there is no instance
		 */
		void clear( Entity & e ) const
		{
			EntityExtra * & rpee = e.entityExtra( eeid_ );
			delete rpee;
			rpee = NULL;
		}

		int eeid() const { return eeid_; }

	private:
		static EntityExtra * construct( Entity & e )
		{
			return new Derived( e );
		}

		int			eeid_;	///< Entity extra id (index in array)
		FactoryFn	ff_;	///< Factory fn for touch masquerading as a feature
	};

	void pyAdditionalMembers( const ScriptList & pList ) const
		{ MF_ASSERT( false ); }
	void pyAdditionalMethods( const ScriptList & pList ) const
		{ MF_ASSERT( false ); }
protected:
	// The following is here so that it can look more like Controller code.
	Entity & entity()					{ return entity_; }
	const Entity & entity() const		{ return entity_; }

	Entity & entity_;
};


/**
 *	This macro does header stuff for entity extra classes that want Python
 *	attributes and methods. The _eeToPyPtr static method exists to allow
 *	EntityExtra classes that use multiple inheritance to function correctly.
 */
#define Py_EntityExtraHeader( CLASS )										\
	Py_FakeHeader( CLASS, PyObjectPlus )									\
																			\
	static CLASS * getSelf( PyObject * self )								\
	{																		\
		return &This::instance( *((Entity*)self) );							\
	}																		\


/*

A derived entity extra class might look like this:
[Note that you may use PyObjectPlus macros, but must NOT derive from it,
nor from any other reference-counted class]

class DerivedEE : public EntityExtra
{
	[Py_EntityExtraHeader( DerivedEE )]

public:
	DerivedEE( Entity & e ) : EntityExtra( e ) { }

	static const Instance<DerivedEE> instance;
};

const EntityExtra::Instance<DerivedEE> DerivedEE::instance(
		[DerivedEE:s_getMethodDefs(), [DerivedEE::s_getAttributeDefs(),
		[&DerivedEE:construct]]] );

*/

BW_END_NAMESPACE

#endif // ENTITY_EXTRA_HPP
