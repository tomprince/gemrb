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
#character generation, proficiencies (GUICG9)
import GemRB
import GUICommon
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import LUProfsSelection

SkillWindow = 0
DoneButton = 0


def RedrawSkills():
	ProfsPointsLeft = GemRB.GetVar ("ProfsPointsLeft")
	if not ProfsPointsLeft:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global SkillWindow, DoneButton
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	SkillWindow = GemRB.LoadWindow(9)

	GUICommon.CloseOtherWindow(SkillWindow.Unload)

	MyChar = GemRB.GetVar ("Slot")
	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]
	LUProfsSelection.SetupProfsWindow (MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN, SkillWindow, RedrawSkills, [0,0,0], Levels,scroll=False,profTableOffset=0)

	BackButton = SkillWindow.GetControl(77)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)

	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	SkillWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def NextPress():
	MyChar = GemRB.GetVar ("Slot")
	LUProfsSelection.ProfsSave(MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN)
	CharGenCommon.next()
