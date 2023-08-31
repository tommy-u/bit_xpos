SRC=bit_xpos.c
EXEC=bit_xpos
CFLAGS=-g -O3 -march=native
LIBS=-lcrypto -fopenmp

NUMA_CTL=numactl --cpunodebind=0 --membind=0

PERF_CACHE_FLAGS=-e L1-dcache-load-misses,L1-dcache-loads,L1-dcache-stores,L1-icache-load-misses,LLC-load-misses,LLC-loads,LLC-store-misses,LLC-stores

$(EXEC): $(SRC)
	gcc $(CFLAGS) $(LIBS) -o $@ $<
# gcc -march=native -O3 -o $@ $<
# gcc -march=native -Ofast -o $@ $<
# gcc -g -O3 -o $@ $< -lcrypto
# gcc -O3 -o $@ $< -lcrypto
# gcc -g -O3 -o $@ $< -lcrypto

run_perf: $(EXEC)
	perf stat $(PERF_CACHE_FLAGS) $(NUMA_CTL) ./$(EXEC) 0 
	perf stat $(PERF_CACHE_FLAGS) $(NUMA_CTL) ./$(EXEC) 1 

cache_grind: $(EXEC)
	valgrind --tool=cachegrind ./$(EXEC) 0

run: run0 run1

run0: $(EXEC)
	$(NUMA_CTL) ./$(EXEC) 0

run1: $(EXEC)
	$(NUMA_CTL) ./$(EXEC) 1

clean:
	rm -rf $(EXEC)

