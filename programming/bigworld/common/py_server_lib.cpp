#include "script/first_include.hpp"

#include "cstdmf/watcher.hpp"
#include "pyscript/script.hpp"
#include "server/bwconfig.hpp"
#include "pyscript/py_data_section.hpp"


BW_BEGIN_NAMESPACE

/*~ module BWConfig
 *	@components{ base, cell, db, bots }
 */

// -----------------------------------------------------------------------------
// Section: Method definitions
// -----------------------------------------------------------------------------

/*~ function BWConfig.getSections
 *	@components{ base, cell, db, bots }
 *
 *	This method returns a list of sections matching the section name
 *	from within bw.xml and its parent files.
 *
 *	@param	sectionName	The name of the section to get.
 */
static PyObject * getSections( const BW::string & sectionName )
{
	BWConfig::SearchIterator iSectionConfig =
		BWConfig::beginSearchSections( sectionName.c_str() );

	ScriptList scriptList = ScriptList::create();

	while (iSectionConfig != BWConfig::endSearch())
	{
		ScriptObject ds( new PyDataSection( *iSectionConfig ),
			ScriptObject::STEAL_REFERENCE );
		scriptList.append( ds );

		++iSectionConfig;
	}

	return scriptList.newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getSections,
		ARG( BW::string, END ), BWConfig )


		
/*~ function BWConfig.getSection
 *	@components{ base, cell, db, bots }
 *
 *	This method returns the first section matching the path
 *	from within bw.xml and its parent files.
 *
 *	@param	sectionName	The path of the section to get.
 */
static PyObject * getSection( const BW::string & sectionPath )
{
	DataSectionPtr pDataSection = BWConfig::getSection( sectionPath.c_str() );
	if ( !pDataSection )
	{
		PyErr_Format( PyExc_ValueError,
			"Section %s not found", sectionPath.c_str() );
        return NULL;
	}
	return new PyDataSection( pDataSection );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, getSection,
		ARG( BW::string, END ), BWConfig )


/*~ function BWConfig.readString
 *	@components{ base, cell, db, bots }
 *
 *	This method returns the given value from bw.xml.
 *
 *	@param	configOption	The path of the option to get.
 */
static PyObject * readString( const BW::string & configOption,
		const BW::string & defaultValue )
{
	return Script::getData( BWConfig::get( configOption.c_str(),
				defaultValue ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, readString,
		ARG( BW::string, OPTARG( BW::string, "", END ) ), BWConfig )


/*~ function BWConfig.readInt
 *	@components{ base, cell, db, bots }
 *
 *	This method returns the given value from bw.xml.
 *
 *	@param	configOption	The path of the option to get.
 */
static PyObject * readInt( const BW::string & configOption,
		int defaultValue )
{
	return Script::getData( BWConfig::get( configOption.c_str(),
				defaultValue ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, readInt,
		ARG( BW::string, OPTARG( int, 0, END ) ), BWConfig )


/*~ function BWConfig.readFloat
 *	@components{ base, cell, db, bots }
 *
 *	This method returns the given value from bw.xml.
 *
 *	@param	configOption	The path of the option to get.
 */
static PyObject * readFloat( const BW::string & configOption,
		double defaultValue )
{
	return Script::getData( BWConfig::get( configOption.c_str(),
				defaultValue ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, readFloat,
		ARG( BW::string, OPTARG( double, 0.0, END ) ), BWConfig )


/*~ function BWConfig.readBool
 *	@components{ base, cell, db, bots }
 *
 *	This method returns the given value from bw.xml.
 *
 *	@param	configOption	The path of the option to get.
 */
static PyObject * readBool( const BW::string & configOption,
		bool defaultValue )
{
	return Script::getData( BWConfig::get( configOption.c_str(),
				defaultValue ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, readBool,
		ARG( BW::string, OPTARG( bool, false, END ) ), BWConfig )


BW_END_NAMESPACE

// py_server.cpp
