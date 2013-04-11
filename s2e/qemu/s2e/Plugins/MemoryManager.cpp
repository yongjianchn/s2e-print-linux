
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
	m_RawMonitor = (s2e::plugins::RawMonitor*)(s2e()->getPlugin("RawMonitor"));
	m_ModuleExecutionDetector = (s2e::plugins::ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
	CorePlugin *plg = s2e()->getCorePlugin();
	
	//m_ModuleExecutionDetector->onModuleLoad.connect(sigc::mem_fun(*this,&MemoryManager::onModuleLoad));
	/*
	备注: m_ModuleExecutionDetector的onModuleLoad不起作用，可以用RawMonitor的onModuleLoad信号
	*/
	
	//m_RawMonitor->onModuleLoad.connect(sigc::mem_fun(*this,&MemoryManager::onModuleLoad));
	/*
	备注：RawMonitor被使用的时候，functionMonitor不起作用（即使成功注册，也不会发射onFunctionCall等信号）
	这个问题不明原因，可能是RaWMonitor提供的东西不如WindowsMonitor充足，使用了反而影响functionMonitor判断；
	因此配置文件里不要使用RawMonitor！！！
	*/
    plg->onTranslateInstructionStart.connect(sigc::mem_fun(*this, &MemoryManager::onTranslateInstructionStart));
}

void MemoryManager::onTranslateInstructionStart(
			ExecutionSignal *signal,
			S2EExecutionState* state,
			TranslationBlock *tb,
			uint64_t pc)
{	
	//ip_options_get-------
	//0xc02620b0 push
	//0xc02620d0 call
	//0xc02620d5 mov
	//0xc02621f0 rep movsl
	//__kmalloc------------
	//0xc013aa30 push
	//memcpy---------------
	//0xc01c61dc rep movsl
	if(pc == 0xc02620b0)//进入ip_options_get的时候在functionMonitor中注册__kmalloc
	{	
		s2e()->getMessagesStream() << "---on PC = 0xc02620b0 register onFunctionCall" << '\n';
		uint64_t wantedPc = 0xc013aa30;
		uint64_t cr3 = state->getPid();
		FunctionMonitor::CallSignal *cs = m_functionMonitor->getCallSignal(state, wantedPc, cr3);
		cs->connect(sigc::mem_fun(*this, &MemoryManager::onFunctionCall));
	}
	/*
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
	*/
	
	//memcpy
	uint32_t inst=0;	
	state->readMemoryConcrete(pc, &inst, 2);
	inst=inst&0xf0ff;
	//if(inst == 0xa0f3)//所有的rep ***指令
	if(pc == 0xc02621f0)//针对rep movsl from ip_options_get
	{
		signal->connect(sigc::mem_fun(*this, 
									  &MemoryManager::onMemcpyExecute));
	}
}

void MemoryManager::onModuleLoad(S2EExecutionState* state,
								  const ModuleDescriptor& module)
{
	s2e()->getMessagesStream() << "---onModuleLoad" << '\n';
	uint64_t wantedPc = 0xc013aa30;
	uint64_t cr3 = state->getPid();
	FunctionMonitor::CallSignal *cs = m_functionMonitor->getCallSignal(state, wantedPc, cr3);
	cs->connect(sigc::mem_fun(*this, &MemoryManager::onFunctionCall));
}
/*
void MemoryManager::onFunctionCall_fro(S2EExecutionState *state, uint64_t pc)
{
	g_s2e->getMessagesStream() << "---ip_options_get call __kmalloc" << '\n';
	//get size
	size = MemoryManager::getArgValue(state);
	s2e()->getMessagesStream() << "分配的size表达式(如果是数，那么是16进制)：" << size << '\n';
}

void MemoryManager::onFunctionReturn_fro(S2EExecutionState *state, uint64_t pc)
{
	g_s2e->getMessagesStream() << "---__kmalloc return to ip_options_get" << '\n';
	//get address
	state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),&address , 4);
	s2e()->getMessagesStream() << "分配的address （eax）:" << hexval(address) << '\n';
	//check
	check___kmalloc(address,size,state);
	grant();
}
*/
void MemoryManager::onFunctionCall(S2EExecutionState* state, FunctionMonitorState *fns)
{
	s2e()->getMessagesStream() << "---onFunctionCall" << '\n';
	//get size
	size = MemoryManager::getArgValue4(state);
	s2e()->getMessagesStream() << "分配的size表达式(如果是数，那么是16进制)：" << size << '\n';
	//注册return时调用的函数
	bool test = false;
	FUNCMON_REGISTER_RETURN_A(state, fns, MemoryManager::onFunctionReturn, test);
}

