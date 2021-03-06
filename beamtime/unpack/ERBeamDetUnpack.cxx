#include "ERBeamDetUnpack.h"

#include <iostream>

#include "TClonesArray.h"

#include "FairRootManager.h"
#include "FairLogger.h"

#include "DetEventFull.h"
#include "DetEventStation.h"
#include "DetMessage.h"

#include "ERBeamDetTOFDigi.h"
#include "ERBeamDetMWPCDigi.h"

using namespace std;

//--------------------------------------------------------------------------------------------------
ERBeamDetUnpack::ERBeamDetUnpack(TString detName):
 ERUnpack(detName),
 fTimeCalConst(0.125)
{

}
//--------------------------------------------------------------------------------------------------
ERBeamDetUnpack::~ERBeamDetUnpack(){

}
//--------------------------------------------------------------------------------------------------
Bool_t ERBeamDetUnpack::Init(SetupConfiguration* setupConf){
	if (!ERUnpack::Init(setupConf))
		return kTRUE;

	FairRootManager* ioman = FairRootManager::Instance();
  	if ( ! ioman ) Fatal("Init", "No FairRootManager");

	fSetupConfiguration = setupConf;

	//@TODO check setup

	fMwpcAmpTimeStations["MWPC1"] = 0;
	fMwpcAmpTimeStations["MWPC2"] = 1;
	fMwpcAmpTimeStations["MWPC3"] = 2;
	fMwpcAmpTimeStations["MWPC4"] = 3;

	fMwpcBnames["MWPC1"] = "BeamDetMWPCDigiX1";
	fMwpcBnames["MWPC2"] = "BeamDetMWPCDigiY1";
	fMwpcBnames["MWPC3"] = "BeamDetMWPCDigiX2";
	fMwpcBnames["MWPC4"] = "BeamDetMWPCDigiY2";

	const std::map<TString, unsigned short> stList = fSetupConfiguration->GetStationList(fDetName);
	for (auto itSt : stList){
		if (itSt.first == TString("F3")){
			fDigiCollections["BeamDetToFDigi1"] = new TClonesArray("ERBeamDetTOFDigi",1000);
			ioman->Register("BeamDetToFDigi1", "BeamDet", fDigiCollections["BeamDetToFDigi1"], kTRUE);
		}
		if (itSt.first == TString("F5")){
			fDigiCollections["BeamDetToFDigi2"] = new TClonesArray("ERBeamDetTOFDigi",1000);
			ioman->Register("BeamDetToFDigi2", "BeamDet", fDigiCollections["BeamDetToFDigi2"], kTRUE);
		}
		for (auto itMwpcStation : fMwpcAmpTimeStations){
			TString bName = fMwpcBnames[itMwpcStation.first];
			if (itSt.first == itMwpcStation.first){
				fDigiCollections[bName] = new TClonesArray("ERBeamDetMWPCDigi",1000);
				ioman->Register(bName, "BeamDet", fDigiCollections[bName], kTRUE);
			}
		}
	}

	return kTRUE;
}
//--------------------------------------------------------------------------------------------------
Bool_t ERBeamDetUnpack::DoUnpack(Int_t* data, Int_t size){
	if (!ERUnpack::DoUnpack(data,size))
		return kTRUE;

	DetEventFull* event = (DetEventFull*)data;

	DetEventDetector* detEvent = (DetEventDetector* )event->GetChild(fDetName);
	const std::map<TString, unsigned short> stList = fSetupConfiguration->GetStationList(fDetName);
	//ToF
	if (stList.find("F3") != stList.end() && stList.find("tF3") != stList.end()){
		std::map<Int_t, std::pair<Double_t, Double_t> > valueMap;
		UnpackAmpTimeStation(detEvent, "F3","tF3",valueMap);
		if (valueMap.size() == 4){
			Double_t time = 0., amp = 0.;
			for (auto itValue : valueMap){
				amp += itValue.second.first;
				time += itValue.second.second;
			}
			time = time*0.25*fTimeCalConst;
			AddToFDigi(amp,time,1);
		}
		else
			LOG(DEBUG) << "Wrong PMT number in ToF!" << FairLogger::endl;
	}
	if (stList.find("F5") != stList.end() && stList.find("tF5") != stList.end()){
		std::map<Int_t, std::pair<Double_t, Double_t> > valueMap;
		UnpackAmpTimeStation(detEvent, "F5","tF5",valueMap);
		if (valueMap.size() == 4){
			Double_t time = 0., amp = 0.;
			for (auto itValue : valueMap){
				amp += itValue.second.first;
				time += itValue.second.second;
			}
			time = time*0.25*fTimeCalConst;
			AddToFDigi(amp,time,2);
		}
		else
			LOG(DEBUG) << "Wrong PMT number in ToF!" << FairLogger::endl;
	}
	// MWPC
	std::map<Int_t, Double_t> mwpcTime;
	if (stList.find("tMWPC") != stList.end()){
		UnpackStation(detEvent,"tMWPC",mwpcTime);
	}

	for (auto itMwpcStation : fMwpcAmpTimeStations){
		TString mwpcAmpSt = itMwpcStation.first;
		Int_t mwpcTimeSt = itMwpcStation.second;
		if (stList.find(mwpcAmpSt) != stList.end()){
			std::map<Int_t, Double_t> mwpcAmp;
			UnpackStation(detEvent, mwpcAmpSt, mwpcAmp);
			if (mwpcTime.find(mwpcTimeSt) != mwpcTime.end()){
				for (auto itChanel : mwpcAmp){
					AddMWPCDigi(itChanel.second, mwpcTime[mwpcTimeSt]*fTimeCalConst, mwpcAmpSt, itChanel.first);
				}
			}
			else{
				LOG(DEBUG) << "MWPC time signal not found for amplitude" << FairLogger::endl;
			}
		}
	}
	
	return kTRUE;
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetUnpack::AddToFDigi(Float_t edep, Double_t time, Int_t tofNb) {
  ERBeamDetTOFDigi *digi; 
  if(tofNb == 1) {
    digi = new((*fDigiCollections["BeamDetToFDigi1"])[fDigiCollections["BeamDetToFDigi1"]->GetEntriesFast()])
                ERBeamDetTOFDigi(fDigiCollections["BeamDetToFDigi1"]->GetEntriesFast(), edep, time, tofNb);
  }
  if(tofNb == 2) {
    digi = new((*fDigiCollections["BeamDetToFDigi2"])[fDigiCollections["BeamDetToFDigi2"]->GetEntriesFast()])
                ERBeamDetTOFDigi(fDigiCollections["BeamDetToFDigi2"]->GetEntriesFast(), edep, time, tofNb);
  }
}
//--------------------------------------------------------------------------------------------------
void ERBeamDetUnpack::AddMWPCDigi(Float_t edep, Double_t time, 
                                            TString mwpcSt, Int_t wireNb) {
  ERBeamDetMWPCDigi *digi;
  TString bName = fMwpcBnames[mwpcSt];
  Int_t mwpcNb = -1;
  Int_t planeNb = -1;
  if (mwpcSt == "MWPC1"){
  	mwpcNb = 1;
  	planeNb = 1;
  }
  if (mwpcSt == "MWPC2"){
  	mwpcNb = 1;
  	planeNb = 2;
  }
  if (mwpcSt == "MWPC3"){
  	mwpcNb = 2;
  	planeNb = 1;
  }
  if (mwpcSt == "MWPC4"){
  	mwpcNb = 2;
  	planeNb = 2;
  }
  digi = new((*fDigiCollections[bName])[fDigiCollections[bName]->GetEntriesFast()])
              ERBeamDetMWPCDigi(fDigiCollections[bName]->GetEntriesFast(), edep, time, 
                                mwpcNb, planeNb, wireNb+1);
}
//--------------------------------------------------------------------------------------------------
ClassImp(ERBeamDetUnpack)
