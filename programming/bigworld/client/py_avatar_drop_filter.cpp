#include "pch.hpp"

#include "py_avatar_drop_filter.hpp"

#include "pyscript/script_math.hpp"

#include "script/script_object.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

typedef WeakPyPtr< PyAvatarDropFilter > WeakPyAvatarDropFilterPtr;


/*~ class NoModule.GroundNormalProvider
 *
 *	This class implements a Vector4Provider to the ground normal of
 *	an AvatarDropFilter. It does this by maintaining a weak reference to the
 *	filter instance.
 */
/**
 *	This class implements a Vector4Provider to the ground normal of
 *	an AvatarDropFilter. It does this by maintaining a weak reference to the
 *	filter instance.
 */
class GroundNormalProvider : public Vector4Provider
{
	Py_Header( GroundNormalProvider, Vector4Provider )

public:
	GroundNormalProvider( PyAvatarDropFilter & pyAvatarDropFilter,
			PyTypeObject * pType = &s_type_ )
		: Vector4Provider( false, pType ),
		wpPyAvatarDropFilter_( &pyAvatarDropFilter )
	{
		BW_GUARD;
		MF_ASSERT( wpPyAvatarDropFilter_.exists() );
	}

	~GroundNormalProvider()
	{
		BW_GUARD;
	}

	PY_RO_ATTRIBUTE_DECLARE( wpPyAvatarDropFilter_.get(), avatarDropFilter );

	virtual void output( Vector4 & val )
	{
		BW_GUARD;
		if (wpPyAvatarDropFilter_.exists() &&
			wpPyAvatarDropFilter_->pAttachedFilter() != NULL)
		{
			const BW::Vector3 & normal =
				wpPyAvatarDropFilter_->pAttachedFilter()->groundNormal();
			val.set( normal.x, normal.y, normal.z, 0.f );
		}
	}

private:
	WeakPyAvatarDropFilterPtr wpPyAvatarDropFilter_;
};

PY_TYPEOBJECT( GroundNormalProvider )

PY_BEGIN_METHODS( GroundNormalProvider )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GroundNormalProvider )
	/*~ attribute GroundNormalProvider.avatarDropFilter
	 *
	 *	This attribute returns the AvatarDropFilter weakly reference by the
	 *	provider or None if the filter has been destroyed.
	 *
	 *	@type AvatarDropFilter
	 */
	PY_ATTRIBUTE( avatarDropFilter )
PY_END_ATTRIBUTES()




PY_TYPEOBJECT( PyAvatarDropFilter )

PY_BEGIN_METHODS( PyAvatarDropFilter )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAvatarDropFilter )
	/*~ attribute AvatarDropFilter.alignToGround
	 *
	 *	This attribute controls whether the entity should also be rotated to
	 *	match the slope of the ground on which they are standing.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( alignToGround )
	/*~ attribute AvatarDropFilter.groundNormal
	 *
	 *	The attribute returns a Vector4 provider to the normalised ground
	 *	normal of the drop filter in the form (x, y, z, 0).
	 *
	 *	@type GroundNormalProvider
	 */
	PY_ATTRIBUTE( groundNormal )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyAvatarDropFilter )

/*~ function BigWorld.AvatarDropFilter
 *
 *	This function creates a new AvatarDropFilter, which is used to move avatars
 *	around the same as an AvatarFilter does, but also to keep them on the
 *	ground beneath their specified positions.
 *
 *  @param	an optional AvatarFilter to initialise the filter with
 *
 *	@return a new PyAvatarDropFilter
 */
/**
 *
 */
PY_FACTORY_NAMED( PyAvatarDropFilter, "AvatarDropFilter", BigWorld );


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
PyAvatarDropFilter::PyAvatarDropFilter( PyTypeObject * pType ) :
	PyAvatarFilter( pType ),
	alignToGround_( false ),
	wpGroundNormalProvider_( NULL )
{
	BW_GUARD;
}


/**
 *	This method returns alignToGround for this object's AvatarDropFilter, if it
 *	exists, and its cache of the value if not.
 */
bool PyAvatarDropFilter::alignToGround() const
{
	BW_GUARD;

	const AvatarDropFilter * pFilter = this->pAttachedFilter();

	if (pFilter == NULL)
	{
		return alignToGround_;
	}

	return pFilter->alignToGround();
}


/**
 *	This method updates alignToGround for this object and its AvatarDropFilter.
 */
void PyAvatarDropFilter::alignToGround( bool newValue )
{
	BW_GUARD;

	if (newValue == this->alignToGround())
	{
		return;
	}

	alignToGround_ = newValue;

	AvatarDropFilter * pFilter = this->pAttachedFilter();

	if (pFilter != NULL)
	{
		pFilter->alignToGround( alignToGround_ );
	}
}


/**
 *	This function returns a GroundNormalProvider instance for this
 *	PyAvatarDropFilter. If we have a valid weak reference, a reference to
 *	its target will be returned.
 */
Vector4ProviderPtr PyAvatarDropFilter::groundNormalProvider()
{
	BW_GUARD;

	Vector4ProviderPtr result;

	if (!wpGroundNormalProvider_.exists())
	{
		GroundNormalProvider * pNewProvider = new GroundNormalProvider( *this );
		result = Vector4ProviderPtr( pNewProvider,
			Vector4ProviderPtr::STEAL_REFERENCE );

		wpGroundNormalProvider_ = pNewProvider;
	}
	else
	{
		result = Vector4ProviderPtr( wpGroundNormalProvider_.get() );
	}

	MF_ASSERT( wpGroundNormalProvider_.exists() );
	MF_ASSERT( result.exists() );

	return result;
}


/**
 *	This method returns the AvatarDropFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
AvatarDropFilter * PyAvatarDropFilter::pAttachedFilter()
{
	BW_GUARD;

	return static_cast< AvatarDropFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	This method returns the const AvatarDropFilter * we created, if not yet
 *	lost, and NULL otherwise.
 */
const AvatarDropFilter * PyAvatarDropFilter::pAttachedFilter() const
{
	BW_GUARD;

	return static_cast< const AvatarDropFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	This method provides a new AvatarDropFilter
 */
AvatarDropFilter * PyAvatarDropFilter::getNewFilter()
{
	BW_GUARD;

	AvatarDropFilter * pNewFilter = new AvatarDropFilter( this );
	pNewFilter->alignToGround( alignToGround_ );
	return pNewFilter;
}


/**
 *	This method updates our cached filter properties before the existing filter
 *	is deleted.
 */
void PyAvatarDropFilter::onLosingAttachedFilter()
{
	BW_GUARD;

	AvatarDropFilter * pFilter = this->pAttachedFilter();

	alignToGround_ = pFilter->alignToGround();
}


BW_END_NAMESPACE

// py_avatar_drop_filter.cpp
