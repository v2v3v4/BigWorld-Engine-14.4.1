#ifndef ASSET_PIPELINE_HIERARCHICAL_CONFIG_CONVERTER
#define ASSET_PIPELINE_HIERARCHICAL_CONFIG_CONVERTER

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "asset_pipeline/conversion/converter.hpp"

BW_BEGIN_NAMESPACE

class HierarchicalConfigConverter : public Converter
{
protected:
	HierarchicalConfigConverter( const BW::string& params );
	~HierarchicalConfigConverter();

public:
	virtual bool createDependencies( const BW::string & sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies );

	virtual bool convert( const BW::string & sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles );

private:
	StringRef configFile_;
	StringRef configDirectory_;
	StringRef outputFile_;

public:
	static uint64 getTypeId() { return s_TypeId; }
	static const char * getVersion() { return s_Version; }
	static const char * getTypeName() { return "HierarchicalConfigConverter"; }
	static Converter * createConverter( const BW::string& params ) { return new HierarchicalConfigConverter( params ); }

	static const uint64 s_TypeId;
	static const char * s_Version;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_HIERARCHICAL_CONFIG_CONVERTER
