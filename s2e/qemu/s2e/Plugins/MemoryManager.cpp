
#include "MemoryManager.h"
#include "LibraryCallMonitor.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/S2EExecutor.h>
#include <klee/Solver.h>

#include <iostream>
#include <sstream>
extern "C" {
#include "config.h"
#include "qemu-common.h"
#include "sysemu.h"
#include "cpus.h"

#include "tcg-llvm.h"
#include "cpu.h"

	extern struct CPUX86State *env;
}
namespace s2e{

namespace plugins{

S2E_DEFINE_PLUGIN(MemoryManager, "MemoryManager plugin", "",);

void MemoryManager::initialize()
{
	m_functionMonitor = (s2e::plugins::FunctionMonitor*)(s2e()->getPlugin("FunctionMonitor"));
	/*
	((s2e::plugins::ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector")))->onModuleLoad.connect(
		sigc::mem_fun(*this,&MemoryManager::onMallocStart)
	);
	*/
	CorePlugin *plg = s2e()->getCorePlugin();
    plg->onTranslateInstructionStart.connect(sigc::mem_fun(*this, 
			&MemoryManager::onTranslateInstructionStart));
//	plg->onTranslateBlockEnd.connect(sigc::mem_fun(*this, 
//									 &MemoryManager::onTranslateBlockEnd));
	
}

void MemoryManager::onTranslateInstructionStart(
			ExecutionSignal *signal,
			S2EExecutionState* state,
			TranslationBlock *tb,
			uint64_t pc)
{
	if(pc == 0xc02620d0)//call __kmalloc in ip_options_get
	{	
		signal->connect(sigc::mem_fun(*this, 
									  &MemoryManager::onFunctionCall_fro));
	}
	if(pc == 0xc02620d5)//return to ip_options_get
	{
		signal->connect(sigc::mem_fun(*this, 
									  &MemoryManager::onFunctionReturn_fro));
	}
	//memcpy
	/*
	uint32_t inst=0;	
	state->readMemoryConcrete(pc, &inst, 2);
	inst=inst&0xf0ff;
	if(inst == 0xa0f3)
	*/
	if(pc == 0xc02621f0)//rep movsl from ip_options_get
	{
		signal->connect(sigc::mem_fun(*this, 
									  &MemoryManager::onMemcpyExecute));
	}
}
void MemoryManager::onMemcpyExecute(S2EExecutionState *state, uint64_t pc)
{
	uint32_t edi;
	state->readCpuRegisterConcrete(offsetof(CPUX86State, regs[R_EDI]), &edi, sizeof(edi));
	
	klee::ref<klee::Expr> ecx = state->readCpuRegister(offsetof(CPUX86State, regs[R_ECX]), klee::Expr::Int32);
	s2e()->getMessagesStream() << "---onMemcpyExecute   pc:  " << hexval(pc) << "\n";
	s2e()->getMessagesStream() << "edi : " << edi << "\n"
							   << "ecx : " << ecx << "\n";

	//check 3 检查memcpy访问是否合法
	//concrete
	if(isa<klee::ConstantExpr>(ecx))
	{
		//检查edi
		int imax = memory_granted_expression.capacity();
		bool bigger = 1;
		for (int i = 0; i< imax; i++)
		{
			uint32_t edi_i = memory_granted_expression.at(i).address;
			if (edi > edi_i)
			{
				bigger = 1;
			}
			else bigger = 0;
		}
		if (bigger == 0)
		{
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			s2e()->getMessagesStream() << "BUG: memcpy edi is too small，can not access. edi:"<<edi<<'\n';
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			//终结
			s2e()->getExecutor()->terminateStateEarly(*state, "BUG: edi can not be accessed\n");
		}
		//检查size
		int ecx_con = cast<klee::ConstantExpr>(ecx)->getZExtValue();
		if(ecx_con < 0 || ecx_con > 0x0fff0000) 
		{
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			s2e()->getMessagesStream() << "BUG: memcpy [Size < 0||Size > 0x0fff0000] Size: "<<hexval(ecx_con)<<'\n';
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			s2e()->getExecutor()->terminateStateEarly(*state, "BUG: memcpy lenth is not valid\n");
		}
	}
	//symbolic
	/*
	else
	{
		klee::ref<klee::Expr> cond = klee::SgtExpr::create(
										 ecx, klee::ConstantExpr::create(
											 0x20, ecx.get()->getWidth()));
		s2e()->getMessagesStream() << "assert cond : " << cond << '\n';
		bool isTrue; 
		if (!(s2e()->getExecutor()->getSolver()->mayBeTrue(klee::Query(state->constraints, cond), isTrue))) { 
			s2e()->getMessagesStream() << "failed to assert the condition" << '\n';
			return;
		} 
		if (isTrue) {
			vector<VarValuePair> inputs;
			ConcreteInputs::iterator it; 
			
			s2e()->getExecutor()->addConstraint_pub(*state, cond);
			s2e()->getExecutor()->getSymbolicSolution(*state, inputs);
			
			s2e()->getMessagesStream() << "====================================================" << '\n';
			s2e()->getMessagesStream() << "BUG:memcpy crash detected!" << '\n'
									   << "input value : " << '\n';
									   
			for (it = inputs.begin(); it != inputs.end(); ++it) {
				const VarValuePair &vp = *it;
				s2e()->getMessagesStream() << "---------" << vp.first << " : ";
		
				for (unsigned i=0; i<vp.second.size(); ++i) {
					s2e()->getMessagesStream() << hexval((unsigned char) vp.second[i]) << " ";
				}
				s2e()->getMessagesStream() << '\n';
			}
			s2e()->getMessagesStream() << "====================================================" << '\n';
		}
	}
	*/
}
/*
void MemoryManager::onMallocStart(S2EExecutionState* state,
								  const ModuleDescriptor& module)
{
	uint64_t eip = state->getPc();
	s2e()->getMessagesStream(state) << "xyj now in onMallocStart(),means library->call signal"<<"\neip:"<< hexval(eip)<<'\n';

	if(eip == 0xc03657f6 || eip == 0xc0366886)
	{
		s2e()->getMessagesStream(state) << "xyj detect malloc  eip: " <<eip<< '\n';
		//get klee expression
		m_grantedMemory.symValue = MemoryManager::getArgValue(state);
		//register return signal
		uint64_t cr3 = state->getPid();
		
		FunctionMonitor::CallSignal *cs = m_functionMonitor->getCallSignal(state, eip, cr3);
		cs->connect(sigc::mem_fun(*this, &MemoryManager::onFunctionCall));
		
	}
}
*/
void MemoryManager::onFunctionCall_fro(S2EExecutionState *state, uint64_t pc)
{
	//message
	g_s2e->getMessagesStream() << "---call __kmalloc"<<'\n';
	g_s2e->getMessagesStream() << "即将获取size表达式"<<'\n';	
	s2e()->getMessagesStream()<<"pc:"<<hexval(pc)<<'\n';

	m_grantedMemory.symValue = MemoryManager::getArgValue(state);
	s2e()->getMessagesStream()<<"表达式(如果是数，那么是16进制)："<<m_grantedMemory.symValue<<'\n';
	
	
	/*//连接FunctionMonitor插件
	uint64_t eip = state->getPc();
	uint64_t cr3 = state->getPid();
	FunctionMonitor::CallSignal *cs = m_functionMonitor->getCallSignal(state, eip, cr3);
	cs->connect(sigc::mem_fun(*this, 
							  &MemoryManager::onFunctionCall));
	*/
}

void MemoryManager::onFunctionReturn_fro(S2EExecutionState *state, uint64_t pc)
{
	//message
	g_s2e->getMessagesStream() << "---现在是返回到调用ip_options_get"<<'\n';
	g_s2e->getMessagesStream() << "即将获取返回值eax"<<'\n';	
	uint32_t eax;
	state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),&eax , 4);
	s2e()->getMessagesStream() << "pc: "<< hexval(pc) <<'\n';
	s2e()->getMessagesStream() << "eax:"<< hexval(eax) <<'\n';
	m_grantedMemory.address = eax;
	
