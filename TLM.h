#ifndef TLM_H
#define TLM_H

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

/* env variables */
#define TLM_EVENTS "TLM_EVENTS"
#define TLM_OUTPUT_FILE "TLM_OUTPUT_FILE"

/* default env variable values */
#define TLM_DEFAULT_EVENTS "PAPI_L1_TCM, PAPI_L2_TCM, PAPI_L3_TCM"
#define TLM_DEFAULT_OUTPUT "TLM_RESULTS.txt"

/* API calls */
EXTERN_C void TLM_Init();
EXTERN_C void TLM_Start();
EXTERN_C void TLM_Stop();
EXTERN_C void TLM_Finalize();
EXTERN_C void TLM_Test();

#endif
