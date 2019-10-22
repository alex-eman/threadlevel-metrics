## Authors:
Alex Emanuelson
alexeemanuelson@gmail.com

Moustafa Naquib
naquibm@wwu.edu
	
Scott Waldron	
waldros2@wwu.edu 

Dylan Carpenter
carpend3@wwu.edu

## Description:
ThreadLevel-Metrics (TLM) is an API that allows the capture of
performance metrics across the process and thread level using PAPI event sets.


## Prerequisites:
User must have the PAPI library installed on their machine.


## Installation
	git clone https://github.com/alex-eman/threadlevel-metrics.git ~/threadlevel-metrics
	
	cd ~/threadlevel-metrics

	make


## Compilation:

    Link libTLM.a to your program during compilation
    Example: mpicc++ -fopenmp -o test test_tlm.C /home/$username/threadlevel-metrics/libTLM.a
    

## API Calls:
To capture performance metrics on your program you must include "TLM.h" to use TLM specific calls.
    
    TLM_Init() - Before any other TLM calls are used the user must call TLM_Init(). 
    This function is called per process and is used to establish the PAPI library as well as all required
    TLM structures. If the user is using MPI, TLM_Init can be called right after MPI_Init(). 
    
    TLM_Start() - Creates PAPI event sets that will start recording metrics on each PAPI event the user has designated.
    
    TLM_Stop() - Stops the PAPI event set. Metrics are then aggregated and freed.
    
    TLM_Finalize() - Metrics are aggregated and written to file.
    
    
## Customization:
TLM allows for flexiblity in what metrics the user can capture.

In the shell execute the following command to see a list of capturable metrics on your machine:

    papi_avail

User specified variables:

    export TLM_EVENTS="PAPI_L1_TCM, PAPI_L1_ICA"
    
    export TLM_OUTPUT_FILE="myPerformanceMetrics.txt"
    
    export OMP_NUM_THREADS=16

Note: To capture multiple PAPI events at one time you must comma-space ", " seperate each event and wrap them in quotations.
    
## Example

	#include "TLM.h"

	int main(int argc, char **argv) {
		MPI_Init(&argc, &argv);
        	TLM_Init();

        	for (int i = 0; i < STEPS; i++) {
            		TLM_Start(); // Start capturing process level metrics
            		int result = dot(a, b);
            		TLM_Stop(); // Stop capturing process metrics
        	}

        	TLM_Finalize(); // Output information to file
        	MPI_Finalize();
    	}
    
    	int dot(int a, int b) {
        	double sum = 0;
        	#pragma omp parallel 
        	{	     
            		TLM_Start(); // Start capturing thread level metrics
            		#pragma omp for reduction (+:sum) 
            		for (size_t i=0; i < a.size(); i++){
				sum += a[i] * b[i];
            		}
            		TLM_Stop(); // Stop capturing thread metrics
        	}	
        	return sum;
    	}

    
## Limitations

    PAPI must be correctly installed on the machine on which you are using ThreadLevel-Metrics.
    
    The user program must use OpenMP if they are using threaded regions.
    
    The event you wish to capture metrics on must be supported on the machine on which you are using ThreadLevel-Metrics. Type papi_avail on the command line for a list of supported and unsupported events.
    
    A TLM_Start() call made at the process level must be matched with a corresponding TLM_Stop() call before entering a threaded region.
    
    In the threaded region, the user must write their #pragma directives based on the following example:
        
        #pragma omp parrallel
        {
        
            TLM_Start();
            
            #prgama omp ...
            
            // Code
            
            TLM_Stop();
        }
## Interpreting Results

    The results file is constructed as follows:
    
        Process #
        
            Event Name
            
                Thread # : Value
                
    If you are using TLM in a with processes and threads, thread 0 will contain an aggregate of the process and thread 0 metrics as the process becomes thread 0 in the threaded region.
    
## License
ThreadLevel-Metrics is released under an LGPL license. For more details see the LICENSE file.

## Acknowledgements
We would like to thank the creator of PerfDump, Todd Gamblin, tgamblin@llnl.gov. We would also like to thank Dr. Tanzima Islam for her guidance and input throughout development.
