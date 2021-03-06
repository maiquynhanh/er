
#include "ERRootSource.h"

#include "FairRootManager.h"
#include "FairRun.h"

#include "ERHe8EventHeader.h"

#include <iostream>
using namespace std;

ERRootSource::ERRootSource():
fFile(NULL),
fTree(NULL),
fTreeName(""),
fBranchName(""),
fCurFile(0),
fOldEvents(0)
{
}

ERRootSource::ERRootSource(const ERRootSource& source){
}

ERRootSource::~ERRootSource(){

}

Bool_t ERRootSource::Init(){
	//input files opening
	if (fPath.size() == 0)
		Fatal("ERRootSource", "No files for source ERRootSource");
	if (fRawEvents.size() == 0)
		Fatal("ERRootSource", "ERRootSource without regiistered events");

	OpenNextFile();

	FairRun* run = FairRun::Instance();
	ERHe8EventHeader* header = (ERHe8EventHeader*)run->GetEventHeader();
	header->Register(fTree, fBranchName);

	for (Int_t iREvent = 0; iREvent < fRawEvents.size(); iREvent++)
		fRawEvents[iREvent]->Register(fTree, fBranchName);
	return kTRUE;
}

Int_t ERRootSource::ReadEvent(UInt_t id){
	FairRootManager* ioman = FairRootManager::Instance();
  	if ( ! ioman ) Fatal("Init", "No FairRootManager");

	//Проверяем есть ли еще события для обработки
	if (fTree->GetEntriesFast() == ioman->GetEntryNr()-fOldEvents){
		fOldEvents += ioman->GetEntryNr();
		if (OpenNextFile())
			return 1;
	}
	//cout << "ev" << ioman->GetEntryNr() << endl;
	fTree->GetEntry(ioman->GetEntryNr()-fOldEvents);

	for (Int_t iREvent = 0; iREvent < fRawEvents.size(); iREvent++)
		fRawEvents[iREvent]->Process();
	return 0;
}

void ERRootSource::Close(){
	if (fFile){
		fFile->Close();
		delete fFile;
	}
}

void ERRootSource::Reset(){
}

void ERRootSource::SetFile(TString path, TString treeName, TString branchName){
	fPath.push_back(path);
	fTreeName = treeName;
	fBranchName = branchName;
	cout << "Input file " << path << " with tree name " << fTreeName <<" and branch name " << 
		fBranchName << " added to source ERRootSource" << endl;
}


Int_t ERRootSource::OpenNextFile(){
	if (fCurFile == fPath.size())
		return 1;
	fFile = new TFile(fPath[fCurFile++]);
	if (!fFile->IsOpen())
		Fatal("ERRootSource", "Can`t open file for source ERRootSource");
	else
		cout << fPath[fCurFile-1] << " opened for source ERRootSource" << endl;
	fTree = (TTree*)fFile->Get(fTreeName);
	if (!fTree)
		Fatal("ERRootSource", "Can`t find tree in input file for source ERRootSource");
	return 0;
}