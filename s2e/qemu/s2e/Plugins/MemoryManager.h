#ifndef S2E_MEMORYMANAGER_H
#define S2E_MEMORYMANAGER_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/FunctionMonitor.h>

#include "ModuleExecutionDetector.h"
#include "FunctionMonitor.h"
#include "RawMonitor.h"

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
	RawMonitor *m_RawMonitor;
	ModuleExecutionDetector *m_ModuleExecutionDetector;
	
	uint32_t address;
	klee::ref<klee::Expr> size;
	uint32_t edi;
	klee::ref<klee::Expr> ecx;
	struct grantedMemory{
		uint32_t address;
		klee::ref<klee::Expr> size;
	};
	grantedMemory m_grantedMemory;
	vector<grantedMemory> memory_granted_expression;
public:
	void initialize();
	
public:
	void onTranslateInstructionStart(ExecutionSignal *signal,
									 S2EExecutionState* state,
									 TranslationBlock *tb,
									 uint64_t pc);
	void onModuleLoad(S2EExecutionState* state,
					   const ModuleDescriptor& mdsc);
public:
	void onFunctionCall_fro(S2EExecutionState *state, uint64_t pc);
	void onFunctionReturn_fro(S2EExecutionState *state, uint64_t pc);
	void onFunctionCall(S2EExecutionState*,FunctionMonitorState*);
	void onFunctionReturn(S2EExecutionState*,bool);
	void onMemcpyExecute(S2EExecutionState *state, uint64_t pc);
	
	klee::ref<klee::Expr> getArgValue(S2EExecutionState* state);
	klee::ref<klee::Expr> getArgValue4(S2EExecutionState* state);

	bool check___kmalloc(uint32_t address, klee::ref<klee::Expr> size, S2EExecutionState *state);
	bool check_rep(uint32_t edi, klee::ref<klee::Expr> ecx, S2EExecutionState *state);
	
	void grant(void);
};

}//namespace plugins
}//namespace s2e
#endif
