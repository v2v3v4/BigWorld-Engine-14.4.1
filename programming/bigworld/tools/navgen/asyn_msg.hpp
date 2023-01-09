#ifndef ASYN_MSG_HPP
#define ASYN_MSG_HPP


BW_BEGIN_NAMESPACE

class AsyncMessage
{
public:
	void reportMessage( const wchar_t* msg, bool severity );
	void show();
	void hide();
	bool isShow() const;
	HWND handle();
	const wchar_t* getLogFileName() const;
	void printMsg( const wchar_t * msg ) const;

private:
	BW::wstring dateMsg( const wchar_t * msg ) const;
	void writeToLog( const wchar_t * msg ) const;
};

BW_END_NAMESPACE

#endif//ASYN_MSG_HPP
