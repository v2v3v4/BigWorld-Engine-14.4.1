#ifndef DATA_SECTION_CENSUS_HPP
#define DATA_SECTION_CENSUS_HPP

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;


/**
 *	This maintains a 'census' of all named DataSections that are currently 
 *	alive. This allows quick lookup of named sections which are alive (but not
 *	necessarily in the cache anymore). It does not hold onto a strong reference
 *	to the DataSections. DataSections are not automatically added to the census 
 *	(because not all DataSections have a name), but they are automatically removed 
 *	from the census when they die.
 */
namespace DataSectionCensus
{
	DataSectionPtr find( const BW::StringRef & id );
	DataSectionPtr add( const BW::StringRef & id,
		const DataSectionPtr & pSect );
	void del( const DataSection * pSect );
	void tryDestroy( const DataSection * pSect );
	void clear();
	void init();
	void fini();
};

BW_END_NAMESPACE

#endif // DATA_SECTION_CENSUS_HPP
