/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#include "utils.h"
#include "tracer.h"
#include "predictor.h"

UINT32 PRED_TYPE=0;   


// usage: predictor <type> <trace>

int main(int argc, char* argv[]){
  
  if (argc != 3) {
    printf("usage: %s <type> <trace>\n", argv[0]);
    exit(-1);
  }
  
  ///////////////////////////////////////////////
  // Init variables
  ///////////////////////////////////////////////
    
    PRED_TYPE  = atoi(argv[1]);
    CBP_TRACER *tracer = new CBP_TRACER(argv[2]);
    PREDICTOR  *brpred = new PREDICTOR();
    CBP_TRACE_RECORD *trace = new CBP_TRACE_RECORD();
    UINT64     numMispred =0;  
    
  ///////////////////////////////////////////////
  // read each trace recod, simulate until done
  ///////////////////////////////////////////////

      while (tracer->GetNextRecord(trace)) {

	if(trace->opType == OPTYPE_BRANCH_COND){

	  bool predDir = brpred->GetPrediction(trace->PC);

	  brpred->UpdatePredictor(trace->PC, trace->branchTaken,predDir);
	  
	  if(predDir != trace->branchTaken){
	    numMispred++; // update mispred stats
	  }
	  
	}
      
      }

    ///////////////////////////////////////////
    //print_stats
    ///////////////////////////////////////////

      printf("\n");
      printf("\nNUM_INSTRUCTIONS     \t : %10llu",   tracer->GetNumInst());
      printf("\nNUM_CONDITIONAL_BR   \t : %10llu",   tracer->GetNumCondBranch());
      printf("\nNUM_MISPREDICTIONS   \t : %10llu",   numMispred);
      printf("\nMISPRED_PER_1K_INST  \t : %10.3f",   1000.0*(double)(numMispred)/(double)(tracer->GetNumInst()));
      printf("\nPERCENTAGE_CORRECT   \t : %10.3f",   100.0-100.0*(double)(numMispred)/(double)(tracer->GetNumCondBranch()));
      printf("\n\n");
}



