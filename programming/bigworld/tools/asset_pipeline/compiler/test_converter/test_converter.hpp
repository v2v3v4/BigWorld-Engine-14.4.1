#ifndef TEST_CONVERTER_HPP
#define TEST_CONVERTER_HPP

#include "asset_pipeline/conversion/converter.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class TestConverter : public Converter
{
protected:
	TestConverter( const BW::string& params );
	~TestConverter();

public:
	virtual bool createDependencies( const BW::string& sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies );

	/* convert a source file. */
	virtual bool convert( const BW::string& sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles );

public:
	static uint64 getTypeId() { return s_TypeId; }
	static const char * getVersion() { return s_Version; }
	static const char * getTypeName() { return "TestConverter"; }
	static Converter * createConverter( const BW::string& params ) { return new TestConverter( params ); } 

	static const uint64 s_TypeId;
	static const char * s_Version;

};

BW_END_NAMESPACE
#endif