	g_s2e->getMessagesStream() << "---grant Memory map address: "<<eax<<"lenth: "<<m_grantedMemory.symValue <<'\n';
	memory_granted_expression.push_back(m_grantedMemory);
	//check 1 判断分配的起始地址的合法性
	if(eax ==  0x0)
	{
		
		s2e()->getMessagesStream() << "==============================================="<<'\n';
		s2e()->getMessagesStream() << "BUG：__kmalloc返回地址是0x0！！！"<<'\n';
		s2e()->getMessagesStream() << "==============================================="<<'\n';
		s2e()->getExecutor()->terminateStateEarly(*state, "BUG: __kmalloc address is 0x0\n");
	}
	//check 2 判断size本身的合法性
	if (isa<klee::ConstantExpr>(m_grantedMemory.symValue))
	{
		//具体值
		int value = cast<klee::ConstantExpr>(m_grantedMemory.symValue)->getZExtValue();
		if (value <= 0 || value >= 0x0fff0000)
		{
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			s2e()->getMessagesStream() << "BUG: __kmalloc [Size <= 0||Size >= 0x0fff0000] Size: "<<value<<'\n';
			s2e()->getMessagesStream() << "============================================================"<<'\n';
			s2e()->getExecutor()->terminateStateEarly(*state, "BUG: __kmalloc size is not valid\n");

		}
	}
	
}

