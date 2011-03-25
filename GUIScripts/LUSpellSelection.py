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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_restype import RES_BAM
import GUICommon

# storage variables
pc = 0
chargen = 0
KitMask = 0

# basic spell selection
SpellsWindow = 0		# << spell selection window
SpellKnownTable = 0		# << known spells per level (table)
DoneButton = 0			# << done/next button
SpellsTextArea = 0		# << spell description area
SpellsSelectPointsLeft = [0]*9	# << spell selections left per level
Spells = [0]*9			# << spells learnable per level
SpellTopIndex = 0		# << scroll bar index
SpellBook = []			# << array containing all the spell indexes to learn
SpellLevel = 0			# << current level of spells
SpellStart = 0			# << starting id of the spell list
SpellPointsLeftLabel = 0	# << label indicating the number of points left
EnhanceGUI = 0			# << scrollbars and extra spell slot for sorcs on LU

# chargen only
SpellsPickButton = 0		# << button to select random spells
SpellsCancelButton = 0		# << cancel chargen


def OpenSpellsWindow (actor, table, level, diff, kit=0, gen=0, recommend=True):
	"""Opens the spells selection window.

	table should refer to the name of the classes MXSPLxxx.2da.
	level contains the current level of the actor.
	diff contains the difference from the old level.
	kit should always be GetKitIndex except when dualclassing.
	gen is true if this is for character generation.
	recommend is used in bg2 for spell recommendation / autopick."""

	global SpellsWindow, DoneButton, SpellsSelectPointsLeft, Spells, chargen, SpellPointsLeftLabel
	global SpellsTextArea, SpellsKnownTable, SpellTopIndex, SpellBook, SpellLevel, pc, SpellStart
	global KitMask, EnhanceGUI

	#enhance GUI?
	if (GemRB.GetVar("GUIEnhancements")):
		EnhanceGUI = 1

	# save our pc
	pc = actor
	chargen = gen

	# this ensures compatibility with chargen, sorc, and dual-classing
	if kit == 0:
		KitMask = 0x4000
	else: # need to implement this if converted to CharGen
		KitMask = kit 

	# make sure there is an entry at the given level (bard)
	SpellsKnownTable = GemRB.LoadTable (table)
	if not SpellsKnownTable.GetValue (str(level), str(1), 1):
		if chargen:
			if GUICommon.GameIsBG2():
				# HACK
				GemRB.SetNextScript("GUICG6")
			elif GUICommon.GameIsBG1():
				# HACK
				from CharGenCommon import next
				next()
		return

	# load our window
	if chargen:
		GemRB.LoadWindowPack("GUICG", 640, 480)
		SpellsWindow = GemRB.LoadWindow (7)
		if not recommend:
			GUICommon.CloseOtherWindow (SpellsWindow.Unload)
		DoneButton = SpellsWindow.GetControl (0)
		SpellsTextArea = SpellsWindow.GetControl (27)
		SpellPointsLeftLabel = SpellsWindow.GetControl (0x1000001b)
		if (EnhanceGUI):
			SpellsWindow.CreateScrollBar (1000, 325,42, 16,252)
			HideUnhideScrollBar(1)
		SpellStart = 2

		# cancel button only applicable for chargen
		SpellsCancelButton = SpellsWindow.GetControl(29)
		SpellsCancelButton.SetState(IE_GUI_BUTTON_ENABLED)
		SpellsCancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SpellsCancelPress)
		SpellsCancelButton.SetText(13727)
		SpellsCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

		if (recommend):
			# recommended spell picks
			SpellsPickButton = SpellsWindow.GetControl(30)
			SpellsPickButton.SetState(IE_GUI_BUTTON_ENABLED)
			SpellsPickButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SpellsPickPress)
			SpellsPickButton.SetText(34210)
	else:
		SpellsWindow = GemRB.LoadWindow (8)
		DoneButton = SpellsWindow.GetControl (28)
		SpellsTextArea = SpellsWindow.GetControl(26)
		SpellPointsLeftLabel = SpellsWindow.GetControl (0x10000018)
		if(EnhanceGUI):
			SpellsWindow.CreateScrollBar (1000, 290,142, 16,252)
			HideUnhideScrollBar(1)
			#25th spell button for sorcerers
			SpellsWindow.CreateButton (24, 231, 345, 42, 42)
		SpellStart = 0

	# setup our variables
	GemRB.SetVar ("SpellTopIndex", 0)

	# the done button also doubles as a next button
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SpellsDonePress)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)
		
	AlreadyShown = 0
	for i in range (9):
		# make sure we always have a value to minus (bards)
		SecondPoints = SpellsKnownTable.GetValue (str(level-diff), str(i+1), 1)
		if not SecondPoints:
			SecondPoints = 0

		# make sure we get more spells of each class before continuing
		SpellsSelectPointsLeft[i] = SpellsKnownTable.GetValue (str(level), str(i+1), 1) - SecondPoints
		if SpellsSelectPointsLeft[i] <= 0:
			continue
		elif chargen and KitMask != 0x4000:
			# specialists get an extra spell per level
			SpellsSelectPointsLeft[i] += 1

		# chargen character seem to get more spells per level (this is kinda dirty hack)
		# except sorcerers
		if chargen and GemRB.GetPlayerStat (pc, IE_CLASS) != 19:
			SpellsSelectPointsLeft[i] += 1

		# get all the spells of the given level
		Spells[i] = GUICommon.GetMageSpells (KitMask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)

		# dump all the spells we already know
		NumDeleted = 0
		for j in range (len (Spells[i])):
			CurrentIndex = j - NumDeleted # this ensure we don't go out of range
			if GUICommon.HasSpell (pc, IE_SPELL_TYPE_WIZARD, i, Spells[i][CurrentIndex][0]) >= 0:
				del Spells[i][CurrentIndex]
				NumDeleted += 1

		# display these spells if it's the first non-zero level
		if AlreadyShown == 0:
			# save the level and spellbook data
			SpellLevel = i
			SpellBook = [0]*len(Spells[i])

			if(EnhanceGUI):
				# setup the scrollbar
				ScrollBar = SpellsWindow.GetControl (1000)
				#FIXME: use other resources instead, this one is bg2-only
				if GemRB.HasResource ("GUISCRCW", RES_BAM):
					ScrollBar.SetSprites ("GUISCRCW", 0, 0,1,2,3,5,4)
				ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, ShowSpells)
				ScrollBar.SetDefaultScrollBar ()

				# only scroll if we have more than 24 spells or 25 if extra 25th spell slot is available in sorcs LevelUp
				if len (Spells[i]) > ( 24 + ExtraSpellButtons() ):
					HideUnhideScrollBar(0)
					if chargen:
						ScrollBar.SetVarAssoc ("SpellTopIndex", GUICommon.ceildiv ( ( len (Spells[i])-24 ) , 6 ) + 1 )
					else: #there are five rows of 5 spells in level up of sorcs
						ScrollBar.SetVarAssoc ("SpellTopIndex", GUICommon.ceildiv ( ( len (Spells[i])-25 ) , 5 ) + 1 )
				else:
					ScrollBar.SetVarAssoc ("SpellTopIndex", 0)
					HideUnhideScrollBar(1)

			# show our spells
			ShowSpells ()
			AlreadyShown = 1

	# show the selection window
	if chargen:
		if recommend:
			SpellsWindow.SetVisible (WINDOW_VISIBLE)
		else:
			SpellsWindow.ShowModal (MODAL_SHADOW_NONE)
	else:
		SpellsWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

