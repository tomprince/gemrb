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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#
# GUICommon.py - common functions for GUIScripts of all game types

import GemRB
import GUIClasses
import CommonTables
from ie_restype import RES_CHU, RES_WMP, RES_ARE
from ie_spells import LS_MEMO
from GUIDefines import *
from ie_stats import *
from ie_slots import SLOT_ALL
from ie_feats import FEAT_STRONG_BACK

OtherWindowFn = None
NextWindowFn = None

CommonTables.Load ()

def CloseOtherWindow (NewWindowFn):
	global OtherWindowFn,NextWindowFn

	GemRB.LeaveContainer()
	if OtherWindowFn and OtherWindowFn != NewWindowFn:
		# allow detection of 'next window'
		NextWindowFn = NewWindowFn
		# switching from a window to something else, call old function
		OtherWindowFn ()
		OtherWindowFn = NewWindowFn
		return 0
	elif OtherWindowFn:
		# something is calling us with its own function, so
		# it is closing down, return true
		OtherWindowFn = None
		return 1
	else:
		# new window, no need to do setup
		OtherWindowFn = NewWindowFn
		NextWindowFn = None
		return 0

def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	height = GemRB.GetSystemVariable (SV_HEIGHT)

	if GemRB.GameType == "pst":
		default = "GUIWORLD"
	else:
		default = "GUIW"

	# use a custom gui if there is one
	gui = "CGUI" + str(width)[:2] + str(height)[:2]
	if GemRB.HasResource (gui, RES_CHU, 1):
		return gui

	gui = None
	if width == 640:
		gui = default
	elif width == 800:
		gui = "GUIW08"
	elif width == 1024:
		gui = "GUIW10"
	elif width == 1280:
		gui = "GUIW12"
	if gui:
		if GemRB.HasResource (gui, RES_CHU, 1):
			return gui

	# fallback to the smallest resolution
	return default

def LocationPressed ():
	AreaInfo = GemRB.GetAreaInfo()
	print( "%s [%d.%d]\n"%(AreaInfo["CurrentArea"], AreaInfo["PositionX"], AreaInfo["PositionY"]) )
	return

def RestPress ():
	# FIXME: check "rest until healed", it's an option in some games
	GemRB.RestParty(0, 0, 8)
	return

def SelectFormation ():
	GemRB.GameSetFormation (GemRB.GetVar ("Formation"))
	return

def OpenFloatMenuWindow ():
	if GameIsPST():
		import FloatMenuWindow
		FloatMenuWindow.OpenFloatMenuWindow()
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

def GetActorPaperDoll (actor):
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "LEVEL%d" %(level+1)
	doll = CommonTables.Pdolls.GetValue (row, which)
	if doll == "*":
		print "GetActorPaperDoll: Missing paper doll for animation", row, which
	return doll

def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)

def GearsClicked ():
	#GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC (),44,249990)
	GemRB.GamePause (2, 0)

def SetColorStat (Actor, Stat, Value):
	t = Value & 0xFF
	t |= t << 8
	t |= t << 16
	GemRB.SetPlayerStat (Actor, Stat, t)
	return

def CheckStat100 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,100, Diff)
	if mystat>=goal:
		return True
	return False

def CheckStat20 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,20, Diff)
	if mystat>=goal:
		return True
	return False

def GameIsPST ():
	return GemRB.GameType == "pst"

def GameIsIWD ():
	return GemRB.GameType == "iwd"

def GameIsHOW ():
	return GemRB.GameType == "how"

def GameIsIWD1 ():
	return GemRB.GameType == "iwd" or GemRB.GameType == "how"

def GameIsIWD2 ():
	return GemRB.GameType == "iwd2"

def GameIsBG1 ():
	return GemRB.GameType == "bg1"

def GameIsBG2 ():
	return GemRB.GameType == "bg2"

def GameIsBG2Demo ():
	return ('BG2Demo' in GemRB.__dict__) and (GemRB.BG2Demo == True)

def GameIsTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP) and GemRB.GetVar("oldgame") == 0

def HasTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP)

def HasHOW ():
	return GemRB.HasResource ("expmap", RES_WMP)

def HasTOTL ():
	return GemRB.HasResource ("ar9700", RES_ARE)

def GetIWDSpellButtonCount ():
	if HasHOW():
		return 24
	else:
		return 20

def SetGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	days = currentTime / 7200
	hours = (currentTime % 7200) / 300
	GemRB.SetToken ('GAMEDAY', str (days))
	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

# Adds class/kit abilities
def AddClassAbilities (pc, table, Level=1, LevelDiff=1, align=-1):
	TmpTable = GemRB.LoadTable (table)
	import Spellbook

	# gotta stay positive
	if Level-LevelDiff < 0:
		return

	# we're doing alignment additions
	if align == -1:
		iMin = 0
		iMax = TmpTable.GetRowCount ()
	else:
		# alignment is expected to be the row required
		iMin = align
		iMax = align+1

	# make sure we don't go out too far
	jMin = Level-LevelDiff
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(iMin, iMax):
		# apply each spell from each new class
		for j in range (jMin, jMax):
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
				# seems all SPINs act like GA_*
				if ab[:4] == "SPIN":
					ab = "GA_" + ab

				# apply spell (AP_) or gain spell (GA_)
				if ab[:2] == "AP":
					GemRB.ApplySpell (pc, ab[3:])
				elif ab[:2] == "GA":
					SpellIndex = Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, ab[3:])
					if SpellIndex == -1:
						GemRB.LearnSpell (pc, ab[3:], LS_MEMO)
					else:
						# make room for one more memorization
						max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, IE_SPELL_TYPE_INNATE, 0, 0)
						GemRB.SetMemorizableSpellsCount (pc, max_mem_cnt+1, IE_SPELL_TYPE_INNATE, 0)
						# memorize another spell instance
						GemRB.MemorizeSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
				else:
					print "ERROR, unknown class ability (type): ", ab

# remove all class abilities up to the given level
# for dual-classing mainly
def RemoveClassAbilities (pc, table, Level):
	TmpTable = GemRB.LoadTable (table)
	import Spellbook

	# gotta stay positive
	if Level < 0:
		return

	# make sure we don't go out too far
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(TmpTable.GetRowCount ()):
		for j in range (jMax):
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
				# get the index
				SpellIndex = Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, ab[3:])

				# seems all SPINs act like GA_*
				if ab[:4] == "SPIN":
					ab = "GA_" + ab

				# apply spell (AP_) or gain spell (GA_)?
				if ab[:2] == "AP":
					GemRB.RemoveEffects (pc, ab[3:])
				elif ab[:2] == "GA":
					if SpellIndex >= 0:
						# TODO: get the correct counts to avoid removing an innate ability
						# given by more than one thing?
						# RemoveSpell will unmemorize them all too
						GemRB.RemoveSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
				else:
					print "ERROR, unknown class ability (type): ", ab

def UpdateInventorySlot (pc, Button, Slot, Type, Equipped=False):
	Button.SetFont ("NUMBER")
	Button.SetBorder (0, 0,0,0,0, 128,128,255,64, 0,1)
	Button.SetBorder (1, 2,2,2,2, 32,32,255,0, 0,0)
	Button.SetBorder (2, 0,0,0,0, 255,128,128,64, 0,1)
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_TOP | IE_GUI_BUTTON_PICTURE, OP_OR)
	Button.SetText ("")

	if Slot == None:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		if Type == "inventory":
			Button.SetTooltip (12013) # Personal Item
		elif Type == "ground":
			Button.SetTooltip (12011) # Ground Item
		else:
			Button.SetTooltip ("")
		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)
		Button.EnableBorder (2, 0)
	else:
		item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
		magical = Slot["Flags"] & IE_INV_ITEM_MAGICAL

		# MaxStackAmount holds the *maximum* item count in the stack while Usages0 holds the actual
		if item["MaxStackAmount"] > 1:
			Button.SetText (str (Slot["Usages0"]))
		else:
			Button.SetText ("")

		# auto-identify mundane items; the actual indentification will happen on transfer
		if not identified and item["LoreToID"] == 0:
			identified = True

		if not identified or item["ItemNameIdentified"] == -1:
			Button.SetTooltip (item["ItemName"])
			Button.EnableBorder (0, 1)
			Button.EnableBorder (1, 0)
		else:
			Button.SetTooltip (item["ItemNameIdentified"])
			Button.EnableBorder (0, 0)
			if magical:
				Button.EnableBorder (1, 1)
			else:
				Button.EnableBorder (1, 0)

		if GemRB.CanUseItemType (SLOT_ALL, Slot['ItemResRef'], pc, Equipped):
			Button.EnableBorder (2, 0)
		else:
			Button.EnableBorder (2, 1)

		Button.SetItemIcon (Slot['ItemResRef'], 0)

	return

