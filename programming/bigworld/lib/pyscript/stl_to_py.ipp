// stl_to_py.ipp


// -----------------------------------------------------------------------------
// Section: PySTLSequenceHolder<class SEQ>
// -----------------------------------------------------------------------------


/**
 *	Constructor
 */
template <class SEQ>
PySTLSequenceHolder<SEQ>::PySTLSequenceHolder
		( SEQ & seq, PyObject * pOwnerIn, bool writableIn ) :
	PySTLSequenceHolderBase( pOwnerIn, writableIn ),
	seq_( seq ),
	eraseBegin_( 0 ),
	eraseEnd_( 0 ),
	insertStart_( 0 ),
	insertCount_( 0 ),
	insertStore_( NULL ),
	insertStock_( 0 )
{
}


/**
 *	This method returns the length of the sequence
 */
template <class SEQ>
int PySTLSequenceHolder<SEQ>::size()
{
	return static_cast<int>( seq_.size() );
}


/**
 *	This method returns the item with the given index in the sequence
 */
template <class SEQ>
PyObject * PySTLSequenceHolder<SEQ>::operator[]( int index )
{
	return Script::getData( seq_[index] );
}


/**
 *	This method returns true if the given object is in the sequence
 */
template <class SEQ>
bool PySTLSequenceHolder<SEQ>::contains( PyObject * pObject )
{
	ItemType extracted = Datatype::DefaultValue<ItemType>::val();

	if (Script::setData( pObject, extracted ) != 0)
	{
		PyErr_Clear();
		return false;
	}

	IteratorType found = std::find( seq_.begin(), seq_.end(), extracted );

	Releaser::release( extracted );

	return found != seq_.end();
}


/**
 *	This method flags the items in the input range as no longer in
 *	the sequence, and remembers to erase them later.
 */
template <class SEQ>
void PySTLSequenceHolder<SEQ>::erase( int indexBegin, int indexEnd )
{
	eraseBegin_ = indexBegin;
	eraseEnd_ = indexEnd;

	for (int i = indexBegin; i < indexEnd; i++)
	{
		Holder::drop( seq_[i], pOwner_ );
	}
}


/**
 *	This method records what the insert range is and sets up item storage
 */
template <class SEQ>
void PySTLSequenceHolder<SEQ>::insertRange( int index, int count )
{
	insertStart_ = index;
	insertCount_ = count;

	insertStore_ = new ItemType[ count ];
	insertStock_ = 0;

	// if it's a pointer it won't clear the array to NULL ... so do it.
	for (int i = 0; i < insertCount_; i++) insertStore_[i] = ItemType();
}


/**
 *	This method converts the given item to the type used by the vector
 *	and stores it for later adding to the sequence.
 */
template <class SEQ>
int PySTLSequenceHolder<SEQ>::insert( PyObject * pItem )
{
	int err = Script::setData(
		pItem, insertStore_[ insertStock_ ], s_varNameValue_ );

	if (err == 0)
	{
		if (Holder::hold( insertStore_[ insertStock_ ], pOwner_ ))
		{
			insertStock_++;
		}
		else
		{
			Releaser::release( insertStore_[ insertStock_ ] );
			// note: we could clear out the entry now with the line below
			//  (in case the destructor does something timely), but there's
			//  not much point because it'll be called anyway on the next
			//  successful insert, or when the array is deleted due to a
			//  cancel. currently we always get cancelled immediately after
			//  any error, so we won't waste time doing this here.
			//insertStore_[ insertStock_ ] = ItemType();
			err = -1;
		}
	}

	return err;
}


/**
 *	This method commits the changes made by prior erases and inserts
 */
template <class SEQ>
void PySTLSequenceHolder<SEQ>::commit()
{
	// release anything from the vector
	for (int i = eraseBegin_; i < eraseEnd_; i++)
	{
		// note: it's OK reference-wise to release all of these here,
		// because if they've been added again in the insert store then
		// we'll have a new reference (or whatever) to them there,
		// acquired when we called setData to extract them.
		Releaser::release( seq_[i] );
	}

	// actually erase stuff
	seq_.erase( seq_.begin() + eraseBegin_, seq_.begin() + eraseEnd_ );
	eraseBegin_ = eraseEnd_ = 0;

	// now insert our store of items
	if (insertStore_ != NULL)
	{
		// make sure we got all the elements to insert
		//  (could relax this condition if desired)
		MF_ASSERT( insertStock_ == insertCount_ );

		// actually insert stuff
		seq_.insert( seq_.begin() + insertStart_,
			insertStore_, insertStore_ + insertStock_ );

		// dispose our store
		bw_safe_delete_array(insertStore_);
		insertStock_ = 0;
	}
}


/**
 *	This method cancels the changes made by prior erases and inserts
 */
template <class SEQ>
void PySTLSequenceHolder<SEQ>::cancel()
{
	// release anything so far inserted
	if (insertStore_ != NULL)
	{
		// drop and release the store
		for (int i = 0; i < insertStock_; i++)
		{
			Holder::drop( insertStore_[i], pOwner_ );
			Releaser::release( insertStore_[i] );
		}

		// dispose our store
		bw_safe_delete_array(insertStore_);
		insertStore_ = NULL;
		insertStock_ = 0;
	}

	// re-hold whatever was let go
	bool good = true;
	for (int i = eraseBegin_; i < eraseEnd_; i++)
	{
		good &= Holder::hold( seq_[i], pOwner_ );
	}
	eraseBegin_ = eraseEnd_ = 0;

	if (!good)
	{
		MF_ASSERT( !"PySTLSequenceHolder::cancel: "
			"Could not re-hold items in erase list!" );
	}
}


/**
 *	This static variable is the variable name used in Script::setData calls
 *
 *	It needs a whole class to initialise it because type info isn't const
 *	data under gcc (it gets initialised along with the other static data
 *	initialisers)
 */
template <class SEQ>
typename PySTLSequenceHolder<SEQ>::InitVNV PySTLSequenceHolder<SEQ>::s_varNameValue_;


/**
 *	Constructor for varNameValue class
 */
/*	must be inline. another compiler bug.
template <class SEQ>
PySTLSequenceHolder<SEQ>::InitVNV::InitVNV() :
	Script::InitTimeJob( -1 )
{
}
*/

/**
 *	varNameValue cast operator
 */
/*
template <class SEQ>
PySTLSequenceHolder<SEQ>::InitVNV::operator const char *() const
{
	return varNameValue_.c_str();
}
*/

/**
 *	varNameValue initialiser (reason for class)
 */
/*
template <class SEQ>
void PySTLSequenceHolder<SEQ>::InitVNV::init()
{
	varNameValue_ = "An item of a PySTLSequence of ";
	varNameValue_+= typeid(PySTLSequenceHolder<SEQ>::ItemType).name();
}
*/


// stl_to_py.ipp
