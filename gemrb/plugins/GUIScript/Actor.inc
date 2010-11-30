PyDoc_STRVAR(GemRB_GetPCs__doc,
"Get a list of all PCs.");

static PyObject* GemRB_GetPCs(PyObject*, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	size_t size = game->GetPartySize(false);
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		PyList_SetItem(list, i, gs->ConstructObject("Actor", game->GetPC(i, false)->GetGlobalID()));
	}

	return list;
}

PyDoc_STRVAR(GemRB_GetNPCs__doc,
"Get a list of all load NPCs.");

static PyObject* GemRB_GetNPCs(PyObject*, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	size_t size = game->GetNPCCount();
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		PyList_SetItem(list, i, gs->ConstructObject("Actor", game->GetNPC(i)->GetGlobalID()));
	}

	return list;
}

PyDoc_STRVAR(GemRB_Actor_get_name__doc,
"Get actors name.\n\n"
"arg => -1 for name, 0 for shortname, 1 for longname");

static PyObject* GemRB_Actor_get_name(PyObject*, PyObject* args)
{
	int GlobalID, Which;

	if (!PyArg_ParseTuple(args, "ii", &GlobalID, &Which)) {
		return AttributeError(GemRB_Actor_get_name__doc);
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor *actor = game->GetActorByGlobalID(GlobalID);
	if (!actor) {
		return RuntimeError("No such actor!");
	}
	return PyString_FromString(actor->GetName(Which));
}

PyDoc_STRVAR(GemRB_Actor_get_scriptname__doc,
"Get actors scriptname.");

static PyObject* GemRB_Actor_get_scriptname(PyObject*, PyObject* args)
{
	int GlobalID;

	if (!PyArg_ParseTuple(args, "i", &GlobalID)) {
		return AttributeError(GemRB_Actor_get_scriptname__doc);
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor *actor = game->GetActorByGlobalID(GlobalID);
	if (!actor) {
		return RuntimeError("No such actor!");
	}
	return PyString_FromString(actor->GetScriptName());
}

PyDoc_STRVAR(GemRB_Actor_get_stat__doc,
"Get actors stats.");

static PyObject* GemRB_Actor_get_stat(PyObject*, PyObject* args)
{
	int GlobalID, Stat;

	if (!PyArg_ParseTuple(args, "ii", &GlobalID, &Stat)) {
		return AttributeError(GemRB_Actor_get_stat__doc);
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor *actor = game->GetActorByGlobalID(GlobalID);
	if (!actor) {
		return RuntimeError("No such actor!");
	}
	return PyInt_FromLong(actor->GetStat(Stat));
}

