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
	char name[512];
	do
	{
		int nowpid  = m_Linux_read->get_process_pid(nowtask);
		m_Linux_read->get_process_name(nowtask,name,512);
		
		printf("porcess %d --- pid:%d\n",i,nowpid);
		printf("          |--- name:%s\n",name);
		
		printf("nowtask:%x \n",nowtask);
		nowtask = m_Linux_read->get_next_task_struct(nowtask);
		printf("nexttask:%x \n",nowtask);
		i++;
	}while(i < 100 && nowtask != taskaddr);

}
