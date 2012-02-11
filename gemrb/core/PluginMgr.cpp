/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "PluginMgr.h"

#include "win32def.h"

#include "Plugin.h"
#include "ResourceDesc.h"

PluginMgr *PluginMgr::Get()
{
	static PluginMgr mgr;
	return &mgr;
}

PluginMgr::PluginMgr()
{
}

PluginMgr::~PluginMgr()
{
}


bool PluginMgr::IsAvailable(SClass_ID plugintype) const
{
	return plugins.find(plugintype) != plugins.end();
}

Plugin* PluginMgr::GetPlugin(SClass_ID plugintype) const
{
	std::map<SClass_ID, PluginFunc>::const_iterator iter = plugins.find(plugintype);
	if (iter != plugins.end())
		return iter->second();
	return NULL;
}

const std::vector<ResourceDesc>& PluginMgr::GetResourceDesc(const TypeID* type)
{
	return resources[type];
}

bool PluginMgr::RegisterPlugin(SClass_ID id, PluginFunc create)
{
	if (plugins.find(id) != plugins.end())
		return false;
	plugins[id] = create;
	return true;
}

void PluginMgr::RegisterResource(const TypeID* type, ResourceFunc create, const char *ext, ieWord keyType)
{
	resources[type].push_back(ResourceDesc(type,create,ext,keyType));
}

void PluginMgr::RegisterInitializer(void (*func)(Config const&))
{
	intializerFunctions.push_back(func);
}

void PluginMgr::RegisterCleanup(void (*func)(void))
{
	cleanupFunctions.push_back(func);
}

void PluginMgr::RunInitializers(Config const& config) const
{
	for (size_t i = 0; i < intializerFunctions.size(); i++)
		intializerFunctions[i](config);
}

void PluginMgr::RunCleanup() const
{
	for (size_t i = 0; i < cleanupFunctions.size(); i++)
		cleanupFunctions[i]();
}

bool PluginMgr::RegisterDriver(const TypeID* type, const char* name, PluginFunc create)
{
	driver_map &map = drivers[type];
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return false;
	map[name] = create;
	return true;
}

Plugin* PluginMgr::GetDriver(const TypeID* type, const char* name)
{
	driver_map &map = drivers[type];
	if (map.begin() == map.end())
		return NULL;
	driver_map::const_iterator iter = map.find(name);
	if (iter != map.end())
		return iter->second();
	return map.begin()->second();
}

const char* TypeExt(SClass_ID type)
{
	switch (type) {
		case IE_2DA_CLASS_ID: return "2da";
		case IE_ACM_CLASS_ID: return "acm";
		case IE_ARE_CLASS_ID: return "are";
		case IE_BAM_CLASS_ID: return "bam";
		case IE_BCS_CLASS_ID: return "bcs";
		case IE_BS_CLASS_ID: return "bs";
		case IE_BIF_CLASS_ID: return "bif";

		case IE_BIO_CLASS_ID:
		/* FIXME
			if (HasFeature(GF_BIOGRAPHY_RES)) {
				return "res";
			}
		 */
			return "bio";


		case IE_BMP_CLASS_ID: return "bmp";
		case IE_PNG_CLASS_ID: return "png";
		case IE_CHR_CLASS_ID: return "chr";
		case IE_CHU_CLASS_ID: return "chu";
		case IE_CRE_CLASS_ID: return "cre";
		case IE_DLG_CLASS_ID: return "dlg";
		case IE_EFF_CLASS_ID: return "eff";
		case IE_GAM_CLASS_ID: return "gam";
		case IE_IDS_CLASS_ID: return "ids";
		case IE_INI_CLASS_ID: return "ini";
		case IE_ITM_CLASS_ID: return "itm";
		case IE_MOS_CLASS_ID: return "mos";
		case IE_MUS_CLASS_ID: return "mus";
		case IE_MVE_CLASS_ID: return "mve";
		case IE_OGG_CLASS_ID: return "ogg";
		case IE_PLT_CLASS_ID: return "plt";
		case IE_PRO_CLASS_ID: return "pro";
		case IE_SAV_CLASS_ID: return "sav";
		case IE_SPL_CLASS_ID: return "spl";
		case IE_SRC_CLASS_ID: return "src";
		case IE_STO_CLASS_ID: return "sto";
		case IE_TIS_CLASS_ID: return "tis";
		case IE_TLK_CLASS_ID: return "tlk";
		case IE_TOH_CLASS_ID: return "toh";
		case IE_TOT_CLASS_ID: return "tot";
		case IE_VAR_CLASS_ID: return "var";
		case IE_VVC_CLASS_ID: return "vvc";
		case IE_WAV_CLASS_ID: return "wav";
		case IE_WED_CLASS_ID: return "wed";
		case IE_WFX_CLASS_ID: return "wfx";
		case IE_WMP_CLASS_ID: return "wmp";
	}
	return NULL;
}
