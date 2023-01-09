#include "pch.hpp"

// This file contains code that uses both the pyscript and network libraries
// and is common to many processes. This is not big enough to be its own
// library...yet.

// BW Tech Headers
#include "cstdmf/bw_string.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/endpoint.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "cstdmf/debug_message_categories.hpp"

#include <arpa/inet.h>
#include <errno.h>


BW_BEGIN_NAMESPACE

namespace PyNetwork
{

// -----------------------------------------------------------------------------
// Section: File descriptor registration
// -----------------------------------------------------------------------------

// Maps from the file descriptor to a callback
typedef BW::map< int, class PyFileDescriptorHandler * > FileDescriptorMap;

// Maps from the file descriptor to the callback associated with reading
FileDescriptorMap g_fdReadHandlers;

// Maps from the file descriptor to the callback associated with writing
FileDescriptorMap g_fdWriteHandlers;

// Stores the dispatcher used to register file descriptors.
Mercury::EventDispatcher * g_pDispatcher = NULL;
Mercury::NetworkInterface * g_pInterface = NULL;

/**
 *	This function initialises the script interface to the network library.
 *
 *	@param dispatcher The event dispatcher that will be used to register file
 *		descriptors against for read/write events.
 *	@param interface The internal network interface to use for reporting from
 *		Python.
 */
bool init( Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & interface )
{
	if (g_pDispatcher != NULL)
	{
		MF_ASSERT_DEV( !"PyNetwork already initialised" );
		return false;
	}

	g_pDispatcher = &dispatcher;
	g_pInterface = &interface;

	return true;
}


/**
 *	This class is used to handle the cleanup tasks.
 */
class PyNetworkFiniTimeJob : public Script::FiniTimeJob
{
protected:
	virtual void fini()
	{
		FileDescriptorMap::iterator iter = g_fdReadHandlers.begin();
		while (iter != g_fdReadHandlers.end())
		{
			g_pDispatcher->deregisterFileDescriptor( iter->first );
			++iter;
		}

		iter = g_fdWriteHandlers.begin();
		while (iter != g_fdWriteHandlers.end())
		{
			g_pDispatcher->deregisterWriteFileDescriptor( iter->first );
			++iter;
		}
	}
};
PyNetworkFiniTimeJob g_finiTimeJob;


/**
 *	This class is used to store the callbacks that have been registered with
 *	file descriptors.
 */
class PyFileDescriptorHandler : public Mercury::InputNotificationHandler
{
public:
	explicit PyFileDescriptorHandler( PyObject * pCallback,
			PyObject * pFileOrSocket ) :
		pCallback_( pCallback ),
		pFileOrSocket_( pFileOrSocket ) {}

	int handleInputNotification( int fd )
	{
		Py_INCREF( pCallback_.get() );
		Script::call( pCallback_.get(),
				PyTuple_Pack( 1, pFileOrSocket_.get() ) );

		return 0;
	}

private:
	PyObjectPtr pCallback_;
	PyObjectPtr pFileOrSocket_;
};


// -----------------------------------------------------------------------------
// Section: Common file descriptor methods
// -----------------------------------------------------------------------------

/**
 *	This helper method extracts the file descriptor from the input object.
 */
bool extractFileDescriptor( PyObject * pFileOrSocket, int & fd )
{
	if (g_pDispatcher == NULL)
	{
		PyErr_SetString( PyExc_SystemError, "PyNetwork not yet initialised" );
		return false;
	}

	PyObject * pFileDescriptor =
		PyObject_CallMethod( pFileOrSocket, "fileno", "" );

	// If the object does not have a fileno method, it could be an integer.
	if (pFileDescriptor == NULL)
	{
		MF_ASSERT( PyErr_Occurred() );

		PyErr_Clear();
		pFileDescriptor = pFileOrSocket;
	}

	fd = int( PyInt_AsLong( pFileDescriptor ) );

	if ((fd == -1) && PyErr_Occurred())
	{
		PyErr_SetString( PyExc_TypeError,
				"First argument is not a file descriptor" );
		return false;
	}

	return true;
}


/**
 *	This method registers a callback that will be called associated with
 *	activity on the input file descriptor.
 */
bool registerFileDescriptorCommon(
		PyObject * pFileOrSocket, PyObject * pCallback, 
		const char * name, bool isRead )
{
	int fd = -1;

	if (!extractFileDescriptor( pFileOrSocket, fd ))
	{
		return false;
	}

	if (!PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Second argument is not callable. "
					"Expected a file descriptor and a callback" );
		return false;
	}

	FileDescriptorMap & map = isRead ? g_fdReadHandlers : g_fdWriteHandlers;

