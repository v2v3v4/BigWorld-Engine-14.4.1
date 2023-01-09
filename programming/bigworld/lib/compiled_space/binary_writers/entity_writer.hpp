#ifndef ENTITY_WRITER_HPP
#define ENTITY_WRITER_HPP

#include "space_writer.hpp"
#include "entity_aggregator.hpp"

namespace BW {
namespace CompiledSpace {

class EntityWriter :
	public ISpaceWriter
{
public:
	EntityWriter();
	~EntityWriter();

	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual bool write( BinaryFormatWriter& writer );

	void convertEntity( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void convertUDO( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );

	void addEntity( const DataSectionPtr& pItemDS, const Matrix& baseTransform );
	void addUDO( const DataSectionPtr& pItemDS, const Matrix& baseTransform);

private:
	
	EntityAggregator clientEntities_;
	EntityAggregator serverEntities_;
	EntityAggregator udos_;
	bool includeServerEntities_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // ENTITY_WRITER_HPP
