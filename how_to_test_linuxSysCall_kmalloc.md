1. 测试程序如果在本机（ubuntu12.04 64）上直接gcc编译，是64位程序，在redhat中跑不了；
2. 如果使用本机gcc加上-m32选项，编译出32位执行程序，可能是gcc版本过高，在redhat中还是运行有问题；
3. 我是用qemu（without s2e）在redhat中编译，再scp to host；然后在host上用s2e运行redhat2.6快照（由于kmalloc地址是写死的，所以必须使用同一个编译的内核，或者修改memorymanager中的kmalloc地址为测试内核的相应kmalloc地址）。
4. 应该也可以在host上使用老版本的gcc -m32编译（记忆中之前用过，这次没试）
5. 注意修改配置文件中hostfile的指定路径
6. 注意修改run.sh脚本中的一些路径
7. guest_program文件夹下子文件夹中已经有编译好的可以在redhat上跑的测试程序，可以直接用于测试。
8. sendto和sendmsg的测试程序符号化过多的参数，可能需要减少，才能成功跑完所有的state。
9. 待补充…


