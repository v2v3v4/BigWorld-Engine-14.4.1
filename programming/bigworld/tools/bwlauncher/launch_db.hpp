#ifndef LAUNCH_DB_HPP__
#define LAUNCH_DB_HPP__


class LaunchDB
{
	BW::map<tstring, tstring> applications_;

	void loadFile( tstring path, tstring file );
	void loadPath( tstring path );

public:
	LaunchDB( tstring path );

	bool launch( const tstring& gameID, const tstring& username, const tstring& password ) const;
	size_t size() const;
	std::pair<tstring, tstring> operator[]( size_t index ) const;
};


#endif//LAUNCH_DB_HPP__
