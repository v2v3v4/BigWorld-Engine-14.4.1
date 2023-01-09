
// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
template <class TYPE>
Factory<TYPE>::Factory()
{
}


/**
 *	Destructor.
 */
template <class TYPE>
Factory<TYPE>::~Factory()
{
}


/**
 *	This method initialises the registered %%%s from the input data
 *	section.
 */
template <class TYPE>
bool Factory<TYPE>::init( DataSectionPtr pSection )
{
	bool succeeded = true;

	if (pSection)
	{
		DataSectionPtr pBaseSections = pSection->openSection( "Bases" );

		if (pBaseSections)
		{
			DataSection::iterator sectionIter = pSection->begin();

			while (sectionIter != pSection->end())
			{
				Creators::iterator creatorIter =
					creators_.find( (*sectionIter)->sectionName() );

				if (creatorIter != creators_.end())
				{
					succeeded &= creatorIter->second->init( *sectionIter );
				}

				sectionIter++;
			}
		}

		pExtensions_ = pSection->openSection( "Extensions" );
	}

	return succeeded;
}


// -----------------------------------------------------------------------------
// Section: General
// -----------------------------------------------------------------------------

/**
 *	This method adds a new creator to this factory. It gives this factory the
 *	ability to create the TYPEs that are associated with the input creator.
 *
 *	@param name		The name to associate with creating the TYPE.
 *	@param creator	The object responsible for creating the TYPE.
 */
template <class TYPE>
void Factory<TYPE>::add( const char * name, Creator<TYPE> & creator )
{
	creators_[ name ] = &creator;
}


/**
 *	This method creates the TYPE associated with the input name.
 *
 *	@param name		The name of the TYPE to create.
 *	@param pSection	The data that should be used to create the TYPE.
 *
 *	@return	The newly created TYPE or NULL if none was created.
 */
template <class TYPE>
TYPE * Factory<TYPE>::create( const char * name,
	DataSectionPtr pSection ) const
{
	Creators::const_iterator iter = creators_.find( name );

	if (iter == creators_.end())
	{
		if ((!pSection || pSection->countChildren() == 0) && pExtensions_)
		{
			pSection = pExtensions_->openSection( name );
		}

		if (pSection)
		{
			iter = creators_.find( pSection->asString() );
		}
	}

	if (iter != creators_.end())
	{
		return iter->second->create( pSection );
	}

#ifndef EDITOR_ENABLED
	ERROR_MSG( "Factory::create: Unable to create %s\n",
		name );
#endif

	return NULL;
}


// -----------------------------------------------------------------------------
// Section: Streaming
// -----------------------------------------------------------------------------

/**
 *	Output streaming operator for Factory.
 */
template <class TYPE>
std::ostream& operator<<(std::ostream& o, const Factory<TYPE>& t)
{
	o << "Factory\n";
	return o;
}


// -----------------------------------------------------------------------------
// Section: Creator<TYPE>
// -----------------------------------------------------------------------------

/**
 *	This method is called to initialise a TYPE creator. It is called by the
 *	Factory's init method.
 *
 *	@return	True if successful, otherwise false.
 *	@see Factory::init
 */
template <class TYPE>
bool Creator<TYPE>::init( DataSectionPtr pSection )
{
	return true;
}


// -----------------------------------------------------------------------------
// Section: StandardCreator
// -----------------------------------------------------------------------------

/**
 *	Constructor. This method registers this creator with the input name.
 */
template <class TYPE>
StandardCreator<TYPE>::StandardCreator( const char * name, Factory<TYPE> & factory )
{
	factory.add( name, *this );
}


/**
 *	This method initialises this creator.
 */
template <class TYPE>
bool StandardCreator<TYPE>::init( DataSectionPtr pSection )
{
	pSection_ = pSection;

	return true;
}


/**
 *	This is a helper method that is used by derived classes to implement the
 *	create method. It calls the init method of the %%%, first with the
 *	creator's section and then with the objects section.
 */
template <class TYPE>
TYPE * StandardCreator<TYPE>::createHelper( DataSectionPtr pSection,
	TYPE * pNewObject )
{
	pNewObject->incRef();

	if (/*!pNewObject->init( pSection_ ) ||*/
		!pNewObject->init( pSection ))
	{
		pNewObject->decRef();
		pNewObject = NULL;
	}

	return pNewObject;
}

BW_END_NAMESPACE
