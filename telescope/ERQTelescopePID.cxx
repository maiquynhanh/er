/********************************************************************************
 *              Copyright (C) Joint Institute for Nuclear Research              *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "ERQTelescopePID.h"

#include "TVector3.h"
#include "TMath.h"
#include "TGeoNode.h"
#include "TGeoManager.h"

#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4EmCalculator.hh"
#include "G4NistManager.hh"

#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "FairLogger.h"
#include "FairEventHeader.h"

#include "ERBeamDetTrack.h"

#include <iostream>
using namespace std;

//--------------------------------------------------------------------------------------------------
ERQTelescopePID::ERQTelescopePID()
  : FairTask("ER qtelescope particle identification scheme"),
  fEventsForProcessing(NULL),
  fQTelescopeSetup(NULL),
  fUserCut("")
{
}
//--------------------------------------------------------------------------------------------------
ERQTelescopePID::ERQTelescopePID(Int_t verbose)
  : FairTask("ER qtelescope particle identification scheme", verbose),
  fEventsForProcessing(NULL),
  fQTelescopeSetup(NULL),
  fUserCut("")
{
}
//--------------------------------------------------------------------------------------------------
ERQTelescopePID::~ERQTelescopePID() {
}
//--------------------------------------------------------------------------------------------------
InitStatus ERQTelescopePID::Init() {
  FairRootManager* ioman = FairRootManager::Instance();
  if ( ! ioman ) Fatal("Init", "No FairRootManager");

  TList* allbrNames = ioman->GetBranchNameList();
  TIter nextBranch(allbrNames);
  TObjString* bName;

  while (bName = (TObjString*)nextBranch()) {
    TString bFullName = bName->GetString();
    
    if (bFullName.Contains("Digi") && bFullName.Contains("QTelescope")) {
      Int_t bPrefixNameLength = bFullName.First('_'); 
      TString brName(bFullName(bPrefixNameLength + 1, bFullName.Length()));
      fQTelescopeDigi[brName] = (TClonesArray*) ioman->GetObject(bFullName);
    }

    if (bFullName.Contains("Track") && bFullName.Contains("QTelescope")) {
      Int_t bPrefixNameLength = bFullName.First('_'); 
      TString brName(bFullName(bPrefixNameLength + 1, bFullName.Length()));
      fQTelescopeTrack[brName] = (TClonesArray*) ioman->GetObject(bFullName);

      //Creating particles collections for every track collection
      for (auto itPDG : fStationParticles[brName]){
        TString brParticleName;
        brParticleName.Form("%s_%d",brName.Data(),itPDG);
        fQTelescopeParticle[brName][itPDG] = new TClonesArray("ERQTelescopeParticle");
        ioman->Register("ERQTelescopeParticle_" + brParticleName, "QTelescope", 
                    fQTelescopeParticle[brName][itPDG], kTRUE);
      }
    }
  }

  if (fUserCut != ""){
    LOG(INFO) << "User cut " << fUserCut << " implementation" << FairLogger::endl;
    TTree* tree = ioman->GetInTree();
    fEventsForProcessing =  new TH1I ("hist", "Events for processing", tree->GetEntries(), 1, tree->GetEntries());
    tree->Draw("MCEventHeader.GetEventID()>>hist",fUserCut,"goff");
  }

  fQTelescopeSetup = ERQTelescopeSetup::Instance();
  fQTelescopeSetup->ReadGeoParamsFromParContainer();
  return kSUCCESS;
}
//--------------------------------------------------------------------------------------------------
void ERQTelescopePID::Exec(Option_t* opt) { 
  
  Reset();
  
  Int_t mcEvent = FairRun::Instance()->GetEventHeader()->GetMCEntryNumber();
  LOG(INFO) << "Event " << mcEvent <<" ERQTelescopePID: " << FairLogger::endl;
  if (fUserCut != "")
    if (!fEventsForProcessing->GetBinContent(mcEvent)){
      LOG(INFO) << "  Skip event with user cut"<< FairLogger::endl;
      return;
    }

  for (const auto itTrackBranches : fQTelescopeTrack) {

    LOG(DEBUG) << " Work with traks in " << itTrackBranches.first << FairLogger::endl;

    for (Int_t iTrack(0); iTrack < itTrackBranches.second->GetEntriesFast(); iTrack++){

      ERQTelescopeTrack* track = (ERQTelescopeTrack*)itTrackBranches.second->At(iTrack);

      for (const auto itParticesBranches : fQTelescopeParticle[itTrackBranches.first]){

        Int_t pdg = itParticesBranches.first;
        Double_t deadEloss = CalcEloss(itTrackBranches.first,track, pdg);

        //mass 
        G4IonTable* ionTable = G4IonTable::GetIonTable();
        G4ParticleDefinition* ion =  ionTable->GetIon(pdg);
        Float_t mass = ion->GetPDGMass()/1000.; //GeV

        //LotentzVector on telescope
        Double_t T = track->GetSumEdep();
        Double_t P = sqrt(pow(T,2) + 2*mass*T);
        TVector3 direction = track->GetTelescopeVertex()-track->GetTargetVertex();
        TLorentzVector lvTelescope (P*sin(direction.Theta())*cos(direction.Phi()),
                                    P*sin(direction.Theta())*sin(direction.Phi()),
                                    P*cos(direction.Theta()),
                                    sqrt(pow(P,2)+pow(mass,2)));
        //LorentVector on target
        T -= deadEloss;
        P = sqrt(pow(T,2) + 2*mass*T);
        TLorentzVector lvTarget (P*sin(direction.Theta())*cos(direction.Phi()),
                                    P*sin(direction.Theta())*sin(direction.Phi()),
                                    P*cos(direction.Theta()),
                                    sqrt(pow(P,2)+pow(mass,2)));

        AddParticle(lvTelescope, lvTarget, deadEloss,itParticesBranches.second);

      }

    }

  }

}
//--------------------------------------------------------------------------------------------------
void ERQTelescopePID::Reset() {
  for (const auto itTrackBranches : fQTelescopeParticle) {
    for (const auto itParticleBranches : itTrackBranches.second)
      if (itParticleBranches.second) {
        itParticleBranches.second->Delete();
      }
  }
}
//--------------------------------------------------------------------------------------------------
void ERQTelescopePID::Finish() {   
}
//--------------------------------------------------------------------------------------------------
ERQTelescopeParticle* ERQTelescopePID::AddParticle(TLorentzVector lvTelescope, TLorentzVector lvTarget, Double_t deadEloss, TClonesArray* col) 
{
  ERQTelescopeParticle *particle = new((*col)
                                        [col->GetEntriesFast()])
                                        ERQTelescopeParticle(lvTelescope,lvTarget,deadEloss);
  return particle;
}
//------------------------------------------------------------------------------------s--------------
void ERQTelescopePID::SetParContainers() {
  // Get run and runtime database
  FairRun* run = FairRun::Instance();
  if ( ! run ) Fatal("SetParContainers", "No analysis run");

  FairRuntimeDb* rtdb = run->GetRuntimeDb();
  if ( ! rtdb ) Fatal("SetParContainers", "No runtime database");
}
//--------------------------------------------------------------------------------------------------
Double_t ERQTelescopePID::CalcEloss(TString station, ERQTelescopeTrack* track, Int_t pdg){
  
  FairRun* run = FairRun::Instance();
  if (!TString(run->ClassName()).Contains("ERRunAna")){
    LOG(FATAL) << "Use ERRunAna for ERQTelescopePID::CalcEloss!!!" << FairLogger::endl;
    return 0;
  }

  //calclculation ion energy loss in BeamDet volumes
  TVector3 telescopeVertex = track->GetTelescopeVertex();
  TVector3 direction = track->GetTargetVertex() - telescopeVertex;

  G4IonTable* ionTable = G4IonTable::GetIonTable();
  G4ParticleDefinition* ion =  ionTable->GetIon(pdg);
  Float_t mass = ion->GetPDGMass()/1000.; //GeV
  G4EmCalculator* calc = new G4EmCalculator();
  G4NistManager* nist = G4NistManager::Instance();


  LOG(DEBUG) << " [CalcEloss] Dead eloss calculation for station " << station << " for pdg " << pdg 
              << " with mass = " << mass << " with telescope vertex = (" << telescopeVertex.X() << ","
              << telescopeVertex.Y() << "," << telescopeVertex.Z() 
              << " with direction = " << direction.X() << "," << direction.Y() << "," << direction.Z()
              << FairLogger::endl;
  
  TGeoNode* node;
  node = gGeoManager->InitTrack(telescopeVertex.X(),telescopeVertex.Y(),telescopeVertex.Z(),
                                direction.X(),direction.Y(),direction.Z());
  
  Float_t sumLoss = 0.;
  Float_t T = track->GetSumEdep();
  

  Bool_t inTarget = kFALSE;
  Float_t tarEdep = 0.;

  while(!gGeoManager->IsOutside()){
    
    TString matName = node->GetMedium()->GetMaterial()->GetName();
    G4Material* mat = nist->FindOrBuildMaterial(matName.Data());
    
    node = gGeoManager->FindNextBoundary();

    LOG(DEBUG) <<" [CalcEloss]  path  = " <<  gGeoManager->GetPath() << FairLogger::endl;

    if (inTarget && !(TString(gGeoManager->GetPath()).Contains("target")))
      break;

    if (TString(gGeoManager->GetPath()).Contains("Sensitive")){
      LOG(DEBUG) <<" [CalcEloss]    Sensetive Volume -> skip" << FairLogger::endl;
      node = gGeoManager->Step();
      continue;
    }
    
    Double_t range = gGeoManager->GetStep();
    Double_t edep = calc->GetDEDX(T*1e3,ion,mat)*range*10*1e-3;

    node = gGeoManager->GetCurrentNode();
    
    LOG(DEBUG) <<" [CalcEloss]    Kinetic Energy  = " << T << FairLogger::endl;
    LOG(DEBUG) <<" [CalcEloss]    medium " << matName << FairLogger::endl;
    LOG(DEBUG) <<" [CalcEloss]    range  = " << range << FairLogger::endl;
    LOG(DEBUG) <<" [CalcEloss]    edep = " << edep << FairLogger::endl;

    if (TString(gGeoManager->GetPath()).Contains("target"))
      inTarget = kTRUE;

    if (inTarget)
      tarEdep+=edep;

    T += edep;
    sumLoss += edep;
    node = gGeoManager->Step();
  }
  
  T += tarEdep/2.;
  sumLoss -= tarEdep/2.;
  
  LOG(DEBUG) <<" [CalcEloss] Target Eloss = " <<  tarEdep << FairLogger::endl;
  LOG(DEBUG) <<" [CalcEloss] Sum Eloss = " <<  sumLoss << FairLogger::endl;

  return sumLoss;
}
//--------------------------------------------------------------------------------------------------
ClassImp(ERQTelescopePID)