#ifndef URL_STRING_LIST_HPP
#define URL_STRING_LIST_HPP

#include "urlrequest/misc.hpp"

struct curl_slist;

BW_BEGIN_NAMESPACE

namespace URL
{

class StringList
{
public:
	StringList():
			pValue_( NULL )
	{}

	~StringList()
		{ this->clear(); }


	void add( const BW::string & s );

	void add( Headers & headers )
	{
		Headers::iterator iter = headers.begin();

		while (iter != headers.end())
		{
			this->add( *iter );

			++iter;
		}
	}

	curl_slist* value() const
		{ return pValue_; }

	void clear();

private:
	curl_slist * pValue_;
};

} // namespace URL

BW_END_NAMESPACE

#endif // URL_STRING_LIST_HPP
