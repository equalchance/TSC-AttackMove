/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Owner draw user interface.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/
#pragma once

#include "always.h"
#include "wstring.h"


class Surface;


/*
**  Owner-draw control messages, handled by CtrlProc and the per-control
**  control procedures. Each is WM_USER + offset.
*/

// Initialise a freshly subclassed control's WINDATA. Per-control: listbox sets item height/font,
// combobox sets item height and clears item colours, static captures its caption, progress bar sets
// max=100, edit strips WS_TABSTOP and deflects focus.
// Used by: all controls.  Input: none.  Output: none.
#define OD_SUBCLASSED           (WM_USER + 151)

// Set a per-element colour. listbox = per-row text colour (wParam = row); combobox = per-item text
// colour (wParam = item 0..50); static = text colour (wParam ignored, lParam == -1 resets to default).
// Used by: listbox, combobox, static.  Input: wParam = element index, lParam = COLORREF.  Output: none.
#define OD_SETCOLOR             (WM_USER + 152)

// Add a column to the multi-column listbox.
// Used by: listbox.  Input: wParam = column width, lParam = column x-position (also the id).  Output: the x-position (existing one if a column is already there).
#define OD_ADDCOLUMN            (WM_USER + 166)

// Remove the column whose x-position matches lParam.
// Used by: listbox.  Input: lParam = column x-position/id.  Output: the id, or -1 if not found.
#define OD_REMOVECOLUMN         (WM_USER + 167)

// Set the contents of one listbox cell.
// Used by: listbox.  Input: wParam = MAKEWPARAM(columnId, row), lParam = CellData*.  Output: column id, or -1 on failure.
#define OD_SETCELL              (WM_USER + 168)

// Set the trackbar step (the increment applied per click).
// Used by: trackbar.  Input: lParam = INT step.  Output: none.
#define OD_SETTRACKSTEP         (WM_USER + 171)

// Show or hide the numeric value drawn beside the trackbar.
// Used by: trackbar.  Input: lParam = BOOL.  Output: none.
#define OD_TRACKNUMBERS         (WM_USER + 172)


namespace OwnerDraw
{
/**
 *  The contents of one multi-column listbox cell, passed via OD_SETCELL.
 *  The game copies the struct's contents out; it never takes ownership.
 */
struct CellData
{
    enum DataType
    {
        INVALID,
        TEXT,       // draws `string` in `color`
        SURFACE,    // draws `surf`, vertically centered
        PING,       // draws a ping gradient bar from `pingtime`
        PRIMARY,    // draws the row's listbox string in `color`
    };

    DataType type = INVALID;
    Wstring string;
    Wstring string2;
    int color = -1;     // COLORREF, -1 = default text color
    Surface* surf = nullptr;
    int pingtime = 0;
};

void Init_Masks();
void Cache_Images();
void Initialize();
bool SubclassDlg(HWND window, LPARAM lparam);
void DrawItem(DRAWITEMSTRUCT* drawitem);
void DrawDialogBack(HWND window);
int Capture_Mouse();
int Release_Mouse();

/**
 *  Creates a modeless game dialog, like WSCreateDialog, but does not
 *  rescale it with Resize_Dialog: the dialog keeps the size Windows gives
 *  its template. Used by the fixed-size dialogs (main menu, options...).
 *  Destroy dialogs created with BeginDialog with EndDialog.
 */
HWND BeginDialog(int id, BOOL (CALLBACK* proc)(HWND, UINT, WPARAM, LPARAM));
void EndDialog(HWND window);

extern int& BorderThickness;
extern int& ColorSteps;
extern int& ScrollBarAdj;
extern COLORREF& TextColor1;
extern COLORREF& TextColor2;
extern COLORREF& CommonColor1;
extern COLORREF& CommonColor2;
extern COLORREF& ListBoxColor;
extern COLORREF& TooltipBoxColor;
extern COLORREF& ColorUnused1;

extern HFONT& FontPtr1;
extern HFONT& FontPtr2;
extern char const*& FontName1;
extern char const*& FontName2;
extern int& FontSize1;
extern int& FontSize2;
}; // namespace OwnerDraw
