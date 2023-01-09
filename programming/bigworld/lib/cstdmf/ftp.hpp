#ifndef _FTP_HPP__
#define _FTP_HPP__

#ifndef WIN32
#error "ftp.hpp should be included on Windows only!"
#endif//WIN32

#include "cstdmf/bw_util.hpp"
#include <wininet.h>
#pragma comment(lib,"wininet.lib")

#define AGENT_NAME L"BWTech"

BW_BEGIN_NAMESPACE

// this is an ftp write only class, should only be used in error handling
class Ftp
{
	enum Status
	{
		NOT_CONNECTED,
		CONNECTED,
		FOLDER_CREATED,
		SENDING,
		COMPLETED,
		FAILURE
	};

	Status status_;
	HINTERNET inet_;
	HINTERNET ftp_;
	const wchar_t* dirName_;
	const wchar_t* removeFileName_;
	BW::wstring localFileName_;

	static void __stdcall statusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
		LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
	{
		Ftp* This = (Ftp*)dwContext;

		This->callback( hInternet, dwContext, dwInternetStatus, lpvStatusInformation, dwStatusInformationLength );
	}

	void __stdcall callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
		LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
	{
		if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
		{
			INTERNET_ASYNC_RESULT* result = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

			if (status_ == NOT_CONNECTED)
			{
				if (result->dwError == ERROR_SUCCESS)
				{
					ftp_ = (HINTERNET) result->dwResult;
					status_ = CONNECTED;
					FtpCreateDirectory( ftp_, dirName_ );
				}
				else
				{
					status_ = FAILURE;
					close();
				}
			}
			else if (status_ == CONNECTED)
			{
				status_ = FOLDER_CREATED;
				FtpSetCurrentDirectory( ftp_, dirName_ );
			}
			else if (status_ == FOLDER_CREATED)
			{
				status_ = SENDING;
				FtpPutFile( ftp_, localFileName_.c_str(), removeFileName_, FTP_TRANSFER_TYPE_ASCII, dwContext );
			}
			else if (status_ == SENDING)
			{
				status_ = result->dwError == ERROR_SUCCESS ? COMPLETED : FAILURE;
				DeleteFile( localFileName_.c_str() );
				close();
			}
		}
	}

	void close()
	{
		if (ftp_)
		{
			InternetCloseHandle( ftp_ );
		}

		if (inet_)
		{
			InternetCloseHandle( inet_ );
		}

		ftp_ = inet_ = NULL;
	}
public:
	Ftp( const wchar_t* server, const wchar_t* username, const wchar_t* password,
		const wchar_t* dirName, const wchar_t* fileName, const void* buffer, size_t size )
		: status_( NOT_CONNECTED ), inet_( NULL ), ftp_( NULL ),
		dirName_( dirName ), removeFileName_( fileName )
	{
		localFileName_ = getTempFilePathName();

		if (!localFileName_.empty())
		{
			HANDLE file = CreateFile( localFileName_.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL );

			if (file != INVALID_HANDLE_VALUE)
			{
				DWORD bytesWritten;

				MF_ASSERT( size <= UINT_MAX );
				WriteFile( file, buffer, ( DWORD ) size, &bytesWritten, NULL );

				CloseHandle( file );

				inet_ = InternetOpen( AGENT_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, INTERNET_FLAG_ASYNC );

				if( inet_ )
				{
					InternetSetStatusCallback( inet_, (INTERNET_STATUS_CALLBACK)statusCallback );

					InternetConnect( inet_, server, INTERNET_DEFAULT_FTP_PORT,
						username, password, INTERNET_SERVICE_FTP, 0, (DWORD_PTR)this );
				}
			}
		}

		if (inet_ == NULL)
		{
			status_ = FAILURE;
		}
	}

	~Ftp()
	{
		close();
	}

	bool completed() const
	{
		return status_ == COMPLETED;
	}

	bool failure() const
	{
		return status_ == FAILURE;
	}
};

BW_END_NAMESPACE

#endif//_FTP_HPP__
