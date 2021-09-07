#:%s/^[ ]\+/\t/g
all:
	clang -o3  -emit-llvm hello.c -c -o hello.bc
	clang -o3  -emit-llvm hello.c -S -o hello.ll

dis:
	llvm-dis hello.bc

convert-bc:
	llc hello.bc -o hello.s

o:
	gcc -no-pie hello.s -o hello.native

clean:
	if [ -e hello.bc ]; then rm -r hello.bc; fi
	if [ -e hello.s ]; then rm -r hello.s; fi
	if [ -e hello.native ]; then rm -r hello.native; fi
	if [ -e hello.ll ]; then rm -r hello.ll; fi
