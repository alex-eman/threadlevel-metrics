username=$(shell whoami)
papi-include=/papi-install/include
papi-lib=/papi-install/lib/libpapi.a

all: test

papi_utils.o: papi_utils.C
	mpic++ -c papi_utils.C -I/home/$(username)$(papi-include)

TLM.o: TLM.C papi_utils.o
	mpic++ -fopenmp -c -std=c++0x TLM.C -I/home/$(username)$(papi-include)

test: test_TLM.C TLM.o 
	mpic++ -fopenmp -o test test_TLM.C -I/home/$(username)$(papi-include) papi_utils.o TLM.o /home/$(username)$(papi-lib) libTLM.a

clean:
	rm *.o


