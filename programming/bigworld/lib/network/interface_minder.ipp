#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: InterfaceMinder
// -----------------------------------------------------------------------------

/**
 * 	This is the default constructor.
 * 	It initialises the InterfaceMinder without any interfaces.
 *
 * 	@param name	Name of the interface.
 */
INLINE InterfaceMinder::InterfaceMinder( const char * name ) :
	name_( name )
{
	elements_.reserve( 256 );
}


/**
 * 	This method returns the handler for the given interface.
 *
 * 	@param index	Index of the interface required.
 *
 * 	@return The handler for the given interface index.
 */
INLINE InputMessageHandler * InterfaceMinder::handler( int index )
{
	return elements_[ index ].pHandler();
}

/**
 * 	This method sets the handler for a given interface.
 *
 * 	@param index	The index for which to set a handler
 * 	@param pHandler	A message handler object
 */
INLINE void InterfaceMinder::handler( int index, InputMessageHandler * pHandler )
{
	elements_[ index ].pHandler( pHandler );
}


/**
 *	This method returns the InterfaceElement with the input id.
 */
INLINE
const InterfaceElement & InterfaceMinder::interfaceElement( uint8 id ) const
{
	return elements_[ id ];
}

} // namespace Mercury

// interface_minder.ipp
