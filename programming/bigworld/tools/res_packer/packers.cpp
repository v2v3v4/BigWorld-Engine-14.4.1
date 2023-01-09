
#include "packers.hpp"


BW_BEGIN_NAMESPACE

Packers& Packers::instance()
{
	static Packers s_instance_;
	return s_instance_;
}

void Packers::add( BasePacker* packer, unsigned short priority )
{
	if ( !packer )
		return;

	// insert ordered by priority
	Items::iterator i = packers_.begin();
	for( ; i != packers_.end(); ++i )
	{
		if ( (*i).first > priority )
			break;
	}
	packers_.insert( i, Item( priority, packer ) );
}

BasePacker* Packers::find( const BW::string& src, const BW::string& dst )
{
	// return the first packer that can handle the data
	for( Items::iterator i = packers_.begin();
		i != packers_.end(); ++i )
	{
		if ( (*i).second->prepare( src, dst ) )
			return (*i).second;
	}
	return NULL;
}

BW_END_NAMESPACE
