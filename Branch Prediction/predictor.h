#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include "utils.h"
#include "tracer.h"



typedef enum {
  PRED_TYPE_NEVERTAKEN    =0, 
  PRED_TYPE_ALWAYSTAKEN   =1,
  PRED_TYPE_LAST_TIME     =2,
  PRED_TYPE_TWOBIT_COUNTER=3,
  PRED_TYPE_TWOLEVEL_PRED =4,
  PRED_TYPE_MAX           =5
}PredType;




/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class PREDICTOR{


 private:
  UINT32  *lastTimeTable;  // for LastTime Predictor

  UINT32  *twoBitCounterTable; // for TwoBitCounter Predictor

  UINT32  GHR; // Global History Register for TwoLevelPred
  UINT32  *PHT;          // pattern history table for TwoLevelPred
  UINT32  historyLength; // history length for TwoLevelPred
  UINT32  numPhtEntries; // entries in pht for TwoLevelPred

 public:

  // The interface to the four functions below CAN NOT be changed
  PREDICTOR(void);
  bool    GetPrediction(UINT32 PC);  
  void    UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir);

  bool    GetPredictionLastTimePred(UINT32 PC);  
  bool    GetPredictionTwoBitCounterPred(UINT32 PC);  
  bool    GetPredictionTwoLevelPred(UINT32 PC);  

  void    UpdateLastTimePred(UINT32 PC, bool resolveDir, bool predDir);
  void    UpdateTwoBitCounterPred(UINT32 PC, bool resolveDir, bool predDir);
  void    UpdateTwoLevelPred(UINT32 PC, bool resolveDir, bool predDir);
};



/***********************************************************/
#endif