# PST uses a button, IWD2 two types, the rest are the same with two labels
def SetEncumbranceLabels (Window, ControlID, Control2ID, pc, invert_colors = False):
	"""Displays the encumbrance as a ratio of current to maximum."""

	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	# encumbrance
	max_encumb = CommonTables.StrMod.GetValue (sstr, 3) + CommonTables.StrModEx.GetValue (ext_str, 3)
	if GemRB.HasFeat (pc, FEAT_STRONG_BACK):
		max_encumb += max_encumb/2
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Control = Window.GetControl (ControlID)
	if GameIsPST():
		# FIXME: there should be a space before LB symbol (':')
		Control.SetText (str (encumbrance) + ":\n\n\n\n" + str (max_encumb) + ":")
	elif GameIsIWD2() and not Control2ID:
		Control.SetText (str (encumbrance) + "/" + str(max_encumb) + GemRB.GetString(39537))
	else:
		Control.SetText (str (encumbrance) + ":")
		if not Control2ID: # shouldn't happen
			print "Missing second control parameter to SetEncumbranceLabels!"
			return
		Control2 = Window.GetControl (Control2ID)
		Control2.SetText (str (max_encumb) + ":")

	ratio = (0.0 + encumbrance) / max_encumb
	if ratio > 1.0:
		if invert_colors:
			Control.SetTextColor (255, 0, 0, True)
		else:
			Control.SetTextColor (255, 0, 0)
	elif ratio > 0.8:
		if invert_colors:
			Control.SetTextColor (255, 255, 0, True)
		else:
			Control.SetTextColor (255, 255, 0)
	else:
		if invert_colors:
			Control.SetTextColor (255, 255, 255, True)
		else:
			Control.SetTextColor (255, 255, 255)

	if Control2ID:
		Control2.SetTextColor (255, 0, 0)
		
	return

def GetActorClassTitle (actor):
	"""Returns the string representation of the actors class."""

	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)

	if ClassTitle == 0:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = CommonTables.Classes.FindValue ( 5, Class )
		KitIndex = GetKitIndex (actor)
		Multi = CommonTables.Classes.GetValue (ClassIndex, 4)
		Dual = IsDualClassed (actor, 1)

		if Multi and Dual[0] == 0: # true multi class
			ClassTitle = CommonTables.Classes.GetValue (ClassIndex, 2)
			ClassTitle = GemRB.GetString (ClassTitle)
		else:
			if Dual[0]: # dual class
				# first (previous) kit or class of the dual class
				if Dual[0] == 1:
					ClassTitle = CommonTables.KitList.GetValue (Dual[1], 2)
				elif Dual[0] == 2:
					ClassTitle = CommonTables.Classes.GetValue (Dual[1], 2)
				ClassTitle = GemRB.GetString (ClassTitle) + " / "
				ClassTitle += GemRB.GetString (CommonTables.Classes.GetValue (Dual[2], 2))
			else: # ordinary class or kit
				if KitIndex:
					ClassTitle = CommonTables.KitList.GetValue (KitIndex, 2)
				else:
					ClassTitle = CommonTables.Classes.GetValue (ClassIndex, 2)
				if ClassTitle != "*":
					ClassTitle = GemRB.GetString (ClassTitle)
	else:
		ClassTitle = GemRB.GetString (ClassTitle)

	#GetActorClassTitle returns string now...
	#if ClassTitle == "*":
	#	return 0

	return ClassTitle


