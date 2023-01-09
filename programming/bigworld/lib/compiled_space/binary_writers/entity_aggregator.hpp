#ifndef DATA_SECTION_AGGREGATOR_HPP
#define DATA_SECTION_AGGREGATOR_HPP

#include "resmgr/datasection.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

struct FourCC;

namespace CompiledSpace
{
	class BinaryFormatWriter;
}

class EntityAggregator
{
public:
	typedef CompiledSpace::BinaryFormatWriter BinaryFormatWriter;

	void add( const DataSectionPtr& pDS,
		const Matrix& chunkTransform );

	bool write( const BW::string& filename,
		const BW::vector<BW::string>& filterOut );

	bool writeToBinary( BinaryFormatWriter& writer,
		const FourCC& magic, uint32 version,
		const BW::vector<BW::string>& filterOut );

private:
	struct Section
	{
		Matrix chunkTransform_;
		DataSectionPtr pDS_;
	};
	BW::vector<Section> sections_;
};

BW_END_NAMESPACE


#endif // DATA_SECTION_AGGREGATOR_HPP