	if (map.find( fd ) != map.end())
	{
		PyErr_Format( PyExc_ValueError, "Handler already registered for %d",
				fd );
		return false;
	}

	PyFileDescriptorHandler * pHandler =
		new PyFileDescriptorHandler( pCallback, pFileOrSocket );

	bool result = isRead ?
		g_pDispatcher->registerFileDescriptor( fd, pHandler, name ) :
		g_pDispatcher->registerWriteFileDescriptor( fd, pHandler, name );

	if (!result)
	{
		delete pHandler;
		PyErr_SetString( PyExc_TypeError, "Invalid file descriptor" );
		return false;
	}

	map[ fd ] = pHandler;
	return true;
}


/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 */
bool deregisterFileDescriptorCommon( PyObject * pFileOrSocket, bool isRead )
{
	int fd = -1;

	if (!extractFileDescriptor( pFileOrSocket, fd ))
	{
		return false;
	}

	FileDescriptorMap & map = isRead ? g_fdReadHandlers : g_fdWriteHandlers;

	FileDescriptorMap::iterator iter = map.find( fd );

	if (iter == map.end())
	{
		PyErr_Format( PyExc_ValueError, "No handler registered for %d", fd );
		return false;
	}

	delete iter->second;
	map.erase( iter );

	bool result = isRead ?
		g_pDispatcher->deregisterFileDescriptor( fd ) :
		g_pDispatcher->deregisterWriteFileDescriptor( fd );

	if (!result)
	{
		NETWORK_WARNING_MSG( "PyNetwork::deregisterFileDescriptorCommon: "
				"Failed to deregister file descriptor. isRead = %d\n", isRead );
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: Read file descriptor methods
// -----------------------------------------------------------------------------

/*~ function BigWorld.registerFileDescriptor
 *	@components{ base, cell, db }
 *
 *	This function registers a callback that will be called whenever there is
 *	data for reading on the input file descriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 *	@param callback A callable object that expects the fileDescriptor object as
 *		its only argument.
 *	@param name			A name for the file descriptor, for debugging.
 */
/**
 *	This method registers a callback that will be called whenever there is data
 *	for reading on the input file descriptor.
 *
 *	@param pFileOrSocket The file descriptor.
 *	@param pCallback	The callback that will be called when there is data to be
 *		read from the file descriptor. The callback will be called with the
 *		file descriptor object passed in.
 *	@param name			A name for the file descriptor, for debugging.
 */
bool registerFileDescriptor( PyObjectPtr pFileOrSocket, PyObjectPtr pCallback,
	const BW::string & name )
{
	return registerFileDescriptorCommon( pFileOrSocket.get(),
			pCallback.get(), name.c_str(), true );
}
PY_AUTO_MODULE_FUNCTION( RETOK, registerFileDescriptor,
		ARG( PyObjectPtr, ARG( PyObjectPtr, 
			OPTARG( BW::string, "Unnamed Python", END ) ) ), BigWorld )


/*~ function BigWorld.deregisterFileDescriptor
 *	@components{ base, cell, db }
 *
 *	This function deregisters a callback that has been registered with
 *	BigWorld.registerFileDescriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 */
/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 *
 *	@param pFileOrSocket The file descriptor to deregister.
 */
bool deregisterFileDescriptor( PyObjectPtr pFileOrSocket )
{
	return deregisterFileDescriptorCommon( pFileOrSocket.get(), true );
}
PY_AUTO_MODULE_FUNCTION( RETOK,
	deregisterFileDescriptor, ARG( PyObjectPtr, END ), BigWorld )


// -----------------------------------------------------------------------------
// Section: Write file descriptor methods
// -----------------------------------------------------------------------------

/*~ function BigWorld.registerWriteFileDescriptor
 *	@components{ base, cell, db }
 *
 *	This function registers a callback that will be called when the input file
 *	descriptor is available for writing.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 *	@param callback A callable object that expects the fileDescriptor object as
 *		its only argument.
 *	@param name			A name for the file descriptor, for debugging.
 */
/**
 *	This method registers a callback that will be called when the input file
 *	descriptor becomes available for writing.
 *
 *	@param pFileOrSocket	The file descriptor
 *	@param pCallback	The callback that will be called when the file
 *		descriptor is available for writing. The callable object expects the
 *		pFileOrSocket object as its only argument.
 *	@param name			A name for the file descriptor, for debugging.
 */
bool registerWriteFileDescriptor( PyObjectPtr pFileOrSocket,
		PyObjectPtr pCallback, const BW::string & name )
{
	return registerFileDescriptorCommon( pFileOrSocket.get(),
			pCallback.get(), name.c_str(), false );
}
PY_AUTO_MODULE_FUNCTION( RETOK, registerWriteFileDescriptor,
		ARG( PyObjectPtr, ARG( PyObjectPtr,
			OPTARG( BW::string, "Unnamed Python", END ) ) ), BigWorld )


/*~ function BigWorld.deregisterWriteFileDescriptor
 *	@components{ base, cell }
 *
 *	This function deregisters a callback that has been registered with
 *	BigWorld.registerWriteFileDescriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 */
/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 *
 *	@param pFileOrSocket The file descriptor to deregister.
 */
bool deregisterWriteFileDescriptor( PyObjectPtr pFileOrSocket )
{
	return deregisterFileDescriptorCommon( pFileOrSocket.get(), false );
}
PY_AUTO_MODULE_FUNCTION( RETOK,
	deregisterWriteFileDescriptor, ARG( PyObjectPtr, END ), BigWorld )


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/*~ function BigWorld.address
 *	@components{ base, cell, db }
 *
 *	This method returns the address of the internal network interface.
 */
/**
 *	This function returns a tuple containing the address of the network
 *	interface.
 */
PyObject * address()
{
	if (g_pInterface == NULL)
	{
		PyErr_SetString( PyExc_SystemError, "PyNetwork not yet initialised" );
		return NULL;
	}

	const Mercury::Address & addr = g_pInterface->address();

	return Py_BuildValue( "si", addr.ipAsString(), ntohs( addr.port ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, address, END, BigWorld )



#define __DOC__getNetworkInterfaces \
"getNetworkInterfaces( [netmask] ) -> tuple of tuples of (ip, interface)\n\n" \
"Returns a tuple of network interfaces that are available for the current host, " \
"optionally filtered by a CIDR netmask of form 'a.b.c.d/n'.\n\n" \
"Raises an OSError on error, or an empty tuple if no IPv4 interfaces could be found."


/*~ function BigWorld.getNetworkInterfaces
 *	@components{ cell, base, db }
 *
 *  This function returns a tuple of network interfaces that are available 
 *  for the current host, optionally filtered by a CIDR netmask. Note that
 *  only IPv4 addresses are supported.
 *
 *	@{
 *	>>>	BigWorld.getNetworkInterfaces()
 *	(('192.168.1.1', 'eth0'), ('213.40.0.102', 'eth1'), ('127.0.0.1', 'lo'))
 *	>>>	BigWorld.getNetworkInterfaces( '192.168.0.0/16' )
 *	(('192.168.1.1', 'eth0'),)
 *	@}
 *	
 *	Raises an OSError if the system ioctl call returns an error.   
 *
 *  @param netmask  an (optional) netmask of form "a.b.c.d/#bits".
 *	@return         a tuple of tuples of form: (IP address, interface name),
 *                  or an empty tuple if there are no network interfaces.
 */
/**
 *  This function returns a Python tuple of network interfaces for the 
 *  current host, optionally filtered by a (CIDR) netmask of form "a.b.c.d/#bits".
 *  Note that only IPv4 addresses are supported.
 * 
 *  @see Endpoint::getInterfaces
 */
static PyObject * getNetworkInterfaces( const BW::string & netmask )
{
    // get map of interfaces, filtered by netmask if given
    Endpoint endpoint;
    endpoint.socket( SOCK_DGRAM );
    Endpoint::InterfaceMap interfaceMap;

    if (!endpoint.getInterfaces( interfaceMap, netmask.c_str() ))
    {
        PyErr_Format( PyExc_OSError, "No interfaces" );
        return NULL;
    }

    // pack into a python tuple of (ip address, interface) tuples.
    uint32 interfaceIndex = 0;
    char ipAddr[ INET_ADDRSTRLEN ];
    Endpoint::InterfaceMap::iterator it;
    PyObject * tupleOfinterfaceTuples = PyTuple_New( interfaceMap.size() );

    for (it = interfaceMap.begin(); it != interfaceMap.end(); ++it)
    {
        if (inet_ntop( AF_INET, &(it->first), ipAddr, INET_ADDRSTRLEN ) == NULL)
        {
            PyErr_Format( PyExc_OSError, "%s", strerror( errno ) );
            return NULL;
        }

        PyObject * interfaceTuple = PyTuple_Pack( 2,
            PyString_FromString( ipAddr ), 
            PyString_FromString( it->second.c_str() ) 
        );

        PyTuple_SetItem( tupleOfinterfaceTuples, interfaceIndex++, interfaceTuple );
    }

    return tupleOfinterfaceTuples;
}

PY_AUTO_MODULE_FUNCTION_WITH_DOC( RETOWN, 
    getNetworkInterfaces, OPTARG( BW::string, "", END),
    BigWorld, __DOC__getNetworkInterfaces )


} // namespace PyNetwork

BW_END_NAMESPACE

// py_network.cpp
