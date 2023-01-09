#ifndef RESOURCE_REF_HPP
#define RESOURCE_REF_HPP

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This helper class is a reference to some resource.
 *	It is half way between smart pointer and reference count.
 */
class ResourceRef
{
public:
	ResourceRef():
		data_( NULL ),
		modRef_( NULL ),
		id_( "" ),
		pyInstance_( NULL )
	{
	}

	ResourceRef( void * data, void (*modRef)( void *, int ), const BW::string& id, PyObject*(*pyInstance)(ResourceRef&) ):
		data_( data ),
		modRef_( modRef ),
		id_( id ),
		pyInstance_( pyInstance )
	{
		if (data_)
		{
			modRef_( data_, 1 );
		}
	}

	ResourceRef( const ResourceRef & other ):
		data_( NULL )
	{
		*this = other;
	}

	virtual ~ResourceRef()
	{		
		if (data_)
		{
			modRef_( data_, -1 );
		}		
	}

	ResourceRef & operator=( const ResourceRef & other )
	{
		if (data_) modRef_( data_, -1 );
		modRef_ = other.modRef_;
		data_ = other.data_;
		id_ = other.id_;
		pyInstance_ = other.pyInstance_;
		if (data_) modRef_( data_, 1 );
		return *this;
	}

	operator bool() const { return !!data_; }
	void * data()		  { return data_; }
	const BW::string& id() const	{ return id_; }

	PyObject * pyInstance()
	{
		if ( !pyInstance_ )
			return NULL;

		return pyInstance_( *this );
	}

	static ResourceRef getIfLoaded( const BW::string & id );
	static ResourceRef getOrLoad( const BW::string & id );

private:	
	void (*modRef_)( void * data, int delta );
	PyObject*(*pyInstance_)(ResourceRef&);

protected:
	void * data_;
	BW::string id_;

public:
	void autoModRef( int delta )
	{
		if (data_)
			modRef_( data_, delta );
	}

	/**
	 *	This struct is a helper struct for loading different
	 *	kinds of resources depending on their extensions.
	 */
	struct ResourceFns
	{
		ResourceRef (*getIfLoaded_)( const BW::string & id );
		ResourceRef (*getOrLoad_)( const BW::string & id );		

		static const ResourceFns & getForString( const BW::string & id );
	};

	/**
	 *	Standard mod ref function
	 */
	template <class C> static void stdModRef( void * data, int delta )
	{
		C * pVal = (C*)data;
		if (delta > 0)	incrementReferenceCount( *pVal );
		else			decrementReferenceCount( *pVal );
	}	
};

BW_END_NAMESPACE

#endif // RESOURCE_REF_HPP
