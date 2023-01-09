#ifndef SERVER_DISCOVERY_HPP
#define SERVER_DISCOVERY_HPP


#include "cstdmf/main_loop_task.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/stl_to_py.hpp"

BW_BEGIN_NAMESPACE

class Endpoint;
namespace Mercury
{
	class NetworkInterface;
}
/*~ class BigWorld.ServerDiscovery
 *  This class defines an object which can be used to locate BigWorld servers
 *  on a local area network and obtain information regarding their current 
 *  status. An instance of this class is automatically created by BigWorld as 
 *  BigWorld.serverDiscovery, and no further instances can be created by 
 *  script. When the searching attribute is set to a value other than 0, where
 *  previously it had been equal to 0, the ServerDiscovery broadcasts a request
 *  on the LAN for servers to provide it with their details. These details are 
 *  not received all at once, and may be updated periodically. Each time 
 *  details are received or updated the function assigned to the changeNotifier
 *  attribute is called. This continues to collect server details until the 
 *  searching attribute is set to 0. When the searching attribute is assigned 
 *  to  0 all of the server details which have been collected are discarded. As
 *  this can only be used over a LAN it would not be very useful for a live 
 *  MMOG, however it is very useful during testing and development.
 *
 *  Code Example:
 *  @{
 *  # This example prints out the list of known server host names each time
 *  # the list is updated.
 *
 *  # imports
 *  import BigWorld
 *
 *  # print the host names of all the known servers
 *  def listServers():
 *      print "\nServers:"   # print header
 *      for s in BigWorld.serverDiscovery.servers:
 *          print s.hostName   # print entry
 *
 *  # set the server list callback function
 *  BigWorld.serverDiscovery.changeNotifier = listServers
 *
 *  # initiate the search for servers
 *  BigWorld.serverDiscovery.searching = 1
 *  @}
 */
/**
 *	This class discovers servers and their details, in its own time.
 */
class ServerDiscovery : public MainLoopTask, public PyObjectPlus
{
	Py_Header( ServerDiscovery, PyObjectPlus )

public:
	ServerDiscovery( PyTypeObject * pType = &s_type_ );
	~ServerDiscovery();

	bool init()				{ return true; }
	void tick( float dGameTime, float dRenderTime );
	void fini();

	void searching( bool go );
	bool searching()		{ return searching_; }

	/*~ class BigWorld.ServerDiscoveryDetails
	 *  ServerDiscovery::Details defines an object which describes the current
	 *  status of a BigWorld server as might be considered useful in a game lobby.
	 *  Instances of this class are and can only be created by the
	 *  BigWorld.serverDiscovery object. All attributes of this object are
	 *  read-only.
	 */
	/**
	 *	This class used to expose the details of a BigWorld server to client
	 *	script.
	 */
	class Details : public PyObjectPlus
	{
		Py_Header( Details, PyObjectPlus )

	public:
		Details( PyTypeObject * pType = &s_type_ );

		BW::string	hostName_;
		uint32		ip_;
		uint16		port_;
		uint16		uid_;

		int			ownerValid_:1;
		int			usersCountValid_:1;
		BW::string	ownerName_;
		uint		usersCount_;

		PyObject * pyRepr();

		BW::string getServerString() const;

		PY_RO_ATTRIBUTE_DECLARE( hostName_, hostName )
		PY_RO_ATTRIBUTE_DECLARE( ip_, ip )
		PY_RO_ATTRIBUTE_DECLARE( port_, port )
		PY_RO_ATTRIBUTE_DECLARE( uid_, uid )
		PY_RO_ATTRIBUTE_DECLARE(	\
			ownerValid_?ownerName_:BW::string(), ownerName )
		PY_RO_ATTRIBUTE_DECLARE(	\
			usersCountValid_?int(usersCount_):-1, usersCount )

		PY_RO_ATTRIBUTE_DECLARE( getServerString(), serverString );
	};

	typedef SmartPointer<Details> DetailsPtr;

	/**
	 *	This class implements an iterator that can be used to iterate over the
	 *	details of BigWorld servers that have been discovered.
	 */
	class iterator
	{
	public:
		iterator( ServerDiscovery & sd, uint idx ) :
		  sd_( sd ), idx_( idx )	{ }

		void operator++( int )		{ ++idx_; }

		Details * operator->()		{ return &*sd_.details( idx_ ); }
		Details & operator*()		{ return *sd_.details( idx_ ); }

	private:
		ServerDiscovery & sd_;
		uint	idx_;
	};

	DetailsPtr details( uint idx )
		{ return idx < details_.size() ? details_[idx] : NULL; }
	PyObject * changeNotifier()
		{ return changeNotifier_.getObject(); }

	PY_RO_ATTRIBUTE_DECLARE( detailsHolder_, servers )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, searching, searching )
	PY_RW_ATTRIBUTE_DECLARE( changeNotifier_, changeNotifier )

private:
	ServerDiscovery( const ServerDiscovery& );
	ServerDiscovery& operator=( const ServerDiscovery& );

	void sendFindMGM();
	bool recv();


	Endpoint *					pEP_;
	Mercury::NetworkInterface *	pNI_;

	typedef BW::vector< DetailsPtr >		DetailsVector;
	DetailsVector	details_;
	PySTLSequenceHolder< DetailsVector >	detailsHolder_;

	bool			searching_;

	SmartPointer<PyObject>		changeNotifier_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ServerDiscovery::Details )

BW_END_NAMESPACE

#endif // SERVER_DISCOVERY_HPP
