#ifndef FACTORY_HPP
#define FACTORY_HPP

#include "resmgr/datasection.hpp"
#include "cstdmf/stringmap.hpp"
#include <iostream>

BW_BEGIN_NAMESPACE

/**
 *	This class defines an interface for a TYPE creator.
 */
template <class TYPE>
class Creator
{
public:
	virtual bool init( DataSectionPtr pSection );

	/**
	 *	This pure virtual method should be implemented by class to create a
	 *	TYPE. It is used to register a  with the factory.
	 */
	virtual TYPE * create( DataSectionPtr pSection ) = 0;
};


/**
 *	This class implements a singleton object that is responsible for creating
 *	TYPEs. It mainly exists to be able to create TYPEs from data
 *	files.
 */
template <class TYPE>
class Factory
{
public:
	~Factory();
	bool init( DataSectionPtr pSection );

	void add( const char * name, Creator<TYPE> & creator );
	TYPE * create( const char * name, DataSectionPtr pSection = NULL ) const;

protected:
	Factory();

private:
	Factory( const Factory& );
	Factory& operator=( const Factory& );

	template< typename TYPE >
	friend std::ostream& operator<<( std::ostream&, const Factory& );

	typedef typename StringHashMap< Creator<TYPE> *> Creators;
	Creators		creators_;
	DataSectionPtr	pExtensions_;
};


/**
 *	This class is a base class for TYPE creators. It adds a little bit more
 *	functionality. The constructor will add the object to the factory and it
 *	also implements a helper method to help derived classes implement the create
 *	method.
 */
template <class TYPE>
class StandardCreator : public Creator<TYPE>
{
protected:
	StandardCreator( const char * name, Factory<TYPE> & factory );

	virtual bool init( DataSectionPtr pSection );

	TYPE * createHelper( DataSectionPtr pSection, TYPE * p );

private:
	DataSectionPtr pSection_;
};


#define IMPLEMENT_CREATOR( NAME, TYPE )						\
class NAME##Creator : public StandardCreator<TYPE>			\
{															\
public:														\
	/**														\
	 *	This constructor registers this behaviour.			\
	 */														\
	NAME##Creator() : StandardCreator<TYPE>(				\
		#NAME, TYPE##Factory::instance() )					\
	{														\
	}														\
															\
private:													\
	/* Override from TYPE##Creator. */						\
	virtual TYPE * create( DataSectionPtr pSection )		\
	{														\
		return this->createHelper( pSection, new NAME() );	\
	}														\
};															\
															\
															\
/** The creator for ScoreTry. */							\
static NAME##Creator s_##NAME##Creator;						\

BW_END_NAMESPACE


// always included for the template definations
#include "factory.ipp"


#endif // FACTORY_HPP