def SpellsDonePress ():
	"""Move to the next assignable level.

	If there is not another assignable level, then save all the new spells and
	close the window."""

	global SpellBook, SpellLevel, SpellsWindow

	# save all the spells
	for i in range (len (Spells[SpellLevel])):
		if SpellBook[i]: # we need to learn this spell
			GemRB.LearnSpell (pc, Spells[SpellLevel][i][0])

	# check to see if we need to update again
	for i in range (SpellLevel+1, 9):
		if SpellsSelectPointsLeft[i] > 0:
			# reset the variables
			GemRB.SetVar ("SpellTopIndex", 0)
			SpellLevel = i
			SpellBook = [0]*len(Spells[i])

			if (EnhanceGUI):
				# setup the scrollbar
				ScrollBar = SpellsWindow.GetControl (1000)
				if len (Spells[i]) > ( 24 + ExtraSpellButtons() ):
					HideUnhideScrollBar(0)
					if chargen:
						ScrollBar.SetVarAssoc ("SpellTopIndex", GUICommon.ceildiv ( ( len (Spells[i])-24 ) , 6 ) + 1 )
					else:
						ScrollBar.SetVarAssoc ("SpellTopIndex", GUICommon.ceildiv ( ( len (Spells[i])-25 ) , 5 ) + 1 )
				else:
					ScrollBar.SetVarAssoc ("SpellTopIndex", 0)
					HideUnhideScrollBar(1)

			# show the spells and set the done button to off
			ShowSpells ()
			DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
			return

	# close our window and update our records
	if SpellsWindow and (not chargen or GUICommon.GameIsBG2()):
		SpellsWindow.Unload ()
		SpellsWindow = None

	# move to the next script if this is chargen
	if chargen:
		if GUICommon.GameIsBG2():
			# HACK
			GemRB.SetNextScript("GUICG6")
		elif GUICommon.GameIsBG1():
			# HACK
			from CharGenCommon import next
			next()

	return

