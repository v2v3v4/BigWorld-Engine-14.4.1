#ifndef SETTINGS_WRITER_HPP
#define SETTINGS_WRITER_HPP

#include "../compiled_space_settings_types.hpp"
#include "space_writer.hpp"

#include "resmgr/datasection.hpp"

namespace BW {

class CommandLine;

namespace CompiledSpace {

class BinaryFormatWriter;

class SettingsWriter :
	public ISpaceWriter
{
public:
	SettingsWriter();
	~SettingsWriter();
	
	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter& writer );

	typedef std::tr1::function< const AABB() > BoundGeneratorFunc;

	void addBounds( const AABB& bb );
	void addBoundGenerator( BoundGeneratorFunc callback );

private:
	CompiledSpaceSettingsTypes::Header header_;
	typedef BW::vector< BoundGeneratorFunc > GeneratorList;
	GeneratorList generators_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // SETTINGS_WRITER_HPP
