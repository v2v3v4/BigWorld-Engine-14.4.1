#ifndef MRU_HPP
#define MRU_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class MRU
{
public:
	
	MRU();

	static MRU& instance();

	void update( const BW::string& mruName, const BW::string& file, bool add = true );
	void read( const BW::string& mruName, BW::vector<BW::string>& files );
	void getDir( const BW::string& mruName, BW::string& dir, const BW::string& defaultDir = "" );

private:

	unsigned maxMRUs_;
};

BW_END_NAMESPACE

#endif // MRU_HPP
