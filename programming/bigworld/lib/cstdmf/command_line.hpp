#ifndef COMMAND_LINE_HPP__
#define COMMAND_LINE_HPP__

#include "cstdmf/bw_namespace.hpp"

#include "bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class CommandLine
{
	typedef BW::vector< BW::string > Params;
	Params params_;

public:
	CSTDMF_DLL CommandLine( const char* commandLine );

	CSTDMF_DLL bool hasParam( const char* param,
		const char* commandSwitch = "/-" ) const;

	CSTDMF_DLL const char* getParam( const char* param,
		const char* commandSwitch = "/-" ) const;

	CSTDMF_DLL bool	hasFullParam( const char* paramToFind,
		int paramIndex = -1 ) const;

	CSTDMF_DLL const char* getFullParam( const char* paramToFind,
		int paramIndex = -1 ) const;

	CSTDMF_DLL const char* getParamByIndex( int paramIndex ) const;

	CSTDMF_DLL static bool hasSwitch( const char* param,
		const char* commandSwitch = "/-" );

	CSTDMF_DLL size_t size() const;
	CSTDMF_DLL const BW::string& param( size_t index ) const;
};

BW_END_NAMESPACE

#endif//COMMAND_LINE_HPP__
