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
#character generation, multi-class (GUICG10)
import GemRB
import CommonTables
from ie_stats import *
from GUIDefines import *
import GUICG2

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, MyChar
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	ClassWindow = GemRB.LoadWindow(10)

	MyChar = GemRB.GetVar ("Slot")
	ClassCount = CommonTables.Classes.GetRowCount()+1
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	RaceName = CommonTables.Races.GetRowName(CommonTables.Races.FindValue (3, Race) )

	j=0
	for i in range(1,ClassCount):
		if CommonTables.Classes.GetValue(i-1,4)==0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		j = j + 1
	j=0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, RaceName)
		if CommonTables.Classes.GetValue(i-1,4)==0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)

		t = CommonTables.Classes.GetValue(i-1, 0)
		Button.SetText(t )
		j=j+1
		if Allowed ==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClassPress)
		Button.SetVarAssoc("Class", i) #multiclass, actually

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	ClassWindow.SetVisible(WINDOW_VISIBLE)
	return

def ClassPress():
	GUICG2.SetClass()
	Class = GemRB.GetVar("Class")-1
	TextAreaControl.SetText(CommonTables.Classes.GetValue(Class,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.SetVar("Class",0)  # scrapping it
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	GUICG2.SetClass()
	if ClassWindow:
		ClassWindow.Unload()

	# find the class from the class table
	ClassIndex = GemRB.GetVar ("Class") - 1
	Class = CommonTables.Classes.GetValue (ClassIndex, 5)
	#protect against barbarians
	ClassName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, Class) )
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	GemRB.SetNextScript("CharGen4") #alignment
	return
