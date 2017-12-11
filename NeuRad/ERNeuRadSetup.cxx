/********************************************************************************
 *              Copyright (C) Joint Institute for Nuclear Research              *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "ERNeuRadSetup.h"

#include "TGeoManager.h"
#include "TGeoBBox.h"
#include "TMath.h"

#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "FairLogger.h"

// Singleton management
ERNeuRadSetup* ERNeuRadSetup::fInstance = NULL;

ERNeuRadSetup::ERNeuRadSetup() :
  fDigiPar(NULL),
  fZ(0.),
  fLength(0.),
  fFiberWidth(0.),
  fRowNofFibers(-1),
  fRowNofModules(-1),
  fRowNofPixels(-1)
{
  this->AnalyseGeoManager();
  LOG(INFO) << "ERNeuRadSetup::ERNeuRadSetup: " << "constructed! "<< FairLogger::endl;
}

// Singleton management
ERNeuRadSetup* ERNeuRadSetup::Instance(void) {
  if (fInstance == NULL) {
    fInstance = new ERNeuRadSetup();
  }
  return fInstance;
}

void ERNeuRadSetup::AnalyseGeoManager(void) {

    // --- Catch absence of TGeoManager
    if (!gGeoManager) {
      LOG(FATAL) << "ERNeuRadSetup::AnalyseGeoManager: cannot initialize without gGeoManager!" << FairLogger::endl;
    }

    // Get the pointer to the cave node as the top node of the geometry manager
    gGeoManager->CdTop();
    TGeoNode* cave = gGeoManager->GetCurrentNode();

    // --------------------------------------------------------------------------------------------------------------

    // Search for NeuRad node
    TGeoNode* neuRad = NULL;
    for (Int_t iNode = 0; iNode < cave->GetNdaughters(); iNode++) {
        TString name = cave->GetDaughter(iNode)->GetName();
        if ( name.Contains("NeuRad", TString::kIgnoreCase) ) {
            neuRad = cave->GetDaughter(iNode);
            LOG(INFO) << "NeuRad node found. name=" << name << FairLogger::endl;
            break;
        }
    }

    // Get Z position of NeuRad
    fZ = neuRad->GetMatrix()->GetTranslation()[2];
    LOG(INFO) << "NeuRad Z position:" << fZ << " cm" << FairLogger::endl;

    // --------------------------------------------------------------------------------------------------------------

    // Search for a module node as the first child of NeuRad node
    TGeoNode* module = neuRad->GetDaughter(0);
    TString moduleNodeName = module->GetName();
    LOG(INFO) << "module node name=" << moduleNodeName << FairLogger::endl;

    UInt_t iModulesCounter = 0;

    // If not found - search one level deeper
    if (! moduleNodeName.Contains("module", TString::kIgnoreCase)) {
      LOG(INFO) << "wrong module node name! Trying again." << FairLogger::endl;
      module = neuRad->GetDaughter(0)->GetDaughter(0);
      moduleNodeName = module->GetName();
      LOG(INFO) << "module node name=" << moduleNodeName << FairLogger::endl;
      if (! moduleNodeName.Contains("module", TString::kIgnoreCase)) {
        LOG(FATAL) << "wrong module node name! Aborting." << FairLogger::endl;
      } else {
        // Count the number of modules
        //TODO unchecked code
        for (UInt_t iDaughterNode=0; iDaughterNode<neuRad->GetDaughter(0)->GetNdaughters(); iDaughterNode++) {
          moduleNodeName = neuRad->GetDaughter(0)->GetDaughter(iDaughterNode)->GetName();
          if (moduleNodeName.Contains("module", TString::kIgnoreCase)) iModulesCounter++;
        }
      }
    } else {
      // Count the number of modules
      for (UInt_t iDaughterNode=0; iDaughterNode<neuRad->GetNdaughters(); iDaughterNode++) {
        moduleNodeName = neuRad->GetDaughter(iDaughterNode)->GetName();
        if (moduleNodeName.Contains("module", TString::kIgnoreCase)) iModulesCounter++;
      }
    }

    LOG(INFO) << "Found " << iModulesCounter << " modules" << FairLogger::endl;

    // Get module length along Z
    TGeoBBox* module_box = (TGeoBBox*)module->GetVolume()->GetShape();
    fLength = module_box->GetDZ()*2;
    LOG(INFO) << "module length (Z): " << fLength << " cm" << FairLogger::endl;

    // --------------------------------------------------------------------------------------------------------------

    // Search for a pixel node as the first child of a module node
    TGeoNode* pixel = module->GetDaughter(0);
    TString pixelNodeName = pixel->GetName();
    LOG(INFO) << "pixel node name=" << pixelNodeName << FairLogger::endl;

    UInt_t iSubmodulesCounter = 0;
    UInt_t iPixelsCounter = 0;

    // If not found - search one level deeper
    if (! pixelNodeName.Contains("pixel", TString::kIgnoreCase)) {
      LOG(INFO) << "wrong pixel node name! Trying again." << FairLogger::endl;
      LOG(INFO) << "probably there are submodules - check!" << FairLogger::endl;
      if (pixelNodeName.Contains("submodul", TString::kIgnoreCase)) {
        LOG(INFO) << "indeed, submodule found. Count how many of them are in the module." << FairLogger::endl;
        TString submoduleNodeName;
        // Count the number of submodules
        for (UInt_t iDaughterNode=0; iDaughterNode<module->GetNdaughters(); iDaughterNode++) {
          submoduleNodeName = module->GetDaughter(iDaughterNode)->GetName();
          if (submoduleNodeName.Contains("submodul", TString::kIgnoreCase)) iSubmodulesCounter++;
        }
        LOG(INFO) << "Found " << iSubmodulesCounter << " submodules in a module" << FairLogger::endl;
      } else {
        //TODO something is wrong, but not completely wrong...
        LOG(DEBUG) << "something is wrong, but not completely wrong..." << FairLogger::endl;
      }

      pixel = module->GetDaughter(0)->GetDaughter(0);
      pixelNodeName = pixel->GetName();
      LOG(INFO) << "pixel node name=" << pixelNodeName << FairLogger::endl;
      if (! pixelNodeName.Contains("pixel", TString::kIgnoreCase)) {
        LOG(FATAL) << "wrong pixel node name! Aborting." << FairLogger::endl;
      } else {
        // Count the number of pixels in one submodule
        for (UInt_t iDaughterNode=0; iDaughterNode<module->GetDaughter(0)->GetNdaughters(); iDaughterNode++) {
          pixelNodeName = module->GetDaughter(0)->GetDaughter(iDaughterNode)->GetName();
          if (pixelNodeName.Contains("pixel", TString::kIgnoreCase)) iPixelsCounter++;
        }
        // Take into account that there are several submodules with a few pixels
        iPixelsCounter *= iSubmodulesCounter;
      }
    } else {
      // Count the number of modules
      //TODO unchecked code
      for (UInt_t iDaughterNode=0; iDaughterNode<module->GetNdaughters(); iDaughterNode++) {
        pixelNodeName = module->GetDaughter(iDaughterNode)->GetName();
        if (pixelNodeName.Contains("pixel", TString::kIgnoreCase)) iPixelsCounter++;
      }
    }

    LOG(INFO) << "ERNeuRadSetup::AnalyseGeoManager: "
              << "Found " << iPixelsCounter << " pixels in " << iSubmodulesCounter << " submodules." << FairLogger::endl;

    // --------------------------------------------------------------------------------------------------------------

    // Search for a fiber as the first child of a pixel
    TGeoNode* fiber = pixel->GetDaughter(0); // fiber with cladding and dead zone - TODO? somewhat wrong comment?
    TString fiberNodeName = fiber->GetName();
    LOG(INFO) << "fiber node name=" << fiberNodeName << FairLogger::endl;

    UInt_t iCladdingsCounter = 0;
    UInt_t iFibersCounter = 0;

    // If not found - search one level deeper
    if (! fiberNodeName.Contains("fiber", TString::kIgnoreCase)) {
      LOG(INFO) << "wrong fiber node name! Trying again." << FairLogger::endl;
      LOG(INFO) << "probably there are claddings - check!" << FairLogger::endl;

      if (fiberNodeName.Contains("cladding", TString::kIgnoreCase)) {
        LOG(INFO) << "indeed, cladding found. Count how many of them are in the pixel." << FairLogger::endl;
        TString claddingNodeName;
        // Count the number of claddings
        for (UInt_t iDaughterNode=0; iDaughterNode<pixel->GetNdaughters(); iDaughterNode++) {
          claddingNodeName = pixel->GetDaughter(iDaughterNode)->GetName();
          if (claddingNodeName.Contains("cladding", TString::kIgnoreCase)) iCladdingsCounter++;
        }
        LOG(INFO) << "Found " << iCladdingsCounter << " claddings in a pixel." << FairLogger::endl;
      } else {
        //TODO something is wrong, but not completely wrong...
        LOG(DEBUG) << "something is wrong, but not completely wrong..." << FairLogger::endl;

      }

      fiber = pixel->GetDaughter(0)->GetDaughter(0);
      fiberNodeName = fiber->GetName();
      LOG(INFO) << "fiber node name=" << fiberNodeName << FairLogger::endl;
      if (! fiberNodeName.Contains("fiber", TString::kIgnoreCase)) {
        LOG(FATAL) << "wrong fiber node name! Aborting." << FairLogger::endl;
      } else {
        // Count the number of fibers in one cladding
        for (UInt_t iDaughterNode=0; iDaughterNode<pixel->GetDaughter(0)->GetNdaughters(); iDaughterNode++) {
          fiberNodeName = pixel->GetDaughter(0)->GetDaughter(iDaughterNode)->GetName();
          if (fiberNodeName.Contains("fiber", TString::kIgnoreCase)) iFibersCounter++;
        }
        // Take into account that there are several fibers with a few pixels
        iFibersCounter *= iCladdingsCounter;
        // Take into account that there are several claddings (with fibers) in a pixel
        iFibersCounter *= iPixelsCounter;
      }
    } else {
      // Count the number of fibers
      //TODO unchecked code
      for (UInt_t iDaughterNode=0; iDaughterNode<pixel->GetNdaughters(); iDaughterNode++) {
        fiberNodeName = pixel->GetDaughter(iDaughterNode)->GetName();
        if (fiberNodeName.Contains("fiber", TString::kIgnoreCase)) iFibersCounter++;
      }
    }

    LOG(INFO) << "Found " << iFibersCounter
              << " fibers in " << iPixelsCounter << " pixels." << FairLogger::endl;

    // Get fiber width along X
    TGeoBBox* fiber_box = (TGeoBBox*)fiber->GetVolume()->GetShape();
    fFiberWidth = fiber_box->GetDX()*2;
    LOG(INFO) << "fiber width (X): " << fFiberWidth << FairLogger::endl;
    
    // --------------------------------------------------------------------------------------------------------------

    fRowNofModules = Int_t(TMath::Sqrt(iModulesCounter));
    fRowNofPixels = Int_t(TMath::Sqrt(iPixelsCounter));
    fRowNofFibers = Int_t(TMath::Sqrt(iFibersCounter));
/*
    LOG(INFO) << "NeuRad modules in row count:" << fRowNofModules << FairLogger::endl;
    LOG(INFO) << "NeuRad pixels in row count:" << fRowNofPixels << FairLogger::endl;
    LOG(INFO) << "NeuRad fibers in row count:" << fRowNofFibers << FairLogger::endl;

    // Обработка субмодулей в новой геометрии
    Int_t iSubm = -1; // Любой subm
    Int_t nSubm = 0;
    for (Int_t iNode = 0; iNode < module->GetNdaughters(); iNode++) {
      if (TString(module->GetDaughter(iNode)->GetName()).Contains("submodul")) {
        iSubm = iNode;
        nSubm++;
      }
    }
    Int_t nPixel_in_subm = 0;
    if (iSubm > -1) {
      LOG(INFO) << "Submodules in geometry!" << FairLogger::endl;
      TGeoNode* subm = module->GetDaughter(iSubm);
      for (Int_t iNode = 0; iNode < subm->GetNdaughters(); iNode++) {
        if (TString(subm->GetDaughter(iNode)->GetName()).Contains("pixel")) {
          pixel = subm->GetDaughter(iNode);
          nPixel_in_subm++;
        }
      }
      fRowNofPixels = Int_t(TMath::Sqrt(nSubm))*Int_t(TMath::Sqrt(nPixel_in_subm));
      fRowNofFibers = Int_t(TMath::Sqrt(pixel->GetNdaughters()));
    }
*/
    LOG(INFO) << "NeuRad modules in row count: " << fRowNofModules << FairLogger::endl;
    LOG(INFO) << "NeuRad pixels in row count: " << fRowNofPixels << FairLogger::endl;
    LOG(INFO) << "NeuRad fibers in row count: " << fRowNofFibers << FairLogger::endl;

}

//TODO what does the return value mean?
Int_t ERNeuRadSetup::SetParContainers(void) {
  // Get run and runtime database
  FairRunAna* run = FairRunAna::Instance();
  if ( ! run ) Fatal("SetParContainers", "No analysis run");

  FairRuntimeDb* rtdb = run->GetRuntimeDb();
  if ( ! rtdb ) Fatal("SetParContainers", "No runtime database");

  fDigiPar = (ERNeuRadDigiPar*) (rtdb->getContainer("ERNeuRadDigiPar"));
  if (fDigiPar) {
    LOG(INFO) << "ERNeuRadSetup::SetParContainers: " << "ERNeuRadDigiPar initialized! "<< FairLogger::endl;
    return 0;
  }
  return 1;
}

Float_t ERNeuRadSetup::GetModuleX(Int_t iPmtId) const {
  return fModules[iPmtId]->fX;
}

Float_t ERNeuRadSetup::GetModuleY(Int_t iPmtId) const {
  return fModules[iPmtId]->fY;
}

Float_t ERNeuRadSetup::GetFiberX(Int_t iPmtId, Int_t iChId) const {
  return fFibers[iPmtId][iChId]->fX;
}

Float_t ERNeuRadSetup::GetFiberY(Int_t iPmtId, Int_t iChId) const {
  return fFibers[iPmtId][iChId]->fY;
}

