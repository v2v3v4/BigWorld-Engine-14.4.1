#ifndef ASSET_PIPELINE_VISUAL_PROCESSOR
#define ASSET_PIPELINE_VISUAL_PROCESSOR

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "asset_pipeline/conversion/converter.hpp"

BW_BEGIN_NAMESPACE

class VisualProcessor : public Converter
{
protected:
	VisualProcessor( const BW::string& params );
	~VisualProcessor();

public:
	virtual bool createDependencies( const BW::string & sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies );

	virtual bool convert( const BW::string & sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles );

public:
	static uint64 getTypeId() { return s_TypeId; }
	static const char * getVersion() { return s_Version; }
	static const char * getTypeName() { return "VisualProcessor"; }
	static Converter * createConverter( const BW::string& params ) { return new VisualProcessor( params ); }

	static const uint64 s_TypeId;
	static const char * s_Version;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_VISUAL_PROCESSOR
