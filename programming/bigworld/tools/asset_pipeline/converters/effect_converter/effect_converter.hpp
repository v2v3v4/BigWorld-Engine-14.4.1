#ifndef ASSET_PIPELINE_EFFECT_CONVERTER
#define ASSET_PIPELINE_EFFECT_CONVERTER

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "asset_pipeline/conversion/converter.hpp"

BW_BEGIN_NAMESPACE

class EffectConverter : public Converter
{
protected:
	EffectConverter( const BW::string& params );
	~EffectConverter();
	bool debugSymbols_;

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
	static const char * getTypeName() { return "EffectConverter"; }
	static Converter * createConverter( const BW::string& params ) { return new EffectConverter( params ); }

	static const uint64 s_TypeId;
	static const char * s_Version;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_EFFECT_CONVERTER
