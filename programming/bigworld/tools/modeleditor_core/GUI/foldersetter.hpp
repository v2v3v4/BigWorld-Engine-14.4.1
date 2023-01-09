#ifndef FOLDER_SETTER_HPP
#define FOLDER_SETTER_HPP

BW_BEGIN_NAMESPACE

class FolderSetter
{
	static const int MAX_PATH_SIZE = 8192;
	char envFolder_[ MAX_PATH_SIZE ];
	char curFolder_[ MAX_PATH_SIZE ];
public:
	FolderSetter()
	{
		BW_GUARD;

		GetCurrentDirectory( MAX_PATH_SIZE, envFolder_ );
		GetCurrentDirectory( MAX_PATH_SIZE, curFolder_ );
	}
	void enter()
	{
		BW_GUARD;

		GetCurrentDirectory( MAX_PATH_SIZE, envFolder_ );
		SetCurrentDirectory( curFolder_ );
	}
	void leave()
	{
		BW_GUARD;

		GetCurrentDirectory( MAX_PATH_SIZE, curFolder_ );
		SetCurrentDirectory( envFolder_ );
	}
};

class FolderGuard
{
	FolderSetter& setter_;
public:
	FolderGuard( FolderSetter& setter ) : setter_( setter )
	{
		BW_GUARD;

		setter_.enter();
	}
	~FolderGuard()
	{
		BW_GUARD;

		setter_.leave();
	}
};

BW_END_NAMESPACE
#endif//FOLDER_SETTER_HPP