def ShowSpells ():
	"""Shows the viewable 24 spells."""

	j = RowIndex()

	# we have a grid of 24 spells
	for i in range (24 + ExtraSpellButtons()):
		# ensure we can learn this many spells
		SpellButton = SpellsWindow.GetControl (i+SpellStart)
		if i + j >= len (Spells[SpellLevel]):
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			SpellButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue
		else:
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

		# fill in the button with the spell data
		Spell = GemRB.GetSpell (Spells[SpellLevel][i+j][0], 1)
		SpellButton.SetTooltip(Spell['SpellName'])
		SpellButton.SetVarAssoc("ButtonPressed", i)
		SpellButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SpellsSelectPress)
		if GUICommon.GameIsBG2():
			SpellButton.SetSprites("GUIBTBUT",0, 0,1,2,3)
		else:
			SpellButton.SetSprites("GUIBTBUT",0, 0,1,24,25)

		SpellButton.SetSpellIcon(Spells[SpellLevel][i+j][0], 1)
		SpellButton.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)

		# don't allow the selection of an un-learnable spell
		if Spells[SpellLevel][i+j][1] == 0:
			SpellButton.SetState(IE_GUI_BUTTON_LOCKED)
			# shade red
			SpellButton.SetBorder (0, 0,0, 0,0, 200,0,0,100, 1,1)
		elif Spells[SpellLevel][i+j][1] == 1: # learnable
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			# unset any borders on this button or an un-learnable from last level
			# will still shade red even though it is clickable
			SpellButton.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)
		else: # specialist (shouldn't get here)
			# use the green border state for matching specialist spells
			SpellButton.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)
			SpellButton.SetState (IE_GUI_BUTTON_THIRD)

	# show which spells are selected
	ShowSelectedSpells ()

	GemRB.SetToken("number", str(SpellsSelectPointsLeft[SpellLevel]))
	SpellsTextArea.SetText(17250)

	return

def SpellsSelectPress ():
	"""Toggles the selection of the given spell."""

	global SpellsSelectPointsLeft, Spells, SpellBook

	# get our variables
	j = RowIndex()
	i = GemRB.GetVar ("ButtonPressed") + j

	# get the spell that's been pushed
	Spell = GemRB.GetSpell (Spells[SpellLevel][i][0], 1)
	SpellsTextArea.SetText (Spell["SpellDesc"])

	# make sure we can learn the spell
	if Spells[SpellLevel][i][1]:
		if SpellBook[i]: # already picked -- unselecting
			SpellsSelectPointsLeft[SpellLevel] = SpellsSelectPointsLeft[SpellLevel] + 1
			SpellBook[i] = 0
			DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
		else: # selecting
			# we don't have any picks left
			if SpellsSelectPointsLeft[SpellLevel] == 0:
				MarkButton (i, 0)
				return

			# if we have a specialist, we must make sure they pick at least
			# one spell of their school per level
			if SpellsSelectPointsLeft[SpellLevel] == 1 and not HasSpecialistSpell () \
			and Spells[SpellLevel][i][1] != 2:
				SpellsTextArea.SetText (33381)
				MarkButton (i, 0)
				return

			# select the spell and change the done state if need be
			SpellsSelectPointsLeft[SpellLevel] = SpellsSelectPointsLeft[SpellLevel] - 1
			SpellBook[i] = 1
			if SpellsSelectPointsLeft[SpellLevel] == 0:
				DoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	# show selected spells
	ShowSelectedSpells ()

	return

def MarkButton (i, select):
	"""Shows enabled, disabled, or highlighted button.

	If selected is true, the button is highlighted.
	Be sure i is sent with +SpellTopIndex!"""

	j = RowIndex()

	if select:
		type = IE_GUI_BUTTON_SELECTED
	else:
		if Spells[SpellLevel][i][1] == 1:
			type = IE_GUI_BUTTON_ENABLED
		elif Spells[SpellLevel][i][1] == 2:
			# specialist spell
			type = IE_GUI_BUTTON_THIRD
		else: # can't learn
			type = IE_GUI_BUTTON_LOCKED

	# we have to use the index on the actual grid
	SpellButton = SpellsWindow.GetControl(i+SpellStart-j)
	SpellButton.SetState(type)
	return

