#ifndef KEYWORD_PARSER
#define KEYWORD_PARSER


#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE


/**
 *	Base class for keyword arguments.
 */
class KeywordArgumentBase
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~KeywordArgumentBase() {}

	/**
	 *	This method is used to pull an argument from the given dictionary.
	 *
	 *	@param dict 	The dictionary with the keyword arguments.
	 *	@param exceptionOccurred
	 *					If an exception is raised while parsing, this will be
	 *					set to true and the method will return false.
	 *					Otherwise, this will not be modified.
	 *	@return 		true if the argument value was successfully parsed,
	 *					false otherwise.
	 */
	virtual bool getFromDict( PyObject * dict, bool & exceptionOccurred ) = 0;

	/**
	 *	This returns the name of the keyword argument.
	 */
	const string & name() const { return name_; }

protected:
	/**
	 *	Constructor.
	 *
	 *	@param name 	The name of the keyword.
	 *	@param value 	The output value.
	 */
	KeywordArgumentBase( const string & name ) :
			name_( name )
	{}

protected:
	string name_;
};


/**
 *	Template class for keyword arguments.
 */
template< typename T >
class KeywordArgument : public KeywordArgumentBase
{
public: 

	/**
	 *	Constructor.
	 *
	 *	@param name 	The name of the keyword.
	 *	@param value 	The output value.
	 */
	KeywordArgument( const string & name, T & value ):
			KeywordArgumentBase( name ),
			value_( value )
	{}


	/*
	 *	Override from KeywordArgumentBase.
	 */
	virtual bool getFromDict( PyObject * dict, bool & exceptionOccurred )
	{
		if (!PyMapping_HasKeyString( dict, (char *)name_.c_str() ))
		{
			return false;
		}

		PyObject * item = PyDict_GetItemString( dict, (char *)name_.c_str() );

		if (-1 == Script::setData( item, value_, name_.c_str() ))
		{
			exceptionOccurred = true;
			return false;
		}

		return true;
	}


private:
	T & value_;
};


/**
 *	This class parses keywords out of Python dictionaries into types that have
 *	a Script::setData() converter function defined.
 */
class KeywordParser
{
public:

	/**
	 *	Constructor.
	 */
	KeywordParser():
			arguments_()
	{}


	/**
	 *	Destructor.
	 */
	~KeywordParser()
	{
		for (Arguments::iterator iArg = arguments_.begin();
				iArg != arguments_.end();
				++iArg)
		{
			delete iArg->second;
		}
	}


	/**
	 *	This method adds a keyword argument.
	 *
	 *	@param name	 	The keyword name.
	 *	@param value 	The output value.
	 */
	template< typename T >
	void add( const BW::string & name, T & value )
	{
		arguments_.insert( Arguments::value_type( name,
			new KeywordArgument< T >( name, value ) ) );
	}


	enum ParseResult
	{
		NONE_FOUND,			///< No keywords were found.
		EXCEPTION_RAISED, 	///< An exception was raised while parsing.
		SOME_MISSING, 		///< Some keywords were missing.
		ALL_FOUND 			///< All keywords were found.
	};

	/**
	 *	This method parses a given dictionary.
	 *
	 *	@param pDict 			The dictionary to parse keyword arguments from.
	 *	@param shouldRemove		Whether to remove the entries as they are found.
	 *	@param allowOtherArguments
	 *							If false, an exception is raised (and
	 *							EXCEPTION_RAISED) is returned if there are
	 *							other arguments not added.
	 *
	 *	@return 				One of the values of ParseResult.
	 */
	ParseResult parse( PyObject * pDict, bool shouldRemove = true,
			bool allowOtherArguments = true )
	{
		if (!pDict)
		{
			return NONE_FOUND;
		}

		uint numFound = 0;

		for (Arguments::iterator iArg = arguments_.begin();
				iArg != arguments_.end();
				++iArg)
		{
			bool didExceptionOccur = false;
			bool wasPresent = iArg->second->getFromDict( pDict,
				didExceptionOccur );

			if (didExceptionOccur)
			{
				return EXCEPTION_RAISED;
			}

			if (wasPresent)
			{
				if (shouldRemove)
				{
					PyDict_DelItemString( pDict, 
						(char *)(iArg->first.c_str()) );
				}
				++numFound;
			}
		}

		if (!allowOtherArguments)
		{
			Py_ssize_t i = 0;
			PyObject * key = NULL;
			while (PyDict_Next( pDict, &i, &key, NULL ))
			{
				if (shouldRemove ||
						(arguments_.count(
							string( PyString_AsString( key ) ) ) == 0))
				{
					PyErr_Format( PyExc_TypeError,
						"Invalid keyword argument: \"%s\"",
						PyString_AsString( key ) );

					return EXCEPTION_RAISED;
				}
			}
		}

		if (numFound == 0)
		{
			return NONE_FOUND;
		}
		else if (numFound < arguments_.size())
		{
			return SOME_MISSING;
		}
		else
		{
			return ALL_FOUND;
		}
	}


private:
	typedef map< string, KeywordArgumentBase * > Arguments;
	Arguments arguments_;
};


BW_END_NAMESPACE


#endif // KEYWORD_PARSER>HPP
