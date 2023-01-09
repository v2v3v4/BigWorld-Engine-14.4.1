#include "large_task_list_box.hpp"

#include "task_info.hpp"
#include "task_store.hpp"
#include "message_loop.hpp"
#include "details_dialog.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	class ClipboardHelper
	{
	public:
		ClipboardHelper(ATL::CWindow handle) : 
			opened_(FALSE)
		{
			opened_ = ::OpenClipboard(handle);
		}

		~ClipboardHelper()
		{
			if (opened_ != FALSE)
			{
				::CloseClipboard();
			}
		}

		void setTextData(const BW::wstring & text)
		{
			if (opened_ != FALSE)
			{
				HGLOBAL textData = copyTextIntoGlobalBuffer(text);
				if (textData)
				{
					::EmptyClipboard();
					::SetClipboardData(CF_UNICODETEXT, textData);
				}
			}
		}

	private:
		HGLOBAL copyTextIntoGlobalBuffer(const BW::wstring & text)
		{
			typedef BW::wstring::value_type char_type;

			HGLOBAL globalHandle = ::GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(char_type));
			if (globalHandle != nullptr)
			{
				auto buffer = static_cast<char_type *>(::GlobalLock(globalHandle));
				if (buffer != nullptr)
				{
					::CopyMemory(buffer, text.data(), text.length() * sizeof(char_type));
					buffer[text.length() - 1] = 0;

					::GlobalUnlock(globalHandle);
				}
				else
				{
					globalHandle = ::GlobalFree(globalHandle);
				}
			}

			return globalHandle;
		}

		BOOL opened_;
	};

	bool nameSortPredicate(const TaskInfoPtr left, const TaskInfoPtr right)
	{
		return left->source() < right->source();
	}

	bool resultSortPredicate(const TaskInfoPtr left, const TaskInfoPtr right)
	{
		// Want the higher values at the top of the list
		return left->getResult() > right->getResult();
	}

	bool typeSortPredicate(const TaskInfoPtr left, const TaskInfoPtr right)
	{
		return left->converterId() < right->converterId();
	}

	typedef bool (*SortPredicate)(const TaskInfoPtr, const TaskInfoPtr);

	SortPredicate getSortPredicate(LargeTaskListBox::SortMode mode)
	{
		if (mode == LargeTaskListBox::SORT_FILENAME)
		{
			return &nameSortPredicate;
		}
		else if (mode == LargeTaskListBox::SORT_FILETYPE)
		{
			return &typeSortPredicate;
		}
		else if (mode == LargeTaskListBox::SORT_RESULT)
		{
			return &resultSortPredicate;
		}
		else
		{
			return nullptr;
		}
	}
}

LargeTaskListBox::LargeTaskListBox(TaskStore & store, MainMessageLoop & messageLoop) : 
	TaskListBoxBase(store, messageLoop),
	sortMode_(SORT_NONE)
{
	showFilter_[TaskInfo::RESULT_SUCCESS] = true;
	showFilter_[TaskInfo::RESULT_WARNING] = true;
	showFilter_[TaskInfo::RESULT_ERROR] = true;
	contextMenu_.LoadMenuW(MAKEINTRESOURCE(IDR_COMPLETED_TASK_CONTEXT_MENU));
}

void LargeTaskListBox::updateItemHeight()
{
	// Fixes up the item height because we've already missed the WM_MEASUREITEM message
	// as we're being used by SubClassWindow.
	WTL::CClientDC dc(m_hWnd);
	
	TEXTMETRIC metrics;
	if (dc.GetTextMetricsW(&metrics) != FALSE)
	{
		SetItemHeight(0, metrics.tmAscent);
	}
}

void LargeTaskListBox::addTask(TaskInfoPtr task)
{
	allTasks_.push_back(task);
	if (isTaskVisible(task))
	{
		if (sortMode_ == SORT_NONE)
		{
			currentTasks_.push_back(task);
		}
		else
		{
			auto predicate = getSortPredicate(sortMode_);
			MF_ASSERT(predicate != nullptr);
			auto iter = std::lower_bound(std::begin(currentTasks_), std::end(currentTasks_), task, predicate);
			currentTasks_.insert(iter, task);
		}
		updateCurrentCount();
	}
}

void LargeTaskListBox::removeTask(TaskInfoPtr task)
{
	auto iter = std::find(std::begin(allTasks_), std::end(allTasks_), task);
	if (iter != std::end(allTasks_))
	{
		allTasks_.erase(iter);

		auto iter = std::find(std::begin(currentTasks_), std::end(currentTasks_), task);
		if (iter != std::end(currentTasks_))
		{
			currentTasks_.erase(iter);
			updateCurrentCount();
		}
	}
}

void LargeTaskListBox::pauseRedraw()
{
	SetRedraw(FALSE);
}

void LargeTaskListBox::resumeRedraw()
{
	SetRedraw(TRUE);
	RedrawWindow();
}

void LargeTaskListBox::updateCurrentCount(bool resetView)
{
	pauseRedraw();
	const int topIndex = resetView ? 0 : GetTopIndex();
	const int selectedIndex = resetView ? -1 : GetCurSel();
	SetCount(static_cast<int>(currentTasks_.size()));
	SetCurSel(selectedIndex);
	SetTopIndex(topIndex);
	resumeRedraw();
}

void LargeTaskListBox::showNormalTasks(bool show)
{
	showFilter_[TaskInfo::RESULT_SUCCESS] = show;
	regenerateCurrentList();
}

void LargeTaskListBox::showWarningTasks(bool show)
{
	showFilter_[TaskInfo::RESULT_WARNING] = show;
	regenerateCurrentList();
}

