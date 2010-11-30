#-*-python-*-
#GemRB - Infinity Engine Emulator
#Copyright (C) 2009 The GemRB Project
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


import _GemRB

from MetaClasses import metaIDWrapper

class GTable:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Table_GetValue,
    'FindValue': _GemRB.Table_FindValue,
    'GetRowIndex': _GemRB.Table_GetRowIndex,
    'GetRowName': _GemRB.Table_GetRowName,
    'GetColumnIndex': _GemRB.Table_GetColumnIndex,
    'GetColumnName': _GemRB.Table_GetColumnName,
    'GetRowCount': _GemRB.Table_GetRowCount,
    'GetColumnCount': _GemRB.Table_GetColumnCount
  }
  def __del__(self):
    # don't unload tables if the _GemRB module is already unloaded at exit
    if self.ID != -1 and _GemRB:
      pass #_GemRB.Table_Unload(self.ID)
  def __nonzero__(self):
    return self.ID != -1

class GSymbol:
  __metaclass__ = metaIDWrapper
  methods = {
    'GetValue': _GemRB.Symbol_GetValue,
    'Unload': _GemRB.Symbol_Unload
  }

class GWindow:
  __metaclass__ = metaIDWrapper
  methods = {
    'SetSize': _GemRB.Window_SetSize,
    'SetFrame': _GemRB.Window_SetFrame,
    'SetPicture': _GemRB.Window_SetPicture,
    'SetPos': _GemRB.Window_SetPos,
    'HasControl': _GemRB.Window_HasControl,
    'DeleteControl': _GemRB.Window_DeleteControl,
    'Unload': _GemRB.Window_Unload,
    'SetupEquipmentIcons': _GemRB.Window_SetupEquipmentIcons,
    'SetupSpellIcons': _GemRB.Window_SetupSpellIcons,
    'SetupControls': _GemRB.Window_SetupControls,
    'SetVisible': _GemRB.Window_SetVisible,
    'ShowModal': _GemRB.Window_ShowModal,
    'Invalidate': _GemRB.Window_Invalidate
  }
  def GetControl(self, control):
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateWorldMapControl(self, control, *args):
    _GemRB.Window_CreateWorldMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateMapControl(self, control, *args):
    _GemRB.Window_CreateMapControl(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateLabel(self, control, *args):
    _GemRB.Window_CreateLabel(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateButton(self, control, *args):
    _GemRB.Window_CreateButton(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateScrollBar(self, control, *args):
    _GemRB.Window_CreateScrollBar(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
  def CreateTextEdit(self, control, *args):
    _GemRB.Window_CreateTextEdit(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID, control)
 

class GControl:
  __metaclass__ = metaIDWrapper
  methods = {
    'SetVarAssoc': _GemRB.Control_SetVarAssoc,
    'SetPos': _GemRB.Control_SetPos,
    'SetSize': _GemRB.Control_SetSize,
    'SetAnimationPalette': _GemRB.Control_SetAnimationPalette,
    'SetAnimation': _GemRB.Control_SetAnimation,
    'QueryText': _GemRB.Control_QueryText,
    'SetText': _GemRB.Control_SetText,
    'SetTooltip': _GemRB.Control_SetTooltip,
    'SetEvent': _GemRB.Control_SetEvent,
    'SetStatus': _GemRB.Control_SetStatus,
  }
  def AttachScrollBar(self, scrollbar):
    if self.ID[0] != scrollbar.ID[0]:
      raise RuntimeError, "Scrollbar must be in same Window as Control"
    return _GemRB.Control_AttachScrollBar(self.ID, scrollbar.ID)

class GLabel(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'SetTextColor': _GemRB.Label_SetTextColor,
    'SetUseRGB': _GemRB.Label_SetUseRGB
  }

class GTextArea(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'Rewind': _GemRB.TextArea_Rewind,
    'SetHistory': _GemRB.TextArea_SetHistory,
    'Append': _GemRB.TextArea_Append,
    'Clear': _GemRB.TextArea_Clear,
    'Scroll': _GemRB.TextArea_Scroll,
    'SetFlags': _GemRB.Control_TextArea_SetFlags,
    'GetCharSounds': _GemRB.TextArea_GetCharSounds,
    'GetCharacters': _GemRB.TextArea_GetCharacters,
    'GetPortraits': _GemRB.TextArea_GetPortraits
  }
  def MoveText(self, other):
    _GemRB.TextArea_MoveText(self.ID, other.ID)

class GTextEdit(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'SetBufferLength': _GemRB.TextEdit_SetBufferLength
  }
  def ConvertEdit(self, ScrollBarID):
    newID = _GemRB.TextEdit_ConvertEdit(self.ID, ScrollBarID)
    return GTextArea(self.ID)

class GScrollBar(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'SetDefaultScrollBar': _GemRB.ScrollBar_SetDefaultScrollBar,
    'SetSprites': _GemRB.ScrollBar_SetSprites
  }

class GButton(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'SetSprites': _GemRB.Button_SetSprites,
    'SetOverlay': _GemRB.Button_SetOverlay,
    'SetBorder': _GemRB.Button_SetBorder,
    'EnableBorder': _GemRB.Button_EnableBorder,
    'SetFont': _GemRB.Button_SetFont,
    'SetTextColor': _GemRB.Button_SetTextColor,
    'SetFlags': _GemRB.Button_SetFlags,
    'SetState': _GemRB.Button_SetState,
    'SetPictureClipping': _GemRB.Button_SetPictureClipping,
    'SetPicture': _GemRB.Button_SetPicture,
    'SetSprite2D': _GemRB.Button_SetSprite2D,
    'SetMOS': _GemRB.Button_SetMOS,
    'SetPLT': _GemRB.Button_SetPLT,
    'SetBAM': _GemRB.Button_SetBAM,
    'SetSpellIcon': _GemRB.Button_SetSpellIcon,
    'SetItemIcon': _GemRB.Button_SetItemIcon,
    'SetActionIcon': _GemRB.Button_SetActionIcon
  }
  def CreateLabelOnButton(self, control, *args):
    _GemRB.Button_CreateLabelOnButton(self.ID, control, *args)
    return _GemRB.Window_GetControl(self.ID[0], control)

class GWorldMap(GControl):
  __metaclass__ = metaIDWrapper
  methods = {
    'AdjustScrolling': _GemRB.WorldMap_AdjustScrolling,
    'GetDestinationArea': _GemRB.WorldMap_GetDestinationArea,
    'SetTextColor': _GemRB.WorldMap_SetTextColor
  }

class _Portraits:
  def __init__(self, ID):
    self.ID = ID
  def __getitem__(self, index):
    return _GemRB.SaveGame_GetPortrait(self.ID, index)

class GSaveGame(object):
  __metaclass__ = metaIDWrapper
  methods = {
    'GetDate': _GemRB.SaveGame_GetDate,
    'GetGameDate': _GemRB.SaveGame_GetGameDate,
    'GetName': _GemRB.SaveGame_GetName,
    'GetPortrait': _GemRB.SaveGame_GetPortrait,
    'GetPreview': _GemRB.SaveGame_GetPreview,
    'GetSaveID': _GemRB.SaveGame_GetSaveID,
  }
  properties = {
    'date': _GemRB.SaveGame_GetDate,
    'game_date': _GemRB.SaveGame_GetGameDate,
    'name': _GemRB.SaveGame_GetName,
    'portrait': _Portraits,
    'preview': _GemRB.SaveGame_GetPreview,
    'save_ID': _GemRB.SaveGame_GetSaveID,
  }

class GSprite2D:
  __metaclass__ = metaIDWrapper
  methods = {}

class _Stats:
  def __init__(self, ID):
    self.ID = ID
  def __getitem__(self, index):
    return _GemRB.Actor_get_stat(self.ID, index)

class GActor(object):
  __metaclass__ = metaIDWrapper
  methods = {}
  properties = {
    'name' : [ lambda ID: _GemRB.Actor_get_name(ID, -1) ],
    'shortname' : [ lambda ID: _GemRB.Actor_get_name(ID, 0) ],
    'longname' : [ lambda ID: _GemRB.Actor_get_name(ID, 1) ],
    'scriptname' : [ _GemRB.Actor_get_scriptname ],
    'GlobalID' : [ lambda ID: ID ],
    'stats' : _Stats,
  }
  def __hash__(self):
	  return self.ID

class GArea(object):
  __metaclass__ = metaIDWrapper
  methods = {}
  properties = {
    'name' : [ _GemRB.Area_get_name ],
    'actors' : [ _GemRB.Area_get_actors ],
  }
