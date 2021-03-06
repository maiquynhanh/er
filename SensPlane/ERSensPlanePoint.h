/********************************************************************************
 *              Copyright (C) Joint Institute for Nuclear Research              *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

// ------------------------------------------------------------------------------
// -----           ERSensPlanePoint header file                             -----
// ------------------------------------------------------------------------------

#ifndef ERSENSPLANEPOINT_H
#define ERSENSPLANEPOINT_H

#include "FairMCPoint.h"

#include "TVector3.h"

class ERSensPlanePoint : public FairMCPoint
{
public:

  /** Default constructor **/
  ERSensPlanePoint();

  /** Standard constructor **/
  ERSensPlanePoint(Int_t eventID,
                   Int_t trackID,
                   Int_t mot0trackID,
                   Double_t mass,
                   TVector3 posIn,
                   TVector3 posOut,
                   TVector3 momIn,
                   TVector3 momOut,
                   Double_t tof,
                   Double_t length,
                   Double_t eLoss);

  /** Copy constructor **/
  ERSensPlanePoint(const ERSensPlanePoint&);

  /** Destructor **/
  virtual ~ERSensPlanePoint();

  ERSensPlanePoint& operator=(const ERSensPlanePoint&) { return *this; }

  /** Accessors **/
  Int_t GetEventID()            const { return fEventID; }
  Int_t GetMot0TrackID()        const { return fMot0TrackID; }
  Double_t GetMass()            const { return fMass; }
  Double_t GetXIn()             const { return fX; }
  Double_t GetYIn()             const { return fY; }
  Double_t GetZIn()             const { return fZ; }
  Double_t GetXOut()            const { return fX_out; }
  Double_t GetYOut()            const { return fY_out; }
  Double_t GetZOut()            const { return fZ_out; }
  Double_t GetPxOut()           const { return fPx_out; }
  Double_t GetPyOut()           const { return fPy_out; }
  Double_t GetPzOut()           const { return fPz_out; }

  void PositionIn(TVector3& pos)  const { pos.SetXYZ(fX, fY, fZ); }
  void PositionOut(TVector3& pos) const { pos.SetXYZ(fX_out, fY_out, fZ_out); }
  void MomentumOut(TVector3& mom) const { mom.SetXYZ(fPx_out, fPy_out, fPz_out); }

  /** Point coordinates at given z from linear extrapolation **/
  Double_t GetX(Double_t z) const;
  Double_t GetY(Double_t z) const;

  /** Check for distance between in and out **/
  Bool_t IsUsable() const;

  /** Output to screen **/
  virtual void Print(const Option_t* opt = 0) const;

protected:

  Int_t fEventID;
  Int_t fMot0TrackID;
  Double_t fMass;
  Double32_t fX_out,  fY_out,  fZ_out;
  Double32_t fPx_out, fPy_out, fPz_out;

  ClassDef(ERSensPlanePoint, 1)
};

#endif // ERSENSPLANEPOINT_H
