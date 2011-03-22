/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "AIScript.h"

#include "PythonHelpers.h"

#include "Scriptable/Actor.h"

AIScript::AIScript()
{
	Script = NULL;
	Globals = PyDict_New();
	PyDict_Merge( Globals, gs->pMainDic, false );
}

AIScript::~AIScript()
{
	if (Py_IsInitialized()) {
		Py_DECREF(Script);
		Py_DECREF(Globals);
	}
}

bool AIScript::Open(DataStream* str)
{
	char* string = new char[str->Size()+1];
	str->Read(string, str->Size());
	string[str->Size()] = '\0';
	PyObject* code = Py_CompileString(string, str->originalfile, Py_file_input);
	if (!code)
		return false;
	Script = PyFunction_New(code, Globals);
	if (!Script)
		return false;
	return true;
}
bool AIScript::Update(bool* /*continuing*/, bool* /*done*/)
{
	PyDict_SetItemString(Globals, "self", gs->ConstructObject("Actor", MySelf->GetGlobalID()));

	PyObject* ret = PyObject_CallObject(Script, NULL);	
	if (PyErr_Occurred()) {
		PyErr_Print();
		return false;
	}
	bool result = PyObject_IsTrue(ret);
	Py_DECREF(ret);
	return result;
}
void AIScript::EvaluateAllBlocks()
{
	Update();
}
