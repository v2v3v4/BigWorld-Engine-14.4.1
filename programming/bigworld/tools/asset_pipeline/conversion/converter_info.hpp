#ifndef ASSET_PIPELINE_CONVERTER_INFO
#define ASSET_PIPELINE_CONVERTER_INFO

#include "converter_creator.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

/// struct containing information about a converter in the asset pipeline.
struct ConverterInfo
{
public:
	/// the display name of the converter.
	BW::string			name_;
	/// the id of the converter. Must be unique to each type of converter.
	uint64				typeId_;
	/// the current version of the converter.
	BW::string			version_;
	/// converter flags
	enum CONVERTER_FLAGS
	{
		THREAD_SAFE			= 1 << 0, // can the converter be run on multiple threads.
		CACHE_DEPENDENCIES	= 1 << 1, // should dependencies be read and written to the cache.
		CACHE_CONVERSION	= 1 << 2, // should conversion be read and written to the cache.
		UPGRADE_CONVERSION	= 1 << 3, // is the conversion an upgrade of the source asset.
		DEFAULT_FLAGS		= THREAD_SAFE | CACHE_DEPENDENCIES | CACHE_CONVERSION,
		EXPERIMENTAL_MASK	= ~(CACHE_CONVERSION | CACHE_DEPENDENCIES)
	}					flags_;
	/// function pointer for creating an instance of the converter.
	ConverterCreator	creator_;
};

#define INIT_CONVERTER_INFO( INFO, CONVERTER, FLAGS )		\
	INFO.name_ = CONVERTER::getTypeName();					\
	INFO.typeId_ = CONVERTER::getTypeId();					\
	INFO.version_ = CONVERTER::getVersion();				\
	INFO.flags_ = (ConverterInfo::CONVERTER_FLAGS)( FLAGS );\
	INFO.creator_ = CONVERTER::createConverter;

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER_CREATOR