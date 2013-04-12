s2e = {
	kleeArgs = {
		"--use-batching-search",
		"--use-random-path",
		
		"--state-shared-memory",
		"--flush-tbs-on-state-switch=false",

		"--use-concolic-execution=false",
		"--use-fast-cex-solver=false",
	}
}

plugins = {
	"BaseInstructions",
	"RawMonitor",
	--"ModuleExecutionDetector",
	"FunctionMonitor",
	"MemoryManager",
}

pluginsConfig = {}

pluginsConfig.RawMonitor = {
	kernelStart  = 0xc0000000,
	__kmalloc = {
		delay = false,
		name = "__kmalloc",
		start = 0xc02620b0,
		size = 0x10,
		nativebase = 0x80480000,
		kernelmode = true
	}
}

pluginsConfig.MemoryManager = {

	--ip_options_get------------
	--0xc02620b0 push
	--0xc02620d0 call
	--0xc02620d5 mov
	--0xc02621f0 rep movsl
	--__kmalloc-----------------
	--0xc013aa30 push
	--memcpy--------------------
	--0xc01c61dc rep movsl
	terminateOnBugs = true,
	detectOnly__kmalloc_ip_options_get = true,
	detectOnlyMemcpy_ip_options_get = true,
	pc_ip_options_get_call___kmalloc = 0xc02620d0,
	pc___kmalloc_return_ip_options_get = 0xc02620d5,
	pc_rep_movsl_ip_options_get = 0xc02621f0,
	pc___kmalloc = 0xc013aa30,
}
