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

static int get_actor(PyObject* obj, Actor** ptr)
{
	int GlobalID = PyInt_AsLong(obj);
	if (GlobalID == -1 && PyErr_Occurred()) {
		return false;
	}
	
	Game *game = core->GetGame();
	if (!game) {
		return (bool)RuntimeError( "No game loaded!" );
	}

	Actor *actor = game->GetActorByGlobalID(GlobalID);
	if (!actor) {
		return (bool)RuntimeError("No such actor!");
	}
	*ptr = actor;
	return true;
}


PyDoc_STRVAR(GemRB_Actor_get_name__doc,
"Get actors name.\n\n"
"arg => -1 for name, 0 for shortname, 1 for longname");

static PyObject* GemRB_Actor_get_name(PyObject*, PyObject* args)
{
	Actor *actor;
	int Which;

	if (!PyArg_ParseTuple(args, "O&i", get_actor, &actor, &Which)) {
		return AttributeError(GemRB_Actor_get_name__doc);
	}

	return PyString_FromString(actor->GetName(Which));
}

PyDoc_STRVAR(GemRB_Actor_get_scriptname__doc,
"Get actors scriptname.");

static PyObject* GemRB_Actor_get_scriptname(PyObject*, PyObject* args)
{
	Actor *actor;

	if (!PyArg_ParseTuple(args, "O&", get_actor, &actor)) {
		return AttributeError(GemRB_Actor_get_scriptname__doc);
	}

	return PyString_FromString(actor->GetScriptName());
}

PyDoc_STRVAR(GemRB_Actor_get_stat__doc,
"Get actors stats.");

static PyObject* GemRB_Actor_get_stat(PyObject*, PyObject* args)
{
	Actor *actor;
	int Stat;

	if (!PyArg_ParseTuple(args, "O&i", get_actor, &actor, &Stat)) {
		return AttributeError(GemRB_Actor_get_stat__doc);
	}

	return PyInt_FromLong(actor->GetStat(Stat));
}

PyDoc_STRVAR(GemRB_Actor_get_map__doc,
"Get actors maps.");

static PyObject* GemRB_Actor_get_map(PyObject*, PyObject* args)
{
	Actor *actor;

	if (!PyArg_ParseTuple(args, "O&", get_actor, &actor)) {
		return AttributeError(GemRB_Actor_get_map__doc);
	}

	Map *map = actor->GetCurrentArea();
	if (!map) {
		Py_RETURN_NONE;
	}

	return gs->ConstructObject("Area", map->GetGlobalID());
}

PyDoc_STRVAR(GemRB_Actor_GetCurrentAction__doc,
"Get currently running action.");

static PyObject* GemRB_Actor_GetCurrentAction(PyObject*, PyObject* args)
{
	Actor *actor;

	if (!PyArg_ParseTuple(args, "O&", get_actor, &actor)) {
		return AttributeError(GemRB_Actor_get_map__doc);
	}

	return CObject<Action>(actor->GetCurrentAction());
}

PyDoc_STRVAR(GemRB_Actor_AddAction__doc,
"Enqueue an action");

static PyObject* GemRB_Actor_AddAction(PyObject*, PyObject* args)
{
	Actor *actor;
	PyObject* a;

	if (!PyArg_ParseTuple(args, "O&O", get_actor, &actor, &a)) {
		return AttributeError(GemRB_Actor_get_map__doc);
	}

	CObject<Action> action(a);
	if (action)
		actor->AddAction(action);

	Py_RETURN_NONE;
}