def GetKitIndex (actor):
	"""Return the index of the actors kit from KITLIST.2da.

	Returns 0 if the class is not kitted."""

	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)
	KitIndex = 0

	if Kit & 0xc000 == 0x4000:
		KitIndex = Kit & 0xfff

	# carefully looking for kit by the usability flag
	# since the barbarian kit id clashes with the no-kit value
	if KitIndex == 0 and Kit != 0x4000:
		KitIndex = CommonTables.KitList.FindValue (6, Kit)
		if KitIndex == -1:
			KitIndex = 0

	return KitIndex

def IsDualClassed(actor, verbose):
	"""Returns an array containing the dual class information.

	Return[0] is 0 if not dualclassed, 1 if the old class is a kit, 2 otherwise.
	Return[1] contains either the kit or class index of the old class.
	Return[2] contains the class index of the new class.
	If verbose is false, only Return[0] contains useable data."""

	if GameIsIWD2():
		return (0,-1,-1)

	DualedFrom = GemRB.GetPlayerStat (actor, IE_MC_FLAGS) & MC_WAS_ANY_CLASS

	if verbose:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = CommonTables.Classes.FindValue (5, Class)
		Multi = CommonTables.Classes.GetValue (ClassIndex, 4)
		DualInfo = []
		KitIndex = GetKitIndex (actor)

		if DualedFrom > 0: # first (previous) class of the dual class
			MCColumn = CommonTables.Classes.GetColumnIndex ("MC_WAS_ID")
			FirstClassIndex = CommonTables.Classes.FindValue (MCColumn, DualedFrom)
			if KitIndex:
				DualInfo.append (1)
				DualInfo.append (KitIndex)
			else:
				DualInfo.append (2)
				DualInfo.append (FirstClassIndex)

			# use the first class of the multiclass bunch that isn't the same as the first class
			Mask = 1
			for i in range (1,16):
				if Multi & Mask:
					ClassIndex = CommonTables.Classes.FindValue (5, i)
					if ClassIndex == FirstClassIndex:
						Mask = 1 << i
						continue
					DualInfo.append (ClassIndex)
					break
				Mask = 1 << i
			if len(DualInfo) != 3:
				print "WARNING: Invalid dualclass combination, treating as a single class!"
				print DualedFrom, Class, Multi, KitIndex, DualInfo
				return (0,-1,-1)

			return DualInfo
		else:
			return (0,-1,-1)
	else:
		if DualedFrom > 0:
			return (1,-1,-1)
		else:
			return (0,-1,-1)

def IsDualSwap (actor):
	"""Returns true if the dualed classes are reverse of expection.

	This can happen, because the engine gives dualclass characters the same ID as
	their multiclass counterpart (eg. FIGHTER_MAGE = 3). Logic would dictate that
	the new and old class levels would be stored in IE_LEVEL and IE_LEVEL2,
	respectively; however, if one duals from a fighter to a mage in the above
	example, the levels would actually be in reverse of expectation."""

	Dual = IsDualClassed (actor, 1)

	# not dual classed
	if Dual[0] == 0:
		return 0

	# split the full class name into its individual parts
	# i.e FIGHTER_MAGE becomes [FIGHTER, MAGE]
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = CommonTables.Classes.FindValue (5, Class)
	Class = CommonTables.Classes.GetRowName (Class)
	Class = Class.split("_")

	# get our old class name
	if Dual[0] == 2:
		BaseClass = CommonTables.Classes.GetRowName (Dual[1])
	else:
		BaseClass = GetKitIndex (actor)
		BaseClass = CommonTables.KitList.GetValue (BaseClass, 7)
		if BaseClass == "*":
			# mod boilerplate
			return 0
		BaseClass = CommonTables.Classes.FindValue (5, BaseClass)
		BaseClass = CommonTables.Classes.GetRowName (BaseClass)

	# if our old class is the first class, we need to swap
	if Class[0] == BaseClass:
		return 1

	return 0

