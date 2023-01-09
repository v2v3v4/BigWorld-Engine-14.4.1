// py_script_tuple.ipp

/**
 *	This method creates a tuple of a specified size
 *	@param len		The size of the tuple to create
 *	@return			A new tuple of size len
 */
/* static */ inline ScriptTuple ScriptTuple::create( 
	ScriptTuple::size_type len )
{
	PyObject * pTuple = PyTuple_New( len );
	MF_ASSERT( pTuple );
	return ScriptTuple( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method gets an item from a tuple, this method will assert if the
 *	position is invalid
 *	@param pos		The position to get the value from
 *	@return			The value at position pos
 */
inline ScriptObject ScriptTuple::getItem( ScriptTuple::size_type pos ) const
{
	MF_ASSERT( pos < PyList_GET_SIZE( this->get() ) );
	PyObject * pItem = PyTuple_GET_ITEM( this->get(), pos );
	return ScriptObject( pItem, ScriptObject::FROM_BORROWED_REFERENCE );
}


/**
 *	This method sets an item in the tuple, this method will assert if the
 *	position is invalid
 *	@param pos		The position to set the value of
 *	@param item		The item to set at position pos
 */
inline bool ScriptTuple::setItem( ScriptTuple::size_type pos, 
	const ScriptObject & item ) const
{
	MF_ASSERT( pos < PyTuple_GET_SIZE( this->get() ) );
	// SET_ITEM steals a reference so we need to incref it
	PyTuple_SET_ITEM( this->get(), pos, item.newRef() );

	return true;
}


/**
 *	This method gets the size of the tuple
 *	@return The size of the tuple
 */
inline ScriptTuple::size_type ScriptTuple::size() const
{
	return PyTuple_GET_SIZE( this->get() );
}
