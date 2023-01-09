#include "effect_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/effect_compiler.hpp"
#include "moo/effect_helpers.hpp"
#include "moo/effect_macro_setting.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

const uint64 EffectConverter::s_TypeId = Hash64::compute( EffectConverter::getTypeName() );
const char * EffectConverter::s_Version = "2.7";

/* construct a converter with parameters. */
EffectConverter::EffectConverter( const BW::string & params ) :
	Converter( params ),
	debugSymbols_( params.find( "-debug" ) != BW::string::npos )
{
	BW_GUARD;
}

EffectConverter::~EffectConverter()
{
}

/* builds the dependency list for a source file. */
bool EffectConverter::createDependencies( const BW::string & sourcefile, 
										  const Compiler & compiler,
										  DependencyList & dependencies )
{
	BW_GUARD;

	Moo::EffectCompiler effectCompiler( false, debugSymbols_ );
	BW::list< BW::string > includes;

	BW::string resourceId = sourcefile;
	compiler.resolveRelativePath( resourceId );
	if (!effectCompiler.getIncludes( resourceId, includes ))
	{
		ERROR_MSG( "Could not get includes from effect %s\n", sourcefile.c_str() );
		return false;
	}

	for (BW::list< BW::string >::iterator it =
		includes.begin(); it != includes.end(); ++it)
	{
		compiler.resolveRelativePath( *it );
		dependencies.addPrimarySourceFileDependency( *it );
	}
	return true;
}

/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool EffectConverter::convert( const BW::string & sourcefile,
							   const Compiler & compiler,
							   BW::vector< BW::string > & intermediateFiles,
							   BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	Moo::EffectCompiler effectCompiler( false, debugSymbols_ );
	
	Moo::EffectMacroSetting::MacroSettingVector settings;
	Moo::EffectMacroSetting::getMacroSettings( settings );
	size_t numSettingCombinations = 1;
	for (Moo::EffectMacroSetting::MacroSettingVector::iterator
		it = settings.begin(); it != settings.end(); ++it)
	{
		numSettingCombinations *= (*it)->numMacroOptions();
	}
	for (size_t i = 0; i < numSettingCombinations; ++i)
	{
		size_t index = i;
		for (Moo::EffectMacroSetting::MacroSettingVector::iterator
			it = settings.begin(); it != settings.end(); ++it)
		{
			size_t option = index % (*it)->numMacroOptions();
			(*it)->selectOption((int)option);
			index = index / (*it)->numMacroOptions();
		}
		MF_ASSERT( index == 0 );
		Moo::EffectMacroSetting::updateManagerInfix();

		BW::string outputfile = Moo::EffectHelpers::fxoName( sourcefile );
		compiler.resolveOutputPath( outputfile );
		BWResource::ensureAbsolutePathExists( outputfile );

		BW::string resourceId = sourcefile;
		compiler.resolveRelativePath( resourceId );
		BinaryPtr bin = effectCompiler.compile( resourceId, NULL, &outputfile );
		if (bin == NULL)
		{
			ERROR_MSG( "Failed to compiler effect %s to %s\n", sourcefile.c_str(), outputfile.c_str() );
			return false;
		}
		outputFiles.push_back( outputfile );
	}

	return true;
}
BW_END_NAMESPACE