def IsMultiClassed (actor, verbose):
	"""Returns a tuple containing the multiclass information.

	Return[0] contains the total number of classes.
	Return[1-3] contain the ID of their respective classes.
	If verbose is false, only Return[0] has useable data."""

	# change this if it will ever be needed
	if GameIsIWD2():
		return (0,-1,-1,-1)

	# get our base class
	ClassIndex = CommonTables.Classes.FindValue (5, GemRB.GetPlayerStat (actor, IE_CLASS))
	IsMulti = CommonTables.Classes.GetValue (ClassIndex, 4) # 0 if not multi'd
	IsDual = IsDualClassed (actor, 0)

	# dual-class char's look like multi-class chars
	if (IsMulti == 0) or (IsDual[0] > 0):
		return (0,-1,-1,-1)
	elif verbose == 0:
		return (IsMulti,-1,-1,-1)

	# get all our classes (leave space for our number of classes in the return array)
	Classes = [0]*3
	NumClasses = 0
	Mask = 1 # we're looking at multiples of 2
	ClassNames = CommonTables.Classes.GetRowName(ClassIndex).split("_")

	# loop through each class and test it as a mask
	ClassCount = CommonTables.Classes.GetRowCount()
	for i in range (1, ClassCount):
		if IsMulti&Mask: # it's part of this class
			#we need to place the classes in the array based on their order in the name,
			#NOT the order they are detected in
			CurrentName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, i))
			if CurrentName == "*":
				# we read too far, as the upper range limit is greater than the number of "single" classes
				break
			for j in range(len(ClassNames)):
				if ClassNames[j] == CurrentName:
					Classes[j] = i # mask is (i-1)^2 where i is class id
			NumClasses = NumClasses+1
		Mask = 1 << i # shift to the next multiple of 2 for testing

	# in case we couldn't figure out to which classes the multi belonged
	if NumClasses < 2:
		print "ERROR: couldn't figure out the individual classes of multiclass", ClassNames
		return (0,-1,-1,-1)

	# return the tuple
	return (NumClasses, Classes[0], Classes[1], Classes[2])

def CanDualClass(actor):
	# human
	if GemRB.GetPlayerStat (actor, IE_RACE) != 1:
		return 1

	# already dualclassed
	Dual = IsDualClassed (actor,0)
	if Dual[0] > 0:
		return 1

	DualClassTable = GemRB.LoadTable ("dualclas")
	CurrentStatTable = GemRB.LoadTable ("abdcscrq")
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	KitIndex = GetKitIndex (actor)
	if KitIndex == 0:
		ClassTitle = ClassName
	else:
		ClassTitle = CommonTables.KitList.GetValue (KitIndex, 0)
	Row = DualClassTable.GetRowIndex (ClassTitle)

	# create a lookup table for the DualClassTable columns
	classes = []
	for col in range(DualClassTable.GetColumnCount()):
		classes.append(DualClassTable.GetColumnName(col))

	matches = []
	Sum = 0
	for col in range (0, DualClassTable.GetColumnCount ()):
		value = DualClassTable.GetValue (Row, col)
		Sum += value
		if value == 1:
			matches.append (classes[col])

	# cannot dc if all the columns of the DualClassTable are 0
	if Sum == 0:
		print "CannotDualClass: all the columns of the DualClassTable are 0"
		return 1

	# if the only choice for dc is already the same as the actors base class
	if Sum == 1 and ClassName in matches and KitIndex == 0:
		print "CannotDualClass: the only choice for dc is already the same as the actors base class"
		return 1

	AlignmentTable = GemRB.LoadTable ("alignmnt")
	AlignsTable = GemRB.LoadTable ("aligns")
	Alignment = GemRB.GetPlayerStat (actor, IE_ALIGNMENT)
	AlignmentColName = AlignsTable.FindValue (3, Alignment)
	AlignmentColName = AlignsTable.GetValue (AlignmentColName, 4)
	Sum = 0
	for classy in matches:
		Sum += AlignmentTable.GetValue (classy, AlignmentColName)

	# cannot dc if all the available classes forbid the chars alignment
	if Sum == 0:
		print "CannotDualClass: all the available classes forbid the chars alignment"
		return 1

	# check current class' stat limitations
	ClassStatIndex = CurrentStatTable.GetRowIndex (ClassTitle)
	for stat in range (6):
		minimum = CurrentStatTable.GetValue (ClassStatIndex, stat)
		name = CurrentStatTable.GetColumnName (stat)
		if GemRB.GetPlayerStat (actor, eval ("IE_" + name[4:])) < minimum:
			print "CannotDualClass: current class' stat limitations are too big"
			return 1

	# check new class' stat limitations - make sure there are any good class choices
	TargetStatTable = GemRB.LoadTable ("abdcdsrq")
	for match in matches:
		ClassStatIndex = TargetStatTable.GetRowIndex (match)
		for stat in range (6):
			minimum = TargetStatTable.GetValue (ClassStatIndex, stat)
			name = TargetStatTable.GetColumnName (stat)
			if GemRB.GetPlayerStat (actor, eval ("IE_" + name[4:])) < minimum:
				matches.remove (match)
				break
	if len(matches) == 0:
		print "CannotDualClass: no good new class choices"
		return 1

	# must be at least level 2
	if GemRB.GetPlayerStat (actor, IE_LEVEL) == 1:
		print "CannotDualClass: level 1"
		return 1
	return 0

