s2e-print-linux
===============

add plugin to s2e to print info of some linux OS

添加了插件得到linux的进程信息，添加了qemu控制台命令，输出查看得到的信息


编译的时候，如果第一次没有成功，不要clean，再make一次。我发现s2e本身存在第一次出错的可能。
 
测试插件功能的时候，在命令窗口输入print_linux_process_info可以看到显示信息，但是最好等到虚拟机有进程起来，否则会崩溃。
 
请注意修改run.sh中的各种路径
 
请注意此插件暂时只支持ubuntu9.04
