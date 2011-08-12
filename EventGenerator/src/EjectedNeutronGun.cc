//
//
// Simulate the neutrons that come from the stopping target when muons capture
// on an Al nucleus.  Use the MECO distribution for the kinetic energy of the
// neutrons.
//
// $Id: EjectedNeutronGun.cc,v 1.16 2011/08/12 22:15:18 onoratog Exp $
// $Author: onoratog $
// $Date: 2011/08/12 22:15:18 $
//
// Original author Rob Kutschke (proton gun), adapted to neutron by G. Onorato
//
//

// C++ includes.
#include <iostream>

// Framework includes
#include "art/Framework/Core/Run.h"
#include "art/Framework/Core/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/DAQParams.hh"
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "EventGenerator/inc/EjectedNeutronGun.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "Mu2eUtilities/inc/ConfigFileLookupPolicy.hh"
#include "MCDataProducts/inc/PDGCode.hh"
#include "Mu2eUtilities/inc/SimpleConfig.hh"
#include "Mu2eUtilities/inc/safeSqrt.hh"
#include "TargetGeom/inc/Target.hh"

// Other external includes.
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Units/PhysicalConstants.h"

//ROOT Includes
#include "TH1D.h"
#include "TMath.h"

using namespace std;

static const double spectrumEndPoint = 0.1;

namespace mu2e {

  EjectedNeutronGun::EjectedNeutronGun( art::Run& run, const SimpleConfig& config ):

    // Base class.
    GeneratorBase(),

    // Configurable parameters
    _mean(config.getDouble("ejectedNeutronGun.mean",1.25)),
    _elow(config.getDouble("ejectedNeutronGun.elow",0.)),
    _ehi(config.getDouble("ejectedNeutronGun.ehi",spectrumEndPoint)),
    _czmin(config.getDouble("ejectedNeutronGun.czmin",  -1.)),
    _czmax(config.getDouble("ejectedNeutronGun.czmax",  1.)),
    _phimin(config.getDouble("ejectedNeutronGun.phimin", 0. )),
    _phimax(config.getDouble("ejectedNeutronGun.phimax", CLHEP::twopi )),
    _PStoDSDelay(config.getBool("ejectedNeutronGun.PStoDSDelay", false)),
    _pPulseDelay(config.getBool("ejectedNeutronGun.pPulseDelay", false)),
    _nbins(config.getInt("ejectedNeutronGun.nbins",200)),
    _doHistograms(config.getBool("ejectedNeutronGun.doHistograms",true)),

    _spectrumModel(checkSpectrumModel(config.getInt("ejectedNeutronGun.spectrumNumber",0))),
    _filetoread (config.getString("ejectedNeutronGun.spectrumFile","ConditionsService/data/neutronSpectrum.txt")),							 
    // Initialize random number distributions; getEngine comes from the base class.
    _randPoissonQ( getEngine(), std::abs(_mean) ),
    _randomUnitSphere ( getEngine(), _czmin, _czmax, _phimin, _phimax ),    
    _shape ( getEngine() , &(binnedEnergySpectrum()[0]), _nbins ),  
    _nToSkip (config.getInt("ejectedNeutronGun.nToSkip",0)),
    
     // Histogram pointers
    _hMultiplicity(),
    _hKE(),
    _hKEZoom(),
    _hMomentumMeV(),
    _hzPosition(),
    _hcz(),
    _hphi(),
    _htime(),
    _hmudelay(),
    _hpulsedelay()  {

   
    // About the ConditionsService:
    // The argument to the constructor is ignored for now.  It will be a
    // data base key.  There is a second argument that I have let take its
    // default value of "current"; it will be used to specify a version number.
    ConditionsHandle<AcceleratorParams> accPar("ignored");
    ConditionsHandle<DAQParams>         daqPar("ignored");
    ConditionsHandle<ParticleDataTable> pdt("ignored");

    //Set particle mass
    const HepPDT::ParticleData& p_data = pdt->particle(PDGCode::n0).ref();
    _mass = p_data.mass().value();


    // Default values for the start and end of the live window.
    // Can be overriden by the run-time config; see below.
    _tmin = daqPar->t0;
    _tmax = accPar->deBuncherPeriod;

    _tmin = config.getDouble("ejectedNeutronGun.tmin",  _tmin );
    _tmax = config.getDouble("ejectedNeutronGun.tmax",  _tmax );


    // Book histograms.
    if ( _doHistograms ){
      art::ServiceHandle<art::TFileService> tfs;
      art::TFileDirectory tfdir  = tfs->mkdir( "EjectedNeutronGun" );
      _hMultiplicity = tfdir.make<TH1D>( "hMultiplicity", "Neutron Multiplicity",                20,     0,     20  );
      _hKE           = tfdir.make<TH1D>( "hKE",           "Neutron Kinetic Energy",              100, _elow,   _ehi  );
      _hMomentumMeV  = tfdir.make<TH1D>( "hMomentumMeV",  "Neutron Momentum in MeV",             50, _elow,   _ehi  );
      _hKEZoom       = tfdir.make<TH1D>( "hEZoom",        "Neutron Kinetic Energy (zoom)",      200, _elow,   _ehi  );
      _hzPosition    = tfdir.make<TH1D>( "hzPosition",    "Neutron z Position (Tracker Coord)", 200, -6600., -5600. );
      _hcz           = tfdir.make<TH1D>( "hcz",           "Neutron cos(theta)",                 100,    -1.,     1. );
      _hphi          = tfdir.make<TH1D>( "hphi",          "Neutron azimuth",                    100,  -M_PI,  M_PI  );
      _htime         = tfdir.make<TH1D>( "htime",         "Neutron time ",                      210,   -200.,  3000. );
      _hmudelay      = tfdir.make<TH1D>( "hmudelay",      "Production delay due to muons arriving at ST;(ns)", 300, 0., 2000. );
      _hpulsedelay   = tfdir.make<TH1D>( "hpdelay",       "Production delay due to the proton pulse;(ns)", 60, 0., 300. );
    }

    _fGenerator = auto_ptr<FoilParticleGenerator>(new FoilParticleGenerator( getEngine(), _tmin, _tmax,
									     FoilParticleGenerator::muonFileInputFoil,
                                                                             FoilParticleGenerator::muonFileInputPos,
                                                                             FoilParticleGenerator::negExp,          
									     _PStoDSDelay,
                                                                             _pPulseDelay,
									     _nToSkip));
  }

