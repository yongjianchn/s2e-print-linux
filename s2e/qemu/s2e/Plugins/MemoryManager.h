#ifndef S2E_MEMORYMANAGER_H
#define S2E_MEMORYMANAGER_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/FunctionMonitor.h>

#include "ModuleExecutionDetector.h"
#include "FunctionMonitor.h"

#include <string>
#include <vector>
using namespace std;

namespace s2e{

namespace plugins{

class MemoryManager : public Plugin
{
	S2E_PLUGIN
public:
	MemoryManager(S2E* s2e): Plugin(s2e) {};
	
	FunctionMonitor *m_functionMonitor;
	struct grantedMemory{
		uint32_t address;
		klee::ref<klee::Expr> symValue;
	};
	grantedMemory m_grantedMemory;
	vector<grantedMemory> memory_granted_expression;
public:
	void initialize();
	
public:
	//connect signals
	void onFunctionCall_fro(S2EExecutionState *state, uint64_t pc);
	void onFunctionReturn_fro(S2EExecutionState *state, uint64_t pc);
	void onMemcpyExecute(S2EExecutionState *state, uint64_t pc);
	void onTranslateInstructionStart(
		ExecutionSignal *signal,
		S2EExecutionState* state,
		TranslationBlock *tb,
		uint64_t pc);
	void onTranslateBlockEnd(ExecutionSignal* signal, 
										   S2EExecutionState* state,
										   TranslationBlock* tb,
										   uint64_t endpc/* ending instruction pc */,
										   bool valid/* static target is valid */,
										   uint64_t targetpc/* static target pc */
										  );
	void onMallocStart(S2EExecutionState* state,
									  const ModuleDescriptor& mdsc);
	
	void onFunctionCall(S2EExecutionState*,
						 FunctionMonitorState*);
	void onFunctionReturn(S2EExecutionState*,bool);
	
	klee::ref<klee::Expr> getArgValue(S2EExecutionState* state);
	
};

}//namespace plugins
}//namespace s2e
#endif
