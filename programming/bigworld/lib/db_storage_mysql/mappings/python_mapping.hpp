#ifndef MYSQL_PYTHON_MAPPING_HPP
#define MYSQL_PYTHON_MAPPING_HPP

#include "string_like_mapping.hpp"


BW_BEGIN_NAMESPACE

class Pickler;

/**
 *	This class maps the PYTHON type to the database.
 */
class PythonMapping : public StringLikeMapping
{
public:
	PythonMapping( const Namer & namer, const BW::string & propName,
			int length, DataSectionPtr pDefaultValue );

	virtual bool isBinary() const	{ return true; }

private:
	// This method evaluates expr as a Python expression, pickles the
	// resulting object and stores it in output.
	static void pickleExpression( BW::string & output,
			const BW::string & expr );

	static Pickler & getPickler();
};

BW_END_NAMESPACE

#endif // MYSQL_PYTHON_MAPPING_HPP
