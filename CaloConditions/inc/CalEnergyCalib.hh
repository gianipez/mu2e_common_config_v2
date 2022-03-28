#ifndef CaloConditions_CalEnergyCalib_hh
#define CaloConditions_CalEnergyCalib_hh
//
// CalEnergyCalib collects the net response features of crystal
// used in reconstruction 
//

#include <iostream>
#include <vector>
#include <array>
#include "Offline/CalorimeterGeom/inc/Crystal.hh"
#include "Offline/Mu2eInterfaces/inc/ProditionsEntity.hh"


namespace mu2e {

  struct CalEnergyCorr {
    double scale;
    double offset;   
  }
  
  class CalEnergyCalib : virtual public ProditionsEntity {
    public:
      typedef std::shared_ptr<CalEnergyCalib> ptr_t;
      typedef std::shared_ptr<const CalEnergyCalib> cptr_t;
      constexpr static const char* cxname = {"CalEnergyCalib"};
      
      virtual ~CalEnergyCalib() {}

      //TODO here there will be accessors and functions
      double value(){ return _value; }

      // TODO Function will run calibration routines
      const CalEnergyCorr&  calibrateEnergy(CalOfflineId& id) const;   
      double getPed(CalOfflineId& id);
      
      void print(std::ostream& os) const;
      void printVector(std::ostream& os, std::string const& name, 
		      std::vector<double> const& a) const;

      template<typename T, size_t SIZE>
        void printArray(std::ostream& os, std::string const& name,
          std::array<T,SIZE> const& a) const;
          
  private:

      double _value;
      
  };
}
#endif

