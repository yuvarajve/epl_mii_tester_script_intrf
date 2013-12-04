INCLUDES = -I../module_pcapng/src -I ../sc_xscope_support-master/host_library 

all:
	gcc -pthread ../sc_xscope_support-master/host_library/xscope_host_shared.c mii_tester_script.c -o mii_tester_script $(INCLUDES)

clean:
	@rm -f mii_tester_script
