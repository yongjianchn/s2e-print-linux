#include "Linux_read.h"

#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/WindowsApi/Ntddk.h>

extern "C" {
#include "Linux_pro.h"
#include "config.h"
#include "qemu-common.h"
}

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <assert.h>

void print_linux_process_info()
{	
	s2e::plugins::Linux_read* m_Linux_read = (s2e::plugins::Linux_read*)(g_s2e->getPlugin("Linux_read"));

	m_Linux_read->init_kernel_info();
	uint32_t taskaddr = m_Linux_read->get_first_task_struct();
	uint32_t nowtask = taskaddr;
	int i = 0;
	char p_name[512];
	char m_name[512];
	do
	{
		/*get pid and process name, print*/
		int nowpid  = m_Linux_read->get_process_pid(nowtask);
		m_Linux_read->get_process_name(nowtask,p_name,512);
		printf("\n");
		printf("process %d--- pid:%d\n",i,nowpid);
		printf("          --- name:%s\n",p_name);
		/*end of print pid and process name*/
		
		/*get now_task's module map and print their info one by one*/
		uint32_t nowmmap = m_Linux_read->get_process_firstmmap(nowtask);
		int j = 0;
		while(nowmmap != 0)
		{
			m_Linux_read->get_module_name(nowmmap,m_name,512);//TODO
			int base = m_Linux_read->get_vmstart(nowmmap); 
			int size = m_Linux_read->get_vmend(nowmmap) - m_Linux_read->get_vmstart(nowmmap);
			printf("          --- module %d:  name:%s - base:%x - size:%d\n",j,m_name,base,size);
			nowmmap = m_Linux_read->get_next_mmap(nowmmap);
			j++;
		}
		/*end print module info*/
		
		nowtask = m_Linux_read->get_next_task_struct(nowtask);
		i++;
	}while(i < 100 && nowtask != taskaddr);

}
