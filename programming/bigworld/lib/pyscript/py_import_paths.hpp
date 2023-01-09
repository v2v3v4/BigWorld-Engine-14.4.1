#ifndef PY_IMPORT_PATHS_HPP
#define PY_IMPORT_PATHS_HPP

#include <Python.h>

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	A helper class to generate a Python import paths from path components.
 */
class PyImportPaths
{
public:
	static const char DEFAULT_DELIMITER = ';';

	PyImportPaths( char delimiter = DEFAULT_DELIMITER );

	const BW::string pathsAsString() const;
	PyObject * pathAsObject() const;

	bool empty() const 
	{ return paths_.empty(); }

	void append( const PyImportPaths & other );

	void addResPath( const BW::string & path );
	void addNonResPath( const BW::string & path );

	void setDelimiter( char delimiter )
	{ delimiter_ = delimiter; }

private:
	typedef BW::vector< BW::string > OrderedPaths;
	OrderedPaths paths_;
	char delimiter_;
};

BW_END_NAMESPACE

#endif // PY_IMPORT_PATHS_HPP
