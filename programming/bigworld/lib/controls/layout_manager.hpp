#ifndef LAYOUTMANAGER_HPP
#define LAYOUTMANAGER_HPP

/**
 *  These are a set of classes designed to make automatically resizing
 *  CDialogs, CFormViews etc.
 *
 *  To use these classes do the following:
 *
 *  1.  #include "layout_manager.hpp" in the header of your dialog.
 *
 *  2.  In your dialog's class have OnSize and OnGetMinMaxInfo functions and
 *      include a SizerPtr member variable:
 *
 *          class MyDialog : public CDialog
 *          {
 *              ...
 *              afx_msg void OnSize(UINT type, int cx, int cy);
 *              afx_msg void OnGetMinMaxInfo(MINMAXINFO *mmi);
 *              ...
 *          private:
 *              controls::SizerPtr  rootSizer_;
 *          };
 *
 *  3.  In you dialog's implementation add ON_WM_WIZE and ON_WM_GETMINMAXINFO
 *      to the message map:
 *
 *          BEGIN_MESSAGE_MAP(LayoutManagerDlg, CDialog)
 *              ...
 *              ON_WM_SIZE()
 *              ON_WM_GETMINMAXINFO()
 *          END_MESSAGE_MAP()
 *
 *  4.  Implement OnSize as follows:
 *
 *          void MyDialog::OnSize(UINT type, int cx, int cy)
 *          {
 *              if (rootSizer_ != NULL)
 *                  rootSizer_->onSize(cx, cy);
 *              CDialog::OnSize(type, cx, cy);
 *          }
 *
 *  5.  Similarly implement OnGetMinMaxInfo as follows:
 *
 *          void MyDialog::OnGetMinMaxInfo(MINMAXINFO *mmi)
 *          {
 *              CDialog::OnGetMinMaxInfo(mmi);
 *              if (rootSizer_ != NULL)
 *                  rootSizer_->onGetMinMaxInfo(mmi);
 *          }
 *
 *  6.  In the implementation of MyDialog::OnInitDialog initialize the
 *      root of the resizing tree:
 *
 *      void MyDialog::OnInitDialog()
 *      {
 *          RowSizer *row = new RowSizer(RowSizer::HORIZONTAL);
 *          row->addChild(new WndSizer(this, IDC_IMAGE));
 *          RowSizer *col = new RowSizer(RowSizer::VERTICAL);
 *          col->addChild
 *          (
 *              new WndSizer(this, IDOK, WndSizer::FIT_WIDTH, WndSizer::FIT_VCENTER),
 *              0,
 *              RowSizer::PIXELS
 *          );
 *          col->addChild
 *          (
 *              new WndSizer(this, IDCANCEL, WndSizer::FIT_WIDTH, WndSizer::FIT_VCENTER),
 *              0,
 *              RowSizer::PIXELS
 *          );
 *          col->addChild(new NullSizer(0, 0));
 *          row->addChild(col, 0, RowSizer::PIXELS);
 *          rootSizer_ = row;
 *      }
 *
 *      Here use RowSizer for horizontal and vertical columns/rows, WndSizer
 *      to hold a window (either by id or by a CWnd) and NullSizer to represent
 *      empty space.
 */

#include "sizer.hpp"
#include "row_sizer.hpp"
#include "null_sizer.hpp"
#include "wnd_sizer.hpp"
#include "grid_sizer.hpp"


#endif // LAYOUTMANAGER_HPP
