PyDoc_STRVAR(GemRB_GetAreas__doc,
"Get a list of all loaded Areas.");

static PyObject* GemRB_GetAreas(PyObject*, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	size_t size = game->GetLoadedMapCount();
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		PyList_SetItem(list, i, gs->ConstructObject("Area", game->GetMap(i)->GetGlobalID()));
	}

	return list;
}

PyDoc_STRVAR(GemRB_GetCurrentArea__doc,
"Get current area.");

static PyObject* GemRB_GetCurrentArea(PyObject*, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	int area = game->FindMap(game->CurrentArea);
	if (area == -1)
		return RuntimeError("No current area!");
	return gs->ConstructObject("Area", game->GetMap(area)->GetGlobalID());
}

PyDoc_STRVAR(GemRB_Area_get_name__doc,
"Get area name.");

static PyObject* GemRB_Area_get_name(PyObject*, PyObject* args)
{
	int AreaID;

	if (!PyArg_ParseTuple(args, "i", &AreaID)) {
		return AttributeError(GemRB_Area_get_name__doc);
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Map *area = game->GetMapByGlobalID(AreaID);
	return PyString_FromString(area->GetScriptName());
}

PyDoc_STRVAR(GemRB_Area_get_actors__doc,
"Get a list of all actors in area.");

static PyObject* GemRB_Area_get_actors(PyObject*, PyObject* args)
{
	int AreaID;

	if (!PyArg_ParseTuple(args, "i", &AreaID)) {
		return AttributeError(GemRB_Area_get_name__doc);
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Map *area = game->GetMapByGlobalID(AreaID);

	size_t size = area->GetActorCount(true);
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		PyList_SetItem(list, i, gs->ConstructObject("Actor", area->GetActor(i, true)->GetGlobalID()));
	}

	return list;
}
