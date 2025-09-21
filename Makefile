# programs for cshell
.PHONY: all clean rebuild veryclean

exe:
	make -f target_b.mk target=$(target)
	make -f target_a.mk target=$(target)
	
clean:
	make -f target_b.mk clean target=$(target)
	make -f target_a.mk clean target=$(target)

rebuild:
	make -f target_b.mk rebuild target=$(target)
	make -f target_a.mk rebuild target=$(target)

veryclean:
	make -f target_b.mk veryclean
	make -f target_a.mk veryclean