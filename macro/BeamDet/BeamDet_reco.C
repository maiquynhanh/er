void BeamDet_reco(Int_t nEvents = 2000){
  //---------------------Files-----------------------------------------------
  TString inFile = "sim.root";
  TString outFile = "reco.root";
  TString parFile = "par.root";
  // ------------------------------------------------------------------------
  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------
  
  // -----   Digitization run   -------------------------------------------
  FairRunAna *fRun= new FairRunAna();
  fRun->SetInputFile(inFile);
  fRun->SetOutputFile(outFile);
  // ------------------------------------------------------------------------
  //-------- Set MC event header --------------------------------------------
  EREventHeader* header = new EREventHeader();
  fRun->SetEventHeader(header);
  //------------------------------------------------------------------------
  // ------------------------BeamDetDigitizer---------------------------------
  Int_t verbose = 1; // 1 - only standard log print, 2 - print digi information 
  ERBeamDetDigitizer* digitizer = new ERBeamDetDigitizer(verbose);
  digitizer->SetMWPCElossThreshold(0.006);

  digitizer->SetToFTimeSigma(1e-10);
  fRun->AddTask(digitizer);
  // ------------------------------------------------------------------------
  // -----------------------BeamDetTrackFinder------------------------------
  ERBeamDetTrackFinder* trackFinder = new ERBeamDetTrackFinder(1);
  fRun->AddTask(trackFinder);
  // ------------------------------------------------------------------------
  // -----------------------BeamDetTrackPID-------------------------------
  ERBeamDetPID* pid = new ERBeamDetPID(1);
  pid->SetPID(1000160280);
  pid->SetIonMass(26.2716160);
  pid->SetBoxPID(203., 206., 0.005, 0.12);
  pid->SetOffsetToF(0.);
  pid->SetProbabilityThreshold(0.5);

  fRun->AddTask(pid);
  // ------------------------------------------------------------------------
  // -----------Runtime DataBase info -------------------------------------
  FairRuntimeDb* rtdb = fRun->GetRuntimeDb();

  FairParRootFileIo*  parInput = new FairParRootFileIo();
  parInput->open(parFile.Data(), "UPDATE");
  
  rtdb->setFirstInput(parInput);
  
  // -----   Intialise and run   --------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  fRun->Init();
  fRun->Run(0, nEvents);
  // ------------------------------------------------------------------------
  rtdb->setOutput(parInput);
  rtdb->saveOutput();
  
  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  cout << endl << endl;
  cout << "Macro finished succesfully." << endl;
  cout << "Output file writen:  "    << outFile << endl;
  cout << "Parameter file writen " << parFile << endl;
  cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
  cout << endl;
  // ------------------------------------------------------------------------

}
