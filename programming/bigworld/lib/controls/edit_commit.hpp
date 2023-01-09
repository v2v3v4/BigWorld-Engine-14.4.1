#ifndef EDIT_COMMIT_HPP
#define EDIT_COMMIT_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
// EditCommit

namespace controls
{
	class CONTROLS_DLL EditCommit : public CEdit
	{
	
	public:
		EditCommit();
		virtual ~EditCommit();
	
		void autoSelect( bool sel) { autoSelect_ = sel; }
		void commitOnFocusLoss( bool state ) { commitOnFocusLoss_ = state; }
			
		bool needsUpdate()	{ return dirty_; }
		void updateDone()	{ dirty_ = false; }
		bool doUpdate()		{ bool temp = dirty_; dirty_ = false; return temp; }

	private:
		void doCommit() { dirty_ = true; }
		bool autoSelect_;
		bool dirty_;
		bool commitOnFocusLoss_;
	protected:
		DECLARE_MESSAGE_MAP()
	public:
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg BOOL OnSetfocus();
		afx_msg BOOL OnKillfocus();
	};
}

BW_END_NAMESPACE

#endif // EDIT_COMMIT_HPP
