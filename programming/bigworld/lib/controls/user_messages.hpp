#ifndef CONROLS_USER_MESSAGES_HPP
#define CONROLS_USER_MESSAGES_HPP

#include "controls/defs.hpp"

// User defined messages begin at this address for controls.lib:
#define     WM_CONTROL_BASE             (0x1000)

// ColorTimeline user messages:
#define     WM_CT_UPDATE_BEGIN          (WM_CONTROL_BASE +  0)
#define     WM_CT_UPDATE_MIDDLE         (WM_CONTROL_BASE +  1)
#define     WM_CT_UPDATE_DONE           (WM_CONTROL_BASE +  2)
#define     WM_CT_UPDATE_CANCEL         (WM_CONTROL_BASE +  3)
#define     WM_CT_ADDING_COLOR          (WM_CONTROL_BASE +  4)
#define     WM_CT_ADDED_COLOR           (WM_CONTROL_BASE +  5)
#define     WM_CT_NEW_SELECTION         (WM_CONTROL_BASE +  6)
#define     WM_CT_SEL_TIME              (WM_CONTROL_BASE +  7)

// ColorPicker user messages:
#define     WM_CP_LBUTTONDOWN           (WM_CONTROL_BASE + 20)
#define     WM_CP_LBUTTONMOVE           (WM_CONTROL_BASE + 21)
#define     WM_CP_LBUTTONUP             (WM_CONTROL_BASE + 22)      

// Auto-tooltip messages:
#define     WM_SHOW_TOOLTIP             (WM_CONTROL_BASE + 40)
#define     WM_HIDE_TOOLTIP             (WM_CONTROL_BASE + 41)

// Range slider messages:
#define     WM_RANGESLIDER_CHANGED      (WM_CONTROL_BASE + 60)
#define     WM_RANGESLIDER_TRACK        (WM_CONTROL_BASE + 61)

// Edit numeric messages:
#define     WM_EDITNUMERIC_CHANGE       (WM_CONTROL_BASE + 80)
#define     WM_EDITNUMERIC_FINAL_CHANGE (WM_CONTROL_BASE + 81)

// SearchField messages
#define     WM_SEARCHFIELD_CHANGE       (WM_CONTROL_BASE + 82)
#define     WM_SEARCHFIELD_FILTERS      (WM_CONTROL_BASE + 83)
#define     WM_SEARCHFIELD_CLEAR        (WM_CONTROL_BASE + 84)
#endif // CONROLS_USER_MESSAGES_HPP