def ShowSelectedSpells ():
	"""Highlights all selected spells."""

	k = RowIndex()

	# mark all of the spells picked thus far
	for j in range (24 + ExtraSpellButtons()):
		if j + k >= len (SpellBook): # make sure we don't call unavailable indexes
			break
		if SpellBook[j+k]: # selected
			MarkButton (j+k, 1)
		else: # not selected
			MarkButton (j+k, 0)
	
	# show how many spell picks are left
	SpellPointsLeftLabel.SetText (str (SpellsSelectPointsLeft[SpellLevel]))
	return

def SpellsCancelPress ():
	"""Removes all known spells and close the window.

	This is only callable within character generation."""

	# remove all learned spells
	GUICommon.RemoveKnownSpells (pc, IE_SPELL_TYPE_WIZARD, 1, 9, 1)

	if GUICommon.GameIsBG2():
		# unload teh window and go back
		if SpellsWindow:
			SpellsWindow.Unload()
		GemRB.SetNextScript("CharGen6") #haterace
	elif GUICommon.GameIsBG1():
		import CharGenCommon
		CharGenCommon.BackPress()
	else:
		print "Uh-oh in SpellsCancelPress in", GemRB.GameType
	return

def SpellsPickPress ():
	"""Auto-picks spells for the current level based on splautop.2da.

	Only used in character generation.
	Perhaps implement for sorcerers, if possible."""

	global SpellBook, SpellsSelectPointsLeft

	# load up our table
	AutoTable = GemRB.LoadTable ("splautop")

	for i in range (AutoTable.GetRowCount ()):
		if SpellsSelectPointsLeft[SpellLevel] == 0:
			break

		CurrentSpell = AutoTable.GetValue (i, SpellLevel, 0)
		for j in range (len (Spells[SpellLevel])):
			# we can learn the spell, and it's not in our book
			if Spells[SpellLevel][j][0].upper() == CurrentSpell.upper() \
			and Spells[SpellLevel][j][1] and not SpellBook[j]:
				# make sure we learn at least 1 specialist spell
				if SpellsSelectPointsLeft[SpellLevel] == 1 and not HasSpecialistSpell () \
				and Spells[SpellLevel][j][1] != 2:
					SpellsTextArea.SetText (33381)
					break

				# save our spell and decrement the points left
				SpellBook[j] = 1
				SpellsSelectPointsLeft[SpellLevel] -= 1
				break

	# show the spells and update the counts
	ShowSelectedSpells ()

	# if we don't have any points left, we can enable the done button
	if not SpellsSelectPointsLeft[SpellLevel]:
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
				
	return

def HasSpecialistSpell ():
	"""Determines if specialist requirements have been met.

	Always returns true if the mage is not a specialist.
	Returns true only if the specialists knows at least one spell from thier
	school."""

	# always return true for non-kitted classed
	if KitMask == 0x4000:
		return 1

	# return true if we've memorized a school spell of this level
	for i in range (len (Spells[SpellLevel])):
		if Spells[SpellLevel][i][1] == 2 and SpellBook[i]:
			return 1

	# return true if there are no specialist spells of this level
	SpecialistSpellCount = 0
	for i in range (len (Spells[SpellLevel])):
		if Spells[SpellLevel][i][1] == 2:
			SpecialistSpellCount = 1
			break
	if SpecialistSpellCount == 0:
		return 1

	# no luck
	return 0

def RowIndex ():
	"""Determines which factor to use in scrolling of spells

	It depends on if it is character generation where you have
	4 rows of 6 spells (24), or it is sorcs level up window where there
	is 4 rows of 5 spells and 5th row of 4 spell, but you may also use 25th slot there
	and it is 5 rows of 5 with 25 spells seen at once. """

	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	if chargen:
		return ( SpellTopIndex + 1 ) * 6 - 6
	elif EnhanceGUI:
		return ( SpellTopIndex + 1 ) * 5 - 5
	else:
		return SpellTopIndex

def ExtraSpellButtons ():
	"""Determines if extra spell slots are available. """

	if EnhanceGUI and (not chargen):
		return 1
	else:
		return 0

def HideUnhideScrollBar (hide = 0):
	ScrollBar = SpellsWindow.GetControl (1000)

	if hide == 1:
		scrollx = -1
		scrolly = -1
	else:
		if chargen:
			scrollx = 325
			scrolly = 42
		else:
			scrollx = 290
			scrolly = 142

	ScrollBar.SetPos (scrollx, scrolly)
