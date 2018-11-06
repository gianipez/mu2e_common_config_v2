//
//  Persistent representation of a TrkCaloHit, used in the
//  persistent representation of the BTrk Kalman Fit
//  Original author: Dave Brown (LBNL) 31 Aug 2016
//
#ifndef RecoDataProducts_TrkCaloHitSeed_HH
#define RecoDataProducts_TrkCaloHitSeed_HH
#include "RecoDataProducts/inc/HitT0.hh"
#include "RecoDataProducts/inc/HitT0.hh"
#include "RecoDataProducts/inc/CaloCluster.hh"
#include "RecoDataProducts/inc/StrawHitIndex.hh"
#include "DataProducts/inc/StrawId.hh"
#include "canvas/Persistency/Common/Ptr.h"
#include <Rtypes.h>
namespace mu2e {
  struct TrkCaloHitSeed {
    TrkCaloHitSeed() :  _trklen(0), _hitlen(0), _cdoca(0),_rerr(0)  {}
    // construct from the information
  TrkCaloHitSeed(art::Ptr<CaloCluster> const& cluster, HitT0 const& t0, Float_t trklen, Float_t hitlen, Float_t rdrift, Float_t cdoca, Float_t rerr) :
    _cluster(cluster), _t0(t0), _trklen(trklen),
    _hitlen(hitlen), _cdoca(cdoca), _rerr(rerr){}
// accessors
    art::Ptr<CaloCluster> const& caloCluster() const { return _cluster; }
    HitT0 const&  t0() const { return _t0; }
    Float_t	trkLen() const { return _trklen; }
    Float_t	hitLen() const { return _hitlen; }
    Float_t	clusterAxisDOCA() const { return _cdoca; }
    Float_t	transverseErr() const { return _rerr; }
    //
    art::Ptr<CaloCluster> _cluster; // cluster this hit is based on
    HitT0	    _t0;	  // time origin for this hit
    Float_t	    _trklen;	  // Length from the nominal track start to the POCA of this hit
    Float_t	    _hitlen;	  // Length along the cluster axis to the POCA of this hit
    Float_t	    _cdoca;	  // DOCA from the track to the cluster axis, signed by the angular momentum WRT the wire
    Float_t	    _rerr;	  // intrinsic radial error
  };
}
#endif