void LargeTaskListBox::showErrorTasks(bool show)
{
	showFilter_[TaskInfo::RESULT_ERROR] = show;
	regenerateCurrentList();
}

void LargeTaskListBox::regenerateCurrentList()
{
	TaskList currentList;
	filterIntoTaskList(currentList);

	if (sortMode_ != SORT_NONE)
	{
		sortTaskList(currentList);
	}

	using std::swap;
	swap(currentTasks_, currentList);

	updateCurrentCount();
}

void LargeTaskListBox::setSortMode(SortMode mode)
{
	if (sortMode_ != mode)
	{
		sortMode_ = mode;

		if (sortMode_ == SORT_NONE)
		{
			TaskList currentList;
			filterIntoTaskList(currentList);

			using std::swap;
			swap(currentTasks_, currentList);
		}
		else
		{
			sortTaskList(currentTasks_);
		}

		updateCurrentCount(true);
	}
}

bool LargeTaskListBox::isTaskVisible(TaskInfoPtr taskInfo) const
{
	MF_ASSERT(taskInfo != nullptr);
	return showFilter_[static_cast<int>(taskInfo->getResult())];
}

void LargeTaskListBox::filterIntoTaskList(TaskList& list)
{
	list.clear();
	if (showFilter_[TaskInfo::RESULT_SUCCESS])
	{
		// Odds are we'll be showing all the items or close to it
		list.reserve(allTasks_.size());
	}

	auto predicate = [this](TaskInfoPtr taskInfo) { return isTaskVisible(taskInfo); };
	std::copy_if(std::begin(allTasks_), std::end(allTasks_), std::back_inserter(list), predicate);
}

void LargeTaskListBox::sortTaskList(TaskList& list)
{
	auto predicate = getSortPredicate(sortMode_);
	MF_ASSERT(predicate != nullptr);
	std::sort(std::begin(list), std::end(list), predicate);
}

void LargeTaskListBox::copyTaskDetails(TaskInfoPtr taskInfo)
{
	MF_ASSERT(taskInfo != nullptr);

	BW::wstring detailsText = taskInfo->getTextDump();

	ClipboardHelper clipboard(GetParent());
	clipboard.setTextData(detailsText);
}

void LargeTaskListBox::onDoubleClick(UINT flags, ::CPoint clientPoint)
{
	BOOL outside = TRUE;
	uint index = ItemFromPoint(clientPoint, outside);
	if (outside == TRUE)
	{
		return;
	}

	auto taskInfo = currentTasks_[index];
	if (taskInfo)
	{
		createDetailsDialog(GetParent(), taskInfo);
	}
}

void LargeTaskListBox::onContextMenu(CWindow handle, ::CPoint screenPoint)
{
	if (contextMenu_.IsNull())
	{
		return;
	}

	::CPoint clientPoint = screenPoint;
	ScreenToClient(&clientPoint);

	BOOL outside = TRUE;
	uint index = ItemFromPoint(clientPoint, outside);
	if (outside == TRUE)
	{
		return;
	}

	auto taskInfo = currentTasks_[index];
	MF_ASSERT(taskInfo != nullptr);

	WTL::CMenuHandle popupMenu = contextMenu_.GetSubMenu(0);
	UINT id = popupMenu.TrackPopupMenu(TPM_NONOTIFY | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetParent());
	switch (id)
	{
	case ID_TASK_DETAILS:
		createDetailsDialog(GetParent(), taskInfo);
		break;
	case ID_TASK_COPY_DETAILS:
		copyTaskDetails(taskInfo);
		break;
	case ID_TASK_OPEN:
		openTaskApplication(taskInfo);
		break;
	case ID_TASK_OPEN_FOLDER:
		openTaskFolder(taskInfo);
		break;
	};
}

void LargeTaskListBox::onMeasureItem(int id, LPMEASUREITEMSTRUCT measureStruct)
{
	SetMsgHandled(FALSE);
}

void LargeTaskListBox::onDrawItem(int id, LPDRAWITEMSTRUCT drawStruct)
{
	MF_ASSERT(drawStruct->CtlType == ODT_LISTBOX);

	if (drawStruct->itemAction != ODA_DRAWENTIRE && drawStruct->itemAction != ODA_SELECT)
	{
		return;
	}

	const int index = drawStruct->itemID;

	auto taskInfo = currentTasks_[index];
	MF_ASSERT(taskInfo != nullptr);

	WTL::CDCHandle dc(drawStruct->hDC);

	COLORREF oldTextColour = dc.GetTextColor();
	COLORREF oldBackColour = dc.GetBkColor();
	if (taskInfo->hasErrors() || taskInfo->hasFailed())
	{
		dc.SetBkColor(RGB(231, 195, 195));
	}
	else if (taskInfo->hasWarnings())
	{
		dc.SetBkColor(RGB(245, 231, 158));
	}
	if ((drawStruct->itemState & ODS_SELECTED) != 0)
	{
		dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
		dc.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
	}

	UINT oldTextAlign = dc.SetTextAlign(TA_TOP | TA_LEFT);

	const ::CRect& textRect = drawStruct->rcItem;

	auto text = taskInfo->getFormattedName();
	dc.ExtTextOutW(textRect.left, textRect.top, ETO_OPAQUE, &textRect, text.c_str(), static_cast<UINT>(text.length()));

	dc.SetTextAlign(oldTextAlign);
	dc.SetBkColor(oldBackColour);
	dc.SetTextColor(oldTextColour);
}

LRESULT LargeTaskListBox::onEraseBackground(WTL::CDCHandle dc)
{
	SetMsgHandled(FALSE);
	return 0;
}

void LargeTaskListBox::onPaint(WTL::CDCHandle dc)
{
	SetMsgHandled(FALSE);
}

BW_END_NAMESPACE