void MemoryManager::onFunctionCall(S2EExecutionState* state, FunctionMonitorState *fns)
{
	//注册return时调用的函数
	bool test = false;
	FUNCMON_REGISTER_RETURN_A(state, fns, MemoryManager::onFunctionReturn, test);
}

void MemoryManager::onFunctionReturn(S2EExecutionState* state,bool test)
{
	//eax
	uint32_t eax;
	state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),&eax , 4);
	//message
	g_s2e->getMessagesStream() << "---onFunctionReturn"<<'\n';
	//s2e()->getMessagesStream() << "pc: "<< hexval(pc) <<'\n';
	s2e()->getMessagesStream() << "eax:"<< hexval(eax) <<'\n';
	
	m_grantedMemory.address = eax;
	//更新vector
	memory_granted_expression.push_back(m_grantedMemory);
}

klee::ref<klee::Expr> MemoryManager::getArgValue(S2EExecutionState* state)
{	
	uint64_t sp = state->getSp();
	klee::ref<klee::Expr> symValue = state->readMemory(sp, klee::Expr::Int32);
	//klee::ref<klee::Expr> symValue4 = state->readMemory(sp + 0x4, klee::Expr::Int32);
	//klee::ref<klee::Expr> symValue8 = state->readMemory(sp + 0x8, klee::Expr::Int32);
	//klee::ref<klee::Expr> symValuec = state->readMemory(sp + 0xc, klee::Expr::Int32);
	
	/*
	int test;state->readMemoryConcrete(sp,&test,sizeof(int));
	int test4;state->readMemoryConcrete(sp+0x4,&test4,sizeof(int));
	int test8;state->readMemoryConcrete(sp+0x8,&test8,sizeof(int));
	int testc;state->readMemoryConcrete(sp+0xc,&testc,sizeof(int));
	s2e()->getMessagesStream() << "xyj to see is it the lenth ---sp+0x0  "<<test<<'\n';
	s2e()->getMessagesStream() << "xyj to see is it the lenth ---sp+0x4  "<<test4<<'\n';
	s2e()->getMessagesStream() << "xyj to see is it the lenth ---sp+0x8  "<<test8<<'\n';
	s2e()->getMessagesStream() << "xyj to see is it the lenth ---sp+0xc  "<<testc<<'\n';
	*/
	return symValue;
}

}//namespace plugins

}//namespace s2e
