flat:
	clang++ -g -O3 case_exp_flat_2.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o a.out 

JIT:
	clang++ -g -O3 case_exp_flat_JIT.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -o a.out 
clean:
	rm a.out