void ERNeuRadSetup::Print(void) const {
  fDigiPar->print(); //TODO really?! Maybe print something about the setup itself?
}

// ----------------------------------------------------------------------------

Bool_t ERNeuRadSetup::UseCrosstalks(void) const {
  if (!fDigiPar) {
    LOG(FATAL) << "ERNeuRadSetup::UseCrosstalks: fDigiPar is NULL. Aborting." << FairLogger::endl;
  }
  return fDigiPar->UseCrosstalks();
}

Float_t ERNeuRadSetup::GetPixelQuantumEfficiency(Int_t iPmtId, Int_t iChId) const {
  if (!fDigiPar) {
    LOG(FATAL) << "ERNeuRadSetup::GetPixelQuantumEfficiency: fDigiPar is NULL. Aborting." << FairLogger::endl;
  }
  return fDigiPar->GetPixelQuantumEfficiency(iPmtId, iChId);
}

Float_t ERNeuRadSetup::GetPixelGain(Int_t iPmtId, Int_t iChId) const  {
  if (!fDigiPar) {
    LOG(FATAL) << "ERNeuRadSetup::GetPixelGain: fDigiPar is NULL. Aborting." << FairLogger::endl;
  }
  return fDigiPar->GetPixelGain(iPmtId, iChId);
}

Float_t ERNeuRadSetup::GetPixelSigma(Int_t iPmtId, Int_t iChId) const  {
  if (!fDigiPar) {
    LOG(FATAL) << "ERNeuRadSetup::GetPixelSigma: fDigiPar is NULL. Aborting." << FairLogger::endl;
  }
  return fDigiPar->GetPixelSigma(iPmtId, iChId);
}

void ERNeuRadSetup::Crosstalks(Int_t iFiber, TArrayF& crosstalks) const {
  if (!fDigiPar) {
    LOG(FATAL) << "ERNeuRadSetup::Crosstalks: fDigiPar is NULL. Aborting." << FairLogger::endl;
  }
  return fDigiPar->Crosstalks(iFiber, crosstalks);
}

// ----------------------------------------------------------------------------

ClassImp(ERNeuRadSetup)
