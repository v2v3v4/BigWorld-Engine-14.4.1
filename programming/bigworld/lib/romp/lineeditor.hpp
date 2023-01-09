#ifndef LINEEDITOR_HPP
#define LINEEDITOR_HPP

#if ENABLE_CONSOLES

#include "input/input.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
class XConsole;

/**
 *	This class handles key events to edit a line of text
 */
class LineEditor
{
public:
	typedef BW::vector<BW::wstring> StringVector;

	enum ProcessState
	{
		NOT_HANDLED,
		PROCESSED,
		RESULT_SET,
	};


	LineEditor(XConsole * console);
	~LineEditor();

	ProcessState processKeyEvent( KeyEvent event, BW::wstring & resultString );

	const BW::wstring& editString() const			{ return editString_; }
	void editString( const BW::wstring & s );
	void insertInEditString( const BW::wstring & s );

	int cursorPosition() const						{ return cx_; }
	void cursorPosition( int pos );

	bool advancedEditing() const					{ return advancedEditing_; }
	void advancedEditing( bool enable )				{ advancedEditing_ = enable; }

	void tick( float dTime );
	void deactivate();

	const StringVector history() const;
	void setHistory(const StringVector & history);
	void insertHistory( const BW::wstring& history );

	void lineLength( int length ) { lineLength_ = length; }	
	int lineLength() { return lineLength_; }

private:
	LineEditor(const LineEditor&);
	LineEditor& operator=(const LineEditor&);

	bool processAdvanceEditKeys( KeyEvent event );

	int insertChar( int pos, wchar_t c );
	void deleteChar( int pos );
	int curWordStart( int pos ) const;
	int curWordEnd( int pos ) const;

	BW::wstring cutText( int start, int end );
	int pasteText( int pos, const BW::wstring & text );

	void showHistory();

	BW::wstring editString_;		// the currently edited string
	BW::wstring clipBoard_;		// clip-board string

	int		cx_;
	bool	inOverwriteMode_;
	bool	advancedEditing_;
	char	lastChar_;

	BW::vector<BW::wstring>	 history_;
	int	historyShown_;

	int lineLength_;

	XConsole * console_;
};

#ifdef CODE_INLINE
#include "lineeditor.ipp"
#endif

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
#endif // LINEEDITOR_HPP
