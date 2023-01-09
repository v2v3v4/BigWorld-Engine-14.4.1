#ifndef SERVICES_MAP_HPP
#define SERVICES_MAP_HPP

#include "cstdmf/bw_string.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE


class BinaryIStream;


/**
 *	This class represents a mapping of services and mailboxes to its fragments.
 */
class ServicesMap
{
private:
	typedef BW::multimap< BW::string, EntityMailBoxRef > Fragments;

public:

	ServicesMap();
	~ServicesMap();

	typedef Fragments::const_iterator const_iterator;

	const_iterator begin() const { return fragments_.begin(); }
	const_iterator end() const { return fragments_.end(); }

	uint size() const { return fragments_.size(); }

	bool empty() const { return fragments_.empty(); }

	void addFragment( const BW::string & serviceName, 
			const EntityMailBoxRef & mailbox );

	int removeFragment( const BW::string & serviceName,
		const Mercury::Address & address, bool * pDidLoseService = NULL );

	typedef BW::set< BW::string > Names;
	int removeFragmentsForAddress( const Mercury::Address & address,
		Names * pLostServices = NULL );

	void getServiceNames( Names & names ) const;

	bool chooseFragment( const BW::string & serviceName,
		EntityMailBoxRef & outRef ) const;

	typedef BW::vector< EntityMailBoxRef > FragmentMailBoxes;
	void getFragmentsForService( const BW::string & serviceName,
		FragmentMailBoxes & outFragments ) const;
		
private:

	int removeFragmentsForAddress( const Mercury::Address & address, 
		const Fragments::iterator & begin, const Fragments::iterator & end,
		Names * pLostServices );

	Fragments fragments_;
};


BW_END_NAMESPACE


#endif // SERVICES_MAP_HPP

