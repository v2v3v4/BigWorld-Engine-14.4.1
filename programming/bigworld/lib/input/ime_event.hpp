#ifndef IME_EVENT__HPP
#define IME_EVENT__HPP


BW_BEGIN_NAMESPACE

/**
 *	An instance of this class represents an IME related event.
 */
class IMEEvent
{
public:	
	IMEEvent() :	stateChanged_( false ),
					candidatesVisibilityChanged_( false ), 
					candidatesChanged_( false ), 
					selectedCandidateChanged_( false ),
					compositionChanged_( false ),
					compositionCursorPositionChanged_( false ),
					readingVisibilityChanged_( false ),
					readingChanged_( false )
	{
	}

	IMEEvent( const IMEEvent& other )	
		:	stateChanged_( other.stateChanged_ ),
			candidatesVisibilityChanged_( other.candidatesVisibilityChanged_ ), 
			candidatesChanged_( other.candidatesChanged_ ), 
			selectedCandidateChanged_( other.selectedCandidateChanged_ ),
			compositionChanged_( other.compositionChanged_ ),
			compositionCursorPositionChanged_( other.compositionCursorPositionChanged_ ),
			readingVisibilityChanged_( other.readingVisibilityChanged_ ),
			readingChanged_( other.readingChanged_ )
	{
	}


	/**
	 *	This method determines whether or not the current IME state has changed.
	 */
	bool stateChanged() const	{ return stateChanged_; }
	void stateChanged(bool b)	{ stateChanged_ = b; }

	/**
	 *	This method determines whether the reading string visibility has changed.
	 */
	bool readingVisibilityChanged() const	{ return readingVisibilityChanged_; }
	void readingVisibilityChanged(bool b) { readingVisibilityChanged_ = b; }

	/**
	 *	This method determines whether the reading string has changed.
	 */
	bool readingChanged() const { return readingChanged_; }
	void readingChanged(bool b) { readingChanged_ = b; }

	/**
	 *	This method determines whether the candidate visibility has changed.
	 */
	bool candidatesVisibilityChanged() const	{ return candidatesVisibilityChanged_; }
	void candidatesVisibilityChanged(bool b) { candidatesVisibilityChanged_ = b; }

	/**
	 *	This method determines whether or not the list of candidates has changed.
	 */
	bool candidatesChanged() const	{ return candidatesChanged_; }
	void candidatesChanged(bool b)	{ candidatesChanged_ = b; }

	/**
	 *	This method determines whether or not the candidate selection index has changed.
	 */
	bool selectedCandidateChanged() const	{ return selectedCandidateChanged_; }
	void selectedCandidateChanged(bool b)	{ selectedCandidateChanged_ = b; }

	/**
	 *	This method determines whether or not the composition string has changed.
	 */
	bool compositionChanged() const	{ return compositionChanged_; }
	void compositionChanged(bool b) { compositionChanged_ = b; }

	/**
	 *	This method determines whether or not the composition cursor position has changed.
	 */
	bool compositionCursorPositionChanged() const	{ return compositionCursorPositionChanged_; }
	void compositionCursorPositionChanged(bool b) { compositionCursorPositionChanged_ = b; }

	/**
	 *	This method returns true if this event represents any change.
	 */
	bool dirty() const
	{ 
		return	stateChanged() ||
				candidatesVisibilityChanged() ||
				candidatesChanged() ||
				readingChanged() ||
				selectedCandidateChanged() ||
				compositionChanged() ||
				compositionCursorPositionChanged(); 
	}


private:
	bool stateChanged_;
	bool candidatesVisibilityChanged_;
	bool candidatesChanged_;
	bool selectedCandidateChanged_;
	bool compositionChanged_;
	bool compositionCursorPositionChanged_;
	bool readingVisibilityChanged_;
	bool readingChanged_;
};

BW_END_NAMESPACE

#endif // IME_EVENT__HPP

