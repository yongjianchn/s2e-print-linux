#ifndef S2E_PLUGINS_LINUX_READ_H
#define S2E_PLUGINS_LINUX_READ_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include <inttypes.h>
#include <set>
#include <map>
#include <stdint.h>
namespace s2e {
namespace plugins {

class Linux_read : public Plugin
{
	S2E_PLUGIN
public:
	uint32_t hookingpoint;
	uint32_t hookingpoint2;
	uint32_t taskaddr;
	int tasksize;
	int listoffset;
	int pidoffset;
	int mmoffset;
	int pgdoffset;
	int commoffset;
	int commsize;
	int vmstartoffset;
	int vmendoffset;
	int vmnextoffset;
	int vmfileoffset;
	int dentryoffset;
	int dnameoffset;
	int dinameoffset;
public:
	S2EExecutionState *m_state;
public:

	Linux_read(S2E* s2e) : Plugin(s2e){}
	void initialize();
	
	void get_mstate(ExecutionSignal *signal,
					  S2EExecutionState *state,
					  TranslationBlock *tb,
					  uint64_t pc);
	void init_kernel_info(void);
	uint32_t get_first_task_struct(void);
	uint32_t get_next_task_struct(uint32_t pcbaddr);
	int get_process_pid(uint32_t pcbaddr);
	void get_process_name(uint32_t pcbaddr, char *buf, int size);
};

}//namespace plugins
}//namespace s2e

#endif
