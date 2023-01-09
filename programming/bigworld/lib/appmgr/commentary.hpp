#ifndef COMMENTARY_HPP
#define COMMENTARY_HPP

#include "cstdmf/bw_string.hpp"

#include <iostream>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides the global interface for commentary.
 *	The commentary class is also a useful hub for runtime
 *	debugging information.
 */
class Commentary
{
public:
	~Commentary();

	//Accessors
	static Commentary & instance();

	enum LevelId
	{
		COMMENT = 0,
		CRITICAL = 1,
		ERROR_LEVEL = 2,
		WARNING = 3,
		HACK = 4,
		SCRIPT_ERROR = 5,
		NUM_LEVELS
	};

	void	addMsg( const BW::string & msg, int id = COMMENT );
	void	addMsg( const BW::wstring & msg, int id = COMMENT );

	//callbacks
	class View
	{
	public:
		virtual void onAddMsg( const BW::wstring & msg, int id ) = 0;
	};

	void	addView( Commentary::View * view );
	void	delView( Commentary::View * view );

private:
	Commentary();

	Commentary( const Commentary& );
	Commentary& operator=( const Commentary& );

	typedef BW::vector< View* > Views;
	Views	views_;


	friend std::wostream& operator<<( std::wostream&, const Commentary& );
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "commentary.ipp"
#endif

#endif // COMMENTARY_HPP
