#ifndef __PACKERS_HPP__
#define __PACKERS_HPP__


#include <string>
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_vector.hpp"
#include "base_packer.hpp"


BW_BEGIN_NAMESPACE

/**
 *  Packers
 */
class Packers
{
private:
	typedef std::pair<unsigned short,BasePacker*> Item;
	typedef BW::vector<Item> Items;

public:
	static const unsigned short HIGHEST_PRIORITY	= 0;
	static const unsigned short LOWEST_PRIORITY		= 0xFFFF;

	static Packers& instance();

	void add( BasePacker* packer, unsigned short priority );

	BasePacker* find( const BW::string& src, const BW::string& dst );

private:
	Packers() {};

	Items packers_;
};


class PackerFactory
{
public:
	PackerFactory( BasePacker* packer, unsigned short priority = Packers::HIGHEST_PRIORITY )
	{
		Packers::instance().add( packer, priority );
	}
};

#define DECLARE_PACKER()		\
	static PackerFactory s_packer_factory_;
#define IMPLEMENT_PACKER( P )	\
	PackerFactory P::s_packer_factory_( new P() );

#define IMPLEMENT_PRIORITISED_PACKER( P, PRIORITY )		\
	PackerFactory P::s_packer_factory_( new P(), PRIORITY );


BW_END_NAMESPACE

#endif // __PACKERS_HPP__
