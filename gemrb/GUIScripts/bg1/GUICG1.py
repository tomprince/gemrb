# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation, gender (GUICG1)
import GemRB
from GUIDefines import *
from ie_stats import *

import CharGenCommon
import GUICommon

GenderWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global GenderWindow, TextAreaControl, DoneButton

	if GUICommon.CloseOtherWindow (OnLoad):
		if(GenderWindow):
			GenderWindow.Unload()
			GenderWindow = None
		return
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	
	GenderWindow = GemRB.LoadWindow(1)

	BackButton = GenderWindow.GetControl(6)
	BackButton.SetText(15416)
	DoneButton = GenderWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GenderWindow.GetControl(5)
	TextAreaControl.SetText(17236)

	MaleButton = GenderWindow.GetControl(2)
	MaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GenderWindow.GetControl(3)
	FemaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	
	GemRB.SetVar("Gender",0)
	MaleButton.SetVarAssoc("Gender",1)
	FemaleButton.SetVarAssoc("Gender",2)
	MaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClickedMale)
	FemaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClickedFemale)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	GenderWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ClickedMale():
	TextAreaControl.SetText(13083)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	TextAreaControl.SetText(13084)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	MyChar = GemRB.GetVar ("Slot")
	#GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)
	CharGenCommon.next()
	return