void MemoryManager::onFunctionReturn(S2EExecutionState* state,bool test)
{
	g_s2e->getMessagesStream() << "---onFunctionReturn" << '\n';
	//get address
	state->readCpuRegisterConcrete(CPU_OFFSET(regs[R_EAX]),&address , 4);
	s2e()->getMessagesStream() << "分配的address （eax）:" << hexval(address) << '\n';
	//check
	check___kmalloc(address,size,state);
	grant();
}
void MemoryManager::onMemcpyExecute(S2EExecutionState *state, uint64_t pc)
{
	//get edi
	state->readCpuRegisterConcrete(offsetof(CPUX86State, regs[R_EDI]), &edi, sizeof(edi));
	//get ecx
	ecx = state->readCpuRegister(offsetof(CPUX86State, regs[R_ECX]), klee::Expr::Int32);
	s2e()->getMessagesStream() << "---onMemcpyExecute   pc:  " << hexval(pc) << "\n";
	s2e()->getMessagesStream() << "edi : " << hexval(edi) << "\n"
							   << "ecx : " << ecx << "\n";
	//check
	check_rep(edi,ecx,state);
}
klee::ref<klee::Expr> MemoryManager::getArgValue(S2EExecutionState* state)
{	
	uint64_t sp = state->getSp();
	klee::ref<klee::Expr> size = state->readMemory(sp, klee::Expr::Int32);
	return size;
}
klee::ref<klee::Expr> MemoryManager::getArgValue4(S2EExecutionState* state)
{	
	uint64_t sp = state->getSp();
	klee::ref<klee::Expr> size = state->readMemory(sp + 0x4, klee::Expr::Int32);
	return size;
}

void MemoryManager::grant()
{
	m_grantedMemory.address = address;
	m_grantedMemory.size = size;
	g_s2e->getMessagesStream() << "---grant Memory map address: " << hexval(address) << " size: " << size << '\n';
	memory_granted_expression.push_back(m_grantedMemory);
}

bool MemoryManager::check___kmalloc(uint32_t address, klee::ref<klee::Expr> size, S2EExecutionState *state)
{
	bool isok = true;
	//check 1 判断分配的起始地址的合法性
	if(address ==  0x0)
	{
		
		s2e()->getMessagesStream() << "===============================================" << '\n';
		s2e()->getMessagesStream() << "BUG：__kmalloc返回地址是0x0！！！" << '\n';
		s2e()->getMessagesStream() << "===============================================" << '\n';
		s2e()->getExecutor()->terminateStateEarly(*state, "BUG: __kmalloc address is 0x0\n");
		isok = false;
	}
	//check 2 判断size本身的合法性
	if (isa<klee::ConstantExpr>(size))
	{
		//具体值
		int value = cast<klee::ConstantExpr>(size)->getZExtValue();
		if (value <= 0 || value >= 0x0fff0000)
		{
			s2e()->getMessagesStream() << "============================================================" << '\n';
			s2e()->getMessagesStream() << "BUG: __kmalloc [Size <= 0||Size >= 0x0fff0000] Size: " << value << '\n';
			s2e()->getMessagesStream() << "============================================================" << '\n';
			s2e()->getExecutor()->terminateStateEarly(*state, "BUG: __kmalloc size is not valid\n");
			isok = false;
	
		}
	}
	return isok;
}
bool MemoryManager::check_rep(uint32_t edi, klee::ref<klee::Expr> ecx, S2EExecutionState *state)
{
	bool isok = true;
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
			s2e()->getMessagesStream() << "============================================================" << '\n';
			s2e()->getMessagesStream() << "BUG: memcpy edi is too small，can not access. edi:" << hexval(edi) << '\n';
			s2e()->getMessagesStream() << "============================================================" << '\n';
			isok = false;
			//终结
			s2e()->getExecutor()->terminateStateEarly(*state, "BUG: edi can not be accessed\n");
		}
		//检查size
		int ecx_con = cast<klee::ConstantExpr>(ecx)->getZExtValue();
		if(ecx_con < 0 || ecx_con > 0x0fff0000) 
		{
			s2e()->getMessagesStream() << "============================================================" << '\n';
			s2e()->getMessagesStream() << "BUG: memcpy [Size < 0||Size > 0x0fff0000] Size: " << hexval(ecx_con) << '\n';
			s2e()->getMessagesStream() << "============================================================" << '\n';
			isok = false;
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
	return isok;
}

}//namespace plugins

}//namespace s2e
