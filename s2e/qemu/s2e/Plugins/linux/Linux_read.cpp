#include "Linux_read.h"
#include "Linux_pro.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <assert.h>

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(Linux_read, "Linux_read", "",);


void Linux_read::initialize()
{
	init_kernel_info();
	s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
        sigc::mem_fun(*this, &Linux_read::get_mstate));
}

void Linux_read::get_mstate(ExecutionSignal *signal,
							 S2EExecutionState *state,
							 TranslationBlock *tb,
							 uint64_t pc)
{
	m_state = state;
}

void Linux_read::init_kernel_info()
{
	//redhat 9.0
	/*
	hookingpoint = 0xC0120180;
	hookingpoint2 = 0x00000000;
	taskaddr = 0xC031E000;
	tasksize = 1424;
	listoffset = 72;
	pidoffset = 108;
	mmoffset = 44;
	pgdoffset = 12;
	commoffset = 558;
	commsize = 16;
	vmstartoffset = 4;
	vmendoffset = 8;
	vmnextoffset = 12;
	vmfileoffset = 56;
	dentryoffset = 8;
	dnameoffset = 60;
	dinameoffset = 96;
   */
	//ubuntu 9.04 (Ubuntu 2.6.28-11.42) 

	hookingpoint = 0xc01449b0;
	hookingpoint2 = 0x00000000;
    taskaddr = 0xC0687340;
    tasksize = 3212;
    listoffset = 452;
    pidoffset = 496;
    mmoffset = 460;
    pgdoffset = 36;
    commoffset = 792;
    commsize = 16;
    vmstartoffset = 4;
    vmendoffset = 8;
    vmnextoffset = 12;
    vmfileoffset = 72;
    dentryoffset = 12;
    dnameoffset = 28;
    dinameoffset = 96 ;

}
uint32_t Linux_read::get_first_task_struct(){
	return taskaddr;
}

uint32_t Linux_read::get_next_task_struct(uint32_t pcbaddr){
	
	uint32_t retval;
    uint32_t next;

    m_state->readMemoryConcrete(pcbaddr + listoffset + sizeof(uint32_t), 
							  &next,sizeof(uint32_t));
    retval = next - listoffset;
 
    return retval;
}

int Linux_read::get_process_pid(uint32_t pcbaddr){
	int pid;
	m_state->readMemoryConcrete(pcbaddr + pidoffset, &pid, sizeof(pid));
	return pid;
}

void Linux_read::get_process_name(uint32_t pcbaddr, char *buf, int size){
	m_state->readMemoryConcrete(pcbaddr + commoffset,buf, 
							  size);
}
uint32_t Linux_read::get_process_firstmmap(uint32_t pcbaddr){
	uint32_t mmaddr,mmap;
	m_state->readMemoryConcrete(pcbaddr + mmoffset, &mmaddr, sizeof(mmaddr));
	m_state->readMemoryConcrete(mmaddr, &mmap, sizeof(mmap));
	return mmap;
}

uint32_t Linux_read::get_next_mmap(uint32_t mmapaddr){
	uint32_t mmap;
	m_state->readMemoryConcrete(mmapaddr + vmnextoffset, &mmap, sizeof(mmap));
	return mmap;
}

void Linux_read::get_module_name(uint32_t mmap,char *name,int size){
	uint32_t vmfile, dentry;
	m_state->readMemoryConcrete(mmap + vmfileoffset, &vmfile, sizeof(vmfile));
	m_state->readMemoryConcrete(vmfile + dentryoffset, &dentry, sizeof(dentry));
	m_state->readMemoryConcrete(dentry + dinameoffset, name, size < 36 ? size : 36);
}

int Linux_read::get_vmstart(uint32_t mmap){
	uint32_t vmstart;
	m_state->readMemoryConcrete(mmap + vmstartoffset, &vmstart, sizeof(vmstart));
	return vmstart;
}

int Linux_read::get_vmend(uint32_t mmap){
	uint32_t vmend;
	m_state->readMemoryConcrete(mmap + vmendoffset, &vmend, sizeof(vmend));
	return vmend;
}


}//namespace plugins
}//namespace s2e
