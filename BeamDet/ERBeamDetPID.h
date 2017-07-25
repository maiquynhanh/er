// -------------------------------------------------------------------------
// -----                        ERBeamDetPID header file          -----
// -----                        Created   by                 -----
// -------------------------------------------------------------------------

#ifndef ERBeamDetPID_H
#define ERBeamDetPID_H

#include "TClonesArray.h"

#include "FairTask.h"

#include "ERBeamDetTOFDigi.h"
#include "ERBeamDetTrack.h"
#include "ERBeamDetParticle.h"
#include "ERBeamDetSetup.h"

class ERBeamDetPID : public FairTask {

public:
  /** Default constructor **/
  ERBeamDetPID();

  /** Constructor 
  ** verbose: 1 - only standard log print
  **/
  ERBeamDetPID(Int_t verbose);

  /** Destructor **/
  ~ERBeamDetPID();

  /** Virtual method Init **/
  virtual InitStatus Init();

  /** Virtual method Exec **/
  virtual void Exec(Option_t* opt);

  /** Virtual method Finish **/
  virtual void Finish();

  /** Virtual method Reset **/
  virtual void Reset();
  
  /** Modifiers **/
  void SetPID(Int_t pdg) {fPDG = pdg;}
  void SetBoxPID(Double_t tof1, Double_t tof2, Double_t dE1, Double_t dE2)
  {
    fTOF1 = tof1;
    fTOF2 = tof2;
    fdE1  = dE1;
    fdE2  = fdE2;
  }
  void SetOffsetTOF(Double_t offsetTOF){fOffsetTOF = offsetTOF;}
  /** Accessors **/ 
protected:
  //Paramaters
  ERBeamDetSetup *fBeamDetSetup;
  
  //Input arrays
  TClonesArray   *fBeamDetTOFDigi;
  Int_t          fPDG;
  Double_t       fTOF1, fTOF2;
  Double_t       fdE1, fdE2;
  Double_t       fOffsetTOF;

  //Output arrays
  ERBeamDetParticle *fBeamDetTrack ;

private:
  virtual void SetParContainers();
  ClassDef(ERBeamDetPID,1)
};
#endif