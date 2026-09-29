/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Audio options dialog.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

class SoundControlsClass
{
	public:
		SoundControlsClass(void) {}
		void Dialog(void);

		static BOOL CALLBACK Sound_Option_Dialog_Func(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};