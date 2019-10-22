/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018, Western Washington University
// Produced at Western Washington University.
//
// This file is part of ThreadLevelMidnight.
//
// Written by:
//	Alex Emanuelson, emanuea2@wwu.edu
// 	Moustafa Naquib, naquibm@wwu.edu
// 	Scott Waldron, waldros2@wwu.edu
// 	Dylan Carpenter, carpend3@wwu.edu
//
// All rights reserved.
//
// For details, see https://gitlab.cs.wwu.edu/ParallelTeam/ThreadLevelMidnight
// Please also see the LICENSE file for our notice and the LGPL.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "mpi.h"
#include "TLM.h"
#include "papi_utils.h"

#define ROOT 0
#define PROCESS 30	/* Buffer size for Process: # line */
#define EVENT 30	/* Buffer size for {event}: */
#define THREAD 21	/* Buffer size for {Thread#}: # line */

using namespace std;

/* Process Global Data Structures */
static vector<string> event_names;
static vector<PAPIEventSet*> set_list;
static unordered_map<string, vector<long long>> metrics;

/* Process Global Variables */
static int nprocs;
static int num_threads;
const char *output_file_name;

/* Get configured event names from env or take default */
vector<string> get_event_names(const string& env_var, const char *default_events){

	const char *event_name_list = getenv(env_var.c_str());

	/*If no events are exported, take default events */
	if(!event_name_list){
		if(!default_events){
			return {};
		}
		event_name_list = default_events;
	}

	return split(event_name_list, ", ");

}

/* Get configured outut file or take default */
const char* get_output_file(const string& env_var, const char *default_output){

	const char *output_file_name = getenv(env_var.c_str());

	if(!output_file_name){
		if(!default_output){
			return {};
		}
		output_file_name = default_output;
	}

	return output_file_name;

}

/* Establish PAPI library and data structures */
/* NOTE: Called per process */
/* NOTE: Terminates if PAPI_thread_init failes */
EXTERN_C void TLM_Init(){

	/* MPI Variables */
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	/* Initialize PAPI Library */
	PAPI_library_init(PAPI_VER_CURRENT);
		
	/* Extract number of threads in use */
	#ifdef _OPENMP
		int papi_error = PAPI_thread_init(pthread_self);

		if(papi_error != PAPI_OK){
			cout << "Thread Init failed ..." << endl;
			cout << "Exiting" << endl;
			exit(1);
		}else{
			num_threads = omp_get_max_threads();
		}
	#else
		num_threads = 1;
	#endif

	/* Initialize data structures */
	event_names = get_event_names(TLM_EVENTS, TLM_DEFAULT_EVENTS);
	set_list.resize(num_threads);

	for(string event : event_names){
		vector<long long> event_values;
		event_values.resize(num_threads, 0);
		metrics[event] = event_values;
	}

	if(rank == ROOT){
		cout << "======= ThreadLevelMidnight Module Started ======" << endl;
		cout << "         Initialized PAPI with " << event_names.size() << " event(s):" << endl;
		for(string event : event_names){
			cout << "         " << event << endl;
		}
		cout << "=================================================" << endl;
	}

}

/* Create PAPIEvenSets for each desired event, start */
EXTERN_C void TLM_Start(){
	
	int tid;

	#ifdef _OPENMP
		tid = omp_get_thread_num();
	#else
		tid = 0;
	#endif

	/* Create PAPIEventSet */
	PAPIEventSet *set = new PAPIEventSet();
	set->add_from_environment(TLM_EVENTS, TLM_DEFAULT_EVENTS);
	
	/* Insert into data structure and start */
	set_list.at(tid) = set;
	set_list.at(tid)->start();

}

/* Stop collection of specified event, aggregate */
EXTERN_C void TLM_Stop(){

	int tid;

	#ifdef _OPENMP
		tid = omp_get_thread_num();
	#else
		tid = 0;
	#endif

	/* Stop PAPIEventSet */
	set_list.at(tid)->stop();

	/* Collect metrics */
	for(int i = 0; i < event_names.size(); i++){
		metrics[event_names.at(i)].at(tid) += (set_list.at(tid)->values).at(i); 
	}

	/* Free PAPIEventSet */
	delete set_list.at(tid);

}

/* Build output, write to file, clean up */
EXTERN_C void TLM_Finalize(){
	
	int rank, num_chars;
	char *buff;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* Determine padding for Process # line */
	string nprocsToStr = to_string(nprocs);
	int max_padding = nprocsToStr.length();
	string rankToStr = to_string(rank);
	int rank_padding = max_padding - rankToStr.length();

	/* Dynamically allocate buffer for output */
	int buffsize = (PROCESS + (EVENT + (THREAD * num_threads) * event_names.size()));
	buff = (char *)malloc(buffsize);

	/* Build buffer for output */
	num_chars = sprintf(buff, "Process: %*s%d\n", rank_padding, "",  rank);
	for(int i = 0; i < event_names.size(); i++){

		const char *curr = (event_names.at(i)).c_str();	
		num_chars += sprintf(&buff[num_chars], "\t%s:\n", curr);		

		for(int j = 0; j < num_threads; j++){

			long long value = metrics[event_names.at(i)].at(j);
			num_chars += sprintf(&buff[num_chars], "\t\t%8d: %8lld\n", j, value);

		}

	}
 	
	/* MPI-IO Portion */
	MPI_Offset	offset;
	MPI_File fh;
	MPI_Status status;

	offset = rank * num_chars;
	output_file_name = get_output_file(TLM_OUTPUT_FILE, TLM_DEFAULT_OUTPUT);

	/* Write to output file */
	MPI_File_open(MPI_COMM_WORLD, output_file_name, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	MPI_File_write_at(fh, offset, buff, num_chars, MPI_CHAR, &status);
	MPI_File_close(&fh);

}

/* Wrapper functions for binding */
EXTERN_C void TLM_INIT(){
	TLM_Init();
}

EXTERN_C void tlm_init_(){
	TLM_Init();
}

EXTERN_C void tlm_init__(){
	TLM_Init();
}

EXTERN_C void TLM_START(){
	TLM_Start();
}

EXTERN_C void tlm_start_(){
	TLM_Start();
}

EXTERN_C void tlm_start__(){
	TLM_Start();
}

EXTERN_C void TLM_STOP(){
	TLM_Stop();
}

EXTERN_C void tlm_stop_(){
	TLM_Stop();
}

EXTERN_C void tlm_stop__(){
	TLM_Stop();
}

EXTERN_C void TLM_FINALIZE(){
	TLM_Finalize();
}

EXTERN_C void tlm_finalize_(){
	TLM_Finalize();
}

EXTERN_C void tlm_finalize__(){
	TLM_Finalize();
}
