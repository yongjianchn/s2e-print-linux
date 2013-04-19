#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

#include "CallTracer.h"

extern "C" {
#include "config.h"
#include "qemu-common.h"
}

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(CallTracer, "do check for all call", "CallTracer","FunctionMonitor",);

void CallTracer::initialize()
{
	m_onAllCall = ((FunctionMonitor*)(s2e()->getPlugin("FunctionMonitor")))->onAllCall.connect(
					  sigc::mem_fun(*this,&CallTracer::onAllCall));
	FILE *wanted_functions = fopen("wanted_functions", "a");
	fprintf(wanted_functions,"---------------------------\n");
	fclose(wanted_functions);
}
void CallTracer::onAllCall(S2EExecutionState* state)
{
	for(int i = 0; i < 5; i++)
	{
		klee::ref<klee::Expr> value = getArgValue(state,i*4);
		if (!isa<klee::ConstantExpr>(value))//如果是符号值
		{
			uint64_t eip = state->getPc();
			bool exist = false;
			for (int j = 0; j < num; j++)
			{
				if (addrs[j] == eip && offsets[j] == i)
					exist = true;
			}
			if (eip == 0xc010cc20)//过滤掉常用的系统中断相关调转
				exist = true;
			
			if (exist == false)
			{
				addrs[num] = eip;
				offsets[num] = i;
				num ++;
				writeToFile(eip,i);
			}
		}
	}
}
klee::ref<klee::Expr> CallTracer::getArgValue(S2EExecutionState* state, int offset)
{	
	uint64_t sp = state->getSp();
	klee::ref<klee::Expr> value = state->readMemory(sp + offset, klee::Expr::Int32);
	return value;
}
void CallTracer::writeToFile(uint64_t eip, int seq)
{
	FILE *wanted_functions = fopen("wanted_functions", "a");
	fprintf(wanted_functions,"%lX    %d\n", eip, seq);
	fclose(wanted_functions);
}
}//namespace plugins
}//namespace s2e