def IsWarrior (actor):
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	IsWarrior = CommonTables.ClassSkills.GetValue (ClassName, "NO_PROF")

	# warriors get only a -2 penalty for wielding weapons they are not proficient with
	IsWarrior = (IsWarrior == -2)

	Dual = IsDualClassed (actor, 0)
	if Dual[0] > 0:
		DualedFrom = GemRB.GetPlayerStat (actor, IE_MC_FLAGS) & MC_WAS_ANY_CLASS
		MCColumn = CommonTables.Classes.GetColumnIndex ("MC_WAS_ID")
		FirstClassIndex = CommonTables.Classes.FindValue (MCColumn, DualedFrom)
		FirstClassName = CommonTables.Classes.GetRowName (FirstClassIndex)
		OldIsWarrior = CommonTables.ClassSkills.GetValue (FirstClassName, "NO_PROF")
		# there are no warrior to warrior dualclasses, so if the previous class was one, the current one certainly isn't
		if OldIsWarrior == -2:
			return 0
		# but there are also non-warrior to non-warrior dualclasses, so just use the new class check

	return IsWarrior

def SetupDamageInfo (pc, Button):
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	state = GemRB.GetPlayerStat (pc, IE_STATE_ID)

	if hp_max < 1:
		ratio = 0.0
	else:
		ratio = (hp+0.0) / hp_max

	if hp < 1 or (state & STATE_DEAD):
		Button.SetOverlay (0, 64,64,64,200, 64,64,64,200)
	else:
		Button.SetOverlay (ratio, 140,0,0,205, 128,0,0,200)
	ratio_str = "\n%d/%d" %(hp, hp_max)
	Button.SetTooltip (GemRB.GetPlayerName (pc, 1) + ratio_str)

	return ratio_str

def SetCurrentDateTokens (stat):
	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime ()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15)
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	# it is true, they changed the token
	if GameIsBG2():
		GemRB.SetToken ('GAMEDAY', str (days))
	else:
		GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

	return (days, hours)

# return ceil(n/d)
# 
def ceildiv (n, d):
	if d == 0:
		raise ZeroDivisionError("ceildiv by zero")
	elif d < 0:
		return (n+d+1)/d
	else:
		return (n+d-1)/d

# a placeholder for unimplemented and hardcoded key actions
def ResolveKey():
	return

GameWindow = GUIClasses.GWindow(0)
GameControl = GUIClasses.GControl(0,0)
