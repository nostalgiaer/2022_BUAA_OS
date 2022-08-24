.PHONY: all

run: ../gxemul/vmlinux
	cd .. && make                                                                                                                                
	/OSLAB/gxemul -E testmips -C R3000 -M 64 ../gxemul/vmlinux
