#ifndef CONFIG_READER_HPP
#define CONFIG_READER_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class provides an interface for reading Windows INI style
 * configuration files as used by the server tools.
 */
class ConfigReader
{
public:
	ConfigReader( const char *filename );
	~ConfigReader();

	bool read();

	bool getValue( const BW::string section, const BW::string key,
				BW::string &value ) const;

	static void separateLine( const BW::string &line, char sep,
				BW::vector< BW::string > &resultList );

	const char *filename() const { return filename_.c_str(); }

private:
	ConfigReader();

	typedef BW::map< BW::string, BW::string > SectionEntries;
	typedef BW::map< BW::string, SectionEntries * > Sections;

	Sections sections_;

	BW::string filename_;
	char separator_;

	bool handleLine( const char *line, SectionEntries* & currSection );
};

BW_END_NAMESPACE

#endif // CONFIG_READER_HPP
