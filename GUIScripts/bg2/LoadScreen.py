# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# LoadScreen.py - display Loading screen

###################################################

import GemRB
import GUICommon
from GUIDefines import *

LoadScreen = None
hide = None

def SetLoadScreen ():
	return

def StartLoadScreen ():
	global LoadScreen

	GemRB.LoadWindowPack ("guils", 640, 480)
	LoadScreen = GemRB.LoadWindow (0)
	LoadScreen.SetFrame ()
	Middle = LoadScreen.GetControl (3)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic == "":
		#the addition of 1 is not an error, bg2 loadpic resrefs are GTRSK002-GTRSK006
		LoadPic = "GTRSK00"+str(GemRB.Roll(1,5,1) )
	Middle.SetMOS (LoadPic)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	if GUICommon.HasTOB():
		Table = GemRB.LoadTable ("loadh25")
	else:
		Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1,tmp,0)
	HintStr = Table.GetValue (tmp, 0)
	TextArea = LoadScreen.GetControl (2)
	TextArea.SetText (HintStr)
	Bar = LoadScreen.GetControl (0)
	Bar.SetVarAssoc ("Progress", Progress)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	LoadScreen.SetVisible (WINDOW_VISIBLE)

def EndLoadScreen ():
	LoadScreen.SetVisible (WINDOW_VISIBLE)
	LoadScreen.Unload()
        return

