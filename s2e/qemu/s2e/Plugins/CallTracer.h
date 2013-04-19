#ifndef S2E_CALLTRACER_H
#define S2E_CALLTRACER_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/FunctionMonitor.h>

namespace s2e {
namespace plugins {

class CallTracer : public Plugin
{
	S2E_PLUGIN
private:
	uint64_t addrs[1000];
	int offsets[1000];
	int num;
public:
	CallTracer(S2E* s2e): Plugin(s2e){num = 0;};
	
	sigc::connection m_onAllCall;
	
	void initialize();
	
public:
	void onAllCall(S2EExecutionState* state);//
	klee::ref<klee::Expr> getArgValue(S2EExecutionState* state, int offset);
	void writeToFile(uint64_t eip, int seq);
};//class CallTracer

}//namespace plugins
}//namespace s2e
#endif
