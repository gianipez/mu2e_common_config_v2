#ifndef CaloConditions_CalEnergyCalibCache_hh
#define CaloConditions_CalEnergyCalibCache_hh


// This Proditions entitiy cache is for the combined calorimeter calibration output
// author: S. Middleton 2022

#include "Offline/Mu2eInterfaces/inc/ProditionsCache.hh"
#include "Offline/CaloConditions/inc/CalEnergyCalibMaker.hh"
#include "Offline/ProditionsService/inc/ProditionsHandle.hh"


namespace mu2e {
  class CalEnergyCalibCache : public ProditionsCache {
  public: 
    CalEnergyCalibCache(CalEnergyCalibConfig const& config):
      ProditionsCache(CalEnergyCalib::cxname,config.verbose()),
      _useDb(config.useDb()),_maker(config) {}

    void initialize() {
     if(_useDb) {
        _calenergycalib_p = std::make_unique<DbHandle<CalCalibConstant>>();
      }
    }
    
    set_t makeSet(art::EventID const& eid) {
      ProditionsEntity::set_t cids;
      if(_useDb) {  
        auto cal = _calenergycalib_p->get(eid);
        cids.insert(_calenergycalib_p->cid());
      }
      return cids;
    }
    
    DbIoV makeIov(art::EventID const& eid) {
      DbIoV iov;
      iov.setMax(); // start with full IOV range
      if(_useDb) { 
         _calenergycalib_p->get(eid);
        iov.overlap(_calenergycalib_p->iov());
      }
      return iov;
    }
    
    ProditionsEntity::ptr makeEntity(art::EventID const& eid) {
       if(_useDb) {
        return _maker.fromDb( _calenergycalib_p->getPtr(eid)); 
      } else {
	      return _maker.fromFcl();
      }
    }

  private:
    bool _useDb;
    CalEnergyCalibMaker _maker;
    std::unique_ptr<DbHandle<CalCalibConstant>> _calenergycalib_p;
  };
};

#endif
