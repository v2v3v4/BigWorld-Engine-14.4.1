// script.ipp


#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


namespace Script
{

/**
 *	Static function to call a callable object with error checking.
 *	Note that it decrements the reference counts of both of its arguments.
 *
 *	@return true for success
 *	@see callResult
 */
INLINE bool call( PyObject * pFunction, PyObject * pArgs,
	const char * errorPrefix, bool okIfFunctionNull )
{
	PyObject * res = Script::ask(
		pFunction, pArgs, errorPrefix, okIfFunctionNull, true );
	Py_XDECREF( res );
	return (res != NULL) || (okIfFunctionNull && (pFunction == NULL));
}

}
