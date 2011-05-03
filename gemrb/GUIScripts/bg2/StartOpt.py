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
import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	OptionsWindow = GemRB.LoadWindow(13)
	OptionsWindow.SetFrame ()
	SoundButton = OptionsWindow.GetControl(8)
	GameButton = OptionsWindow.GetControl(9)
	GraphicButton = OptionsWindow.GetControl(7)
	BackButton = OptionsWindow.GetControl(11)
	SoundButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GraphicButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	BackButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SoundButton.SetText(17164)
	GameButton.SetText(17165)
	GraphicButton.SetText(17162)
	BackButton.SetText(10308)
	SoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SoundPress)
	GameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, GamePress)
	GraphicButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, GraphicPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	OptionsWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def SoundPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT7")
	return
	
def GamePress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return
	
def GraphicPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT6")
	return
	
def BackPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Start")
	return
