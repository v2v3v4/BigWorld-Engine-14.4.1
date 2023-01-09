// py_script_args.ipp

/**
 *	This method creates a ScriptArgs object with no arguments
 *	@return A new ScriptArgs object
 */
/* static */ inline ScriptArgs ScriptArgs::none()
{
	PyObject * pTuple = PyTuple_New( 0 );
	MF_ASSERT( pTuple );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1 )
{
	PyObject * pTuple = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1,
	const T2 & arg2 )
{
	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1,
	const T2 & arg2, const T3 & arg3 )
{
	PyObject * pTuple = PyTuple_New( 3 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3, typename T4 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1, 
	const T2 & arg2, const T3 & arg3, const T4 & arg4 )
{
	PyObject * pTuple = PyTuple_New( 4 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	PyTuple_SET_ITEM( pTuple, 3, Script::getData( arg4 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1, 
	const T2 & arg2, const T3 & arg3, const T4 & arg4, const T5 & arg5 )
{
	PyObject * pTuple = PyTuple_New( 5 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	PyTuple_SET_ITEM( pTuple, 3, Script::getData( arg4 ) );
	PyTuple_SET_ITEM( pTuple, 4, Script::getData( arg5 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5,
	typename T6 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1,
	const T2 & arg2, const T3 & arg3, const T4 & arg4, const T5 & arg5, 
	const T6 & arg6 )
{
	PyObject * pTuple = PyTuple_New( 6 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	PyTuple_SET_ITEM( pTuple, 3, Script::getData( arg4 ) );
	PyTuple_SET_ITEM( pTuple, 4, Script::getData( arg5 ) );
	PyTuple_SET_ITEM( pTuple, 5, Script::getData( arg6 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5,
	typename T6, typename T7 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1,
	const T2 & arg2, const T3 & arg3, const T4 & arg4, const T5 & arg5, 
	const T6 & arg6, const T7 & arg7 )
{
	PyObject * pTuple = PyTuple_New( 7 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	PyTuple_SET_ITEM( pTuple, 3, Script::getData( arg4 ) );
	PyTuple_SET_ITEM( pTuple, 4, Script::getData( arg5 ) );
	PyTuple_SET_ITEM( pTuple, 5, Script::getData( arg6 ) );
	PyTuple_SET_ITEM( pTuple, 6, Script::getData( arg7 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

template < typename T1, typename T2, typename T3, typename T4, typename T5,
	typename T6, typename T7, typename T8 >
/* static */ inline ScriptArgs ScriptArgs::create( const T1 & arg1, 
	const T2 & arg2, const T3 & arg3, const T4 & arg4, const T5 & arg5, 
	const T6 & arg6, const T7 & arg7, const T8 & arg8 )
{
	PyObject * pTuple = PyTuple_New( 8 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( arg1 ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( arg2 ) );
	PyTuple_SET_ITEM( pTuple, 2, Script::getData( arg3 ) );
	PyTuple_SET_ITEM( pTuple, 3, Script::getData( arg4 ) );
	PyTuple_SET_ITEM( pTuple, 4, Script::getData( arg5 ) );
	PyTuple_SET_ITEM( pTuple, 5, Script::getData( arg6 ) );
	PyTuple_SET_ITEM( pTuple, 6, Script::getData( arg7 ) );
	PyTuple_SET_ITEM( pTuple, 7, Script::getData( arg8 ) );
	return ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE );
}