  EjectedNeutronGun::~EjectedNeutronGun(){
  }
  
  // Check bounds of spectrum number and convert to enum
  EjectedNeutronGun::SpectrumType EjectedNeutronGun::checkSpectrumModel(const int i) {
  
    if (i >= last_enum || i < 0) {
      throw cet::exception("RANGE")
       << "Invalid spectrum model given" ;
    }
  
    return SpectrumType(i);
  
  }

  void EjectedNeutronGun::generate( GenParticleCollection& genParts ){

    // Choose the number of neutrons to generate this event.
    
    long n = _mean < 0 ? static_cast<long>(-_mean):_randPoissonQ.fire();
    
    if ( _doHistograms ) {
      _hMultiplicity->Fill(n);
    }

    //Loop over particles to generate

    for (int i=0; i<n; ++i) {

      //Pick up position and momentum
      CLHEP::Hep3Vector pos(0,0,0);
      double time;
      _fGenerator->generatePositionAndTime(pos, time);

      //Pick up energy
      double eKine = _elow + _shape.fire() * ( _ehi - _elow ); 
      double e   = eKine + _mass;
      
   
      //Pick up momentum vector
      _p = safeSqrt(e*e - _mass*_mass);
      CLHEP::Hep3Vector p3 = _randomUnitSphere.fire(_p);

      //Set Four-momentum
      CLHEP::HepLorentzVector mom(0,0,0,0);
      mom.setPx( p3.x() );
      mom.setPy( p3.y() );
      mom.setPz( p3.z() );
      mom.setE( e );

      // Add the particle to  the list.
      genParts.push_back( GenParticle(PDGCode::n0, GenId::ejectedNeutronGun, pos, mom, time));

      // Fill histograms.
      if ( _doHistograms) {
        _hKE->Fill( eKine );
        _hKEZoom->Fill( eKine );
        _hMomentumMeV->Fill( _p );
        _hzPosition->Fill( pos.z() );
        _hcz->Fill( mom.cosTheta() );
        _hphi->Fill( mom.phi() );
        _htime->Fill( time );
	_hmudelay   ->Fill(_fGenerator->muDelay());
	_hpulsedelay->Fill(_fGenerator->pulseDelay());
      }
    } // end of loop on particles

  } // end generate


  // Continuous function of new energy spectrum from doc-db 1619
  double EjectedNeutronGun::energySpectrum(const double x) {
    // Parameters from doc-db 1619
    
    if (x < 10.0) {
      static const double A  = 0.0583;
      static const double B =   3.8278;
      static const double b1 =   2.1068;
      static const double b2   =   1.114;
      static const double c1s    =   pow(6.1167,2);
      static const double c2s    =  pow(2.4447,2);

      return A*(exp(-pow((x-b1),2)/c1s) + B*exp(-pow((x-b2),2)/c2s));
    }
    
    else {
      static const double Ah  = 0.151;
      static const double Bh =   0.0795;
      static const double c1h    =   3.8292;
      static const double c2h    =  20.0653;
    
      return Ah*(exp(-x/c1h) + Bh*exp(-x/c2h));
    }
    
  } // EjectedNeutronGun::energySpectrum
  
  
  // Compute a binned representation of the energy spectrum of the neutron.
  // Energy in MeV
  std::vector<double> EjectedNeutronGun::binnedEnergySpectrum(){


    // Vector to hold the binned representation of the energy spectrum.
    vector<double> neutronSpectrum;
    // Sanity check.
    if (_nbins <= 0) {
      throw cet::exception("RANGE")
        << "Nonsense EjectedNeutronGun.nbins requested="
        << _nbins
        << "\n";
    }
    
    if (_spectrumModel == docdb1619)
    {
      // Bin width.
      double dE = (_ehi - _elow) / _nbins;
      neutronSpectrum.reserve(_nbins);
  
      for (int ib=0; ib<_nbins; ib++) {
        double x = (_elow+(ib+0.5) * dE)*1000.0; //Function takes energy in MeV
        neutronSpectrum.push_back(energySpectrum(x));
      }
  
      return neutronSpectrum; 
    
    }
    else
    {
      ConfigFileLookupPolicy spectrumFileName;
      string NeutronFileFIP =
	spectrumFileName(_filetoread);
      fstream infile(NeutronFileFIP.c_str(), ios::in);
      double supposedBinning = (_ehi - _elow) / _nbins;
      if (infile.is_open()) {
        double en, val;
	bool read_on = true ;
	while (read_on) {
          if (infile.eof()) {
	    en += supposedBinning;
	    val = 0;
	  } else {
	    infile >> en >> val;
	  }
          if (en >= _elow && en <= _ehi) {
            neutronSpectrum.push_back(val);
          }
	  if (en > _ehi) read_on = false;
        }
      }
      else {
	throw cet::exception("RANGE")
        << "No file associated for the ejected neutron spectrum" << endl;
      }

      return neutronSpectrum;
  
    }
  } // EjectedNeutronGun::binnedEnergySpectrum
  
   

} // namespace mu2e
