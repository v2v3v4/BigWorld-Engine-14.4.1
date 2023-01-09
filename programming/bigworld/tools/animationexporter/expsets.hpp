#ifndef EXPSETS_HPP
#define EXPSETS_HPP

#pragma warning ( disable : 4530 )

#include <iostream>

#include <windows.h>

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class ExportSettings
{
public:
	~ExportSettings();

	bool readSettings( const char* fileName );
	bool writeSettings( BW::string fileName );
	bool readSettings( DataSectionPtr pSection );
	bool writeSettings( DataSectionPtr pSection );
	bool displayDialog( HWND hWnd );

	const BW::string &referenceNodesFile( void ) const { return referenceNodesFile_; }

	enum NodeFilter
	{
		ALL = 0,
		SELECTED,
		VISIBLE
	};

	void setFrameRange( int first, int last ) { frameFirst_ = first; frameLast_ = last; }

	int firstFrame() const { return frameFirst_; };
	int lastFrame() const { return frameLast_; };

	void setStaticFrame( int i ) { staticFrame_ = i; }
	int staticFrame( void ) const { return staticFrame_; }
	
	float	unitScale( ) const;

	NodeFilter	nodeFilter( ) const { return nodeFilter_; };
	void		nodeFilter( NodeFilter nodeFilter ) { nodeFilter_ = nodeFilter; };

	bool		allowScale( ) const { return allowScale_; };
	void		allowScale( bool state ) { allowScale_ = state; };

	bool		exportNodeAnimation() const { return exportNodeAnimation_; }
	void		exportNodeAnimation( bool state ) { exportNodeAnimation_ = state; }

	bool		useLegacyOrientation() const { return useLegacyOrientation_; }
	void		useLegacyOrientation( bool state ) { useLegacyOrientation_ = state; }

	bool		exportCueTrack() const { return exportCueTrack_; }

	static ExportSettings& ExportSettings::instance();

private:
	ExportSettings();
	static INT_PTR CALLBACK dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK settingsDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool			allowScale_;
	bool			exportNodeAnimation_;
	bool			exportCueTrack_;	
	bool			useLegacyOrientation_;	

	NodeFilter		nodeFilter_;

	int	staticFrame_; //the static frame output
	int	frameFirst_;
	int	frameLast_;

	BW::string	referenceNodesFile_;
	BW::string getReferenceFile( HWND hWnd );
	void displayReferenceHierarchyFile( HWND hWnd ) const;

	ExportSettings(const ExportSettings&);
	ExportSettings& operator=(const ExportSettings&);

	friend std::ostream& operator<<(std::ostream&, const ExportSettings&);
};

BW_END_NAMESPACE

#endif
/*expsets.hpp*/
