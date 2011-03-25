# -*-python-*-
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISAVE.py - Save window

###################################################

import GemRB
import GUICommon
import LoadScreen
from GUIDefines import *

SaveWindow = 0
ConfirmWindow = 0
NameField = 0
SaveButton = 0
TextAreaControl = 0
Games = ()
ScrollBar = 0
str_chapter = (48007, 48006, 16205, 16206, 16207, 16208, 16209, 71020, 71021, 71022)

def OpenSaveWindow ():
	global SaveWindow, TextAreaControl, Games, ScrollBar

	if GUICommon.CloseOtherWindow (OpenSaveWindow):
		CloseSaveWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUISAVE", 640, 480)
	Window = SaveWindow = GemRB.LoadWindow (0)
	Window.SetFrame ()
	CancelButton=Window.GetControl (34)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveWindow)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetVar ("LoadIdx",0)

	for i in range(4):
		Button = Window.GetControl (26+i)
		Button.SetText (15588)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SavePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		Button = Window.GetControl (30+i)
		Button.SetText (13957)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetVarAssoc ("LoadIdx",i)

		#area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range(PARTY_SIZE):
			Button = Window.GetControl (40+i*PARTY_SIZE+j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar=Window.GetControl (25)
	ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	Games=GemRB.GetSaveGames ()
	TopIndex = max (0, len(Games) - 4 + 1) #one more for the 'new game'
	GemRB.SetVar ("TopIndex",TopIndex)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex+1)
	ScrollBar.SetDefaultScrollBar ()
	ScrollBarPress ()
	Window.SetVisible (WINDOW_VISIBLE)
	return

def ScrollBarPress():
	Window = SaveWindow

	#draw load game portraits
	Pos = GemRB.GetVar ("TopIndex")
	for i in range(4):
		ActPos = Pos + i

		Button1 = Window.GetControl (26+i)
		Button2 = Window.GetControl (30+i)
		if ActPos<=len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetName()
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == len(Games):
			Slotname = 15304
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Slotname = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		Label = Window.GetControl (0x10000008+i)
		Label.SetText (Slotname)

		if ActPos<len(Games):
			Slotname = Games[ActPos].GetGameDate()
		else:
			Slotname = ""
		Label = Window.GetControl (0x10000010+i)
		Label.SetText (Slotname)

		Button=Window.GetControl (1+i)
		if ActPos<len(Games):
			Button.SetSprite2D(Games[ActPos].GetPreview())
		else:
			Button.SetPicture("")
		for j in range(PARTY_SIZE):
			Button=Window.GetControl (40+i*PARTY_SIZE+j)
			if ActPos<len(Games):
				Button.SetSprite2D(Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture("")
	return

def AbortedSaveGame():
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def ConfirmedSaveGame():
	global ConfirmWindow

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	Label = ConfirmWindow.GetControl (3)
	Slotname = Label.QueryText ()
	LoadScreen.StartLoadScreen()
	if Pos < len(Games):
		GemRB.SaveGame(Games[Pos], Slotname)
	else:
		GemRB.SaveGame(None, Slotname)
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	OpenSaveWindow() # close window
	return

def SavePress():
	global ConfirmWindow, NameField, SaveButton

	Pos = GemRB.GetVar ("TopIndex")+GemRB.GetVar ("LoadIdx")
	ConfirmWindow = GemRB.LoadWindow (1)

	#slot name
	if Pos<len(Games):
		Slotname = Games[Pos].GetName();
		save_strref = 15306
	else:
		Slotname = ""
		save_strref = 15588
	NameField = ConfirmWindow.GetControl (3)
	NameField.SetText (Slotname)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, EditChange)

	#game hours (should be generated from game)
	if Pos<len(Games):
		Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
		Slotname = GemRB.GetString(str_chapter[Chapter-1]) + " " + Games[Pos].GetGameDate()
	else:
		Slotname = ""
	Label = ConfirmWindow.GetControl (0x10000004)
	Label.SetText (Slotname)

	#areapreview
	Button=ConfirmWindow.GetControl (0)
	if Pos<len(Games):
		Button.SetSprite2D(Games[Pos].GetPreview())
	else:
		Button.SetPicture("")

	#portraits
	for j in range(PARTY_SIZE):
		Button=ConfirmWindow.GetControl (40+j)
		if Pos<len(Games):
			Button.SetSprite2D(Games[Pos].GetPortrait(j))
		else:
			Button.SetPicture("")

	#save
	SaveButton=ConfirmWindow.GetControl (7)
	SaveButton.SetText (save_strref)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConfirmedSaveGame)
	SaveButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	#SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	#cancel
	CancelButton=ConfirmWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbortedSaveGame)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def EditChange():
	Name = NameField.QueryText ()
	if len(Name)==0:
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		SaveButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DeleteGameConfirm():
	global Games

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex +GemRB.GetVar ("LoadIdx")
	GemRB.DeleteSaveGame(Games[Pos])
	del Games[Pos]
	if TopIndex>0:
		GemRB.SetVar ("TopIndex",TopIndex-1)
	ScrollBar.SetVarAssoc ("TopIndex", len(Games))
	ScrollBarPress()
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGameCancel():
	if ConfirmWindow:
		ConfirmWindow.Unload ()
	SaveWindow.SetVisible (WINDOW_VISIBLE)
	return

def DeleteGamePress():
	global ConfirmWindow

	SaveWindow.SetVisible (WINDOW_INVISIBLE)
	ConfirmWindow=GemRB.LoadWindow (2)
	Text=ConfirmWindow.GetControl (0)
	Text.SetText (15305)
	DeleteButton=ConfirmWindow.GetControl (1)
	DeleteButton.SetText (13957)
	DeleteButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameConfirm)
	CancelButton=ConfirmWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DeleteGameCancel)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ConfirmWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseSaveWindow ():
	if SaveWindow:
		SaveWindow.Unload ()
	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")
		return

	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) #enabling the game control screen
	GemRB.UnhideGUI () #enabling the other windows
	return
