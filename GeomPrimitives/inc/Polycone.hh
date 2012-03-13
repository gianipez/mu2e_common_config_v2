#ifndef GeomPrimitives_Polycone_hh
#define GeomPrimitives_Polycone_hh

//
// The parameters of a Polycone 
//
// $Id: Polycone.hh,v 1.1 2012/03/13 19:00:18 genser Exp $
// $Author: genser $
// $Date: 2012/03/13 19:00:18 $
//
// Original author KLG
//

#include <cmath>
#include <string>
#include <algorithm>
#include <iterator>
#include <ostream>
#include <vector>

#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Units/PhysicalConstants.h"

namespace mu2e {

  class Polycone{

    //   G4Polycone( const G4String& name, 
    //               G4double phiStart,     // initial phi starting angle
    //               G4double phiTotal,     // total phi angle
    //               G4int numZPlanes,     // number of z planes
    //               const G4double zPlane[],  // position of z planes
    //               const G4double rInner[],  // tangent distance to inner surface
    //               const G4double rOuter[])  // tangent distance to outer surface

  public:

    // we may need to use better containers here
    Polycone( unsigned int numZPlanes,
              double zPlanes[],
              double rInner[],
              double rOuter[],
              CLHEP::Hep3Vector const & originInMu2e,
              std::string const & materialName, 
              double phiStart = 0.,
              double phiTotal = CLHEP::twopi);

    Polycone( double phiStart,
              double phiTotal,
              unsigned int numZPlanes,
              double zPlanes[],
              double rInner[],
              double rOuter[],
              CLHEP::Hep3Vector const & originInMu2e,
              std::string const & materialName);

    // Use compiler-generated copy c'tor, copy assignment, and d'tor.

    double phi0()           const { return _phiStart; }
    double phiTotal()       const { return _phiTotal; }

    std::string const & materialName() const { return _materialName; }

    CLHEP::Hep3Vector const & originInMu2e() const { return _originInMu2e; }

    double const * zPlanes() const { return &_zPlanes[0]; }
    double const * rInner() const { return &_rInner[0]; }
    double const * rOuter() const { return &_rOuter[0]; }

    int const numZPlanes() const { return _numZPlanes; }

  private:
    
    int const _numZPlanes;
  
    std::vector<double> _zPlanes;
    std::vector<double> _rInner;
    std::vector<double> _rOuter;

    CLHEP::Hep3Vector _originInMu2e;
    std::string       _materialName;

    double _phiStart;
    double _phiTotal;

  // do we need/want the rotation here?

};

// TBD
//   inline std::ostream& operator<<(std::ostream& ost,
//                                   const Polycone& tp ){
//     ost << "("
//         << tp.innerRadius() << " "
//         << tp.outerRadius() << " "
//         << tp.zHalfLength() << " "
//         << tp.phi0()        << " "
//         << tp.phiMax()      << ")"
//         << " )";
//     return ost;
//   }

// could use something like
//      std::copy (rOuter[0],rOuter[_numZPlanes],
//                 ostream_iterator<double>(std::cout,", "));



}


#endif /* GeomPrimitives_TubsParams_hh */
