/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Game controls dialog.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

class GameControlsClass 
{
	public:
		GameControlsClass() {};
		void Dialog();

	private:
		void Set();

	private:
		HWND _Dialog;
};
