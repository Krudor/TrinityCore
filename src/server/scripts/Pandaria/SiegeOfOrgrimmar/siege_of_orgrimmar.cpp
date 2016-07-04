#include "ScriptMgr.h"
#include "siege_of_orgrimmar.h"

class at_soo_norushen : public AreaTriggerScript
{
	public:
		at_soo_norushen() : AreaTriggerScript("at_soo_norushen") { }

		bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/, bool /*entered*/) override
		{
			InstanceScript* instance = player->GetInstanceScript();
			if (!instance || instance->GetData(DATA_INSTANCE_PROGRESS) < INSTANCE_PROGRESS_REACHED_NORUSHEN)
				return true;

			instance->SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_REACHED_NORUSHEN);
			return true;
		}
};

void AddSC_siege_of_orgrimmar()
{
	new at_soo_norushen();
}
