//////////////////////////////////////////////////////////////////////////////
// framework
//
// parameter defaults: CalPatRec/fcl/prolog.fcl
//////////////////////////////////////////////////////////////////////////////
#include "fhiclcpp/types/Atom.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "art/Framework/Core/EDProducer.h"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "art_root_io/TFileService.h"
// conditions
#include "Offline/ConditionsService/inc/ConditionsHandle.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"
#include "Offline/CalorimeterGeom/inc/DiskCalorimeter.hh"
// root
#include "TVector2.h"
// data
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitPosition.hh"
#include "Offline/RecoDataProducts/inc/StereoHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitFlag.hh"
#include "Offline/RecoDataProducts/inc/CaloCluster.hh"
#include "Offline/RecoDataProducts/inc/TimeCluster.hh"

// diagnostics

#include "Offline/CalPatRec/inc/DeltaFinder_types.hh"

// #include "CalPatRec/inc/LsqSums2.hh"
#include "Offline/Mu2eUtilities/inc/ModuleHistToolBase.hh"
#include "art/Utilities/make_tool.h"

#include <algorithm>
#include <cmath>
#include "CLHEP/Vector/ThreeVector.h"
#include "Offline/Mu2eUtilities/inc/TwoLinePCA.hh"
#include "Offline/Mu2eUtilities/inc/polyAtan2.hh"

using namespace std;

using CLHEP::Hep3Vector;

namespace mu2e {

  using namespace DeltaFinderTypes;

  class DeltaFinder: public art::EDProducer {
  public:

    struct Config {
      using Name    = fhicl::Name;
      using Comment = fhicl::Comment;
      fhicl::Atom<art::InputTag>     shCollTag        {Name("shCollTag"        ), Comment("SComboHit collection Name"   ) };
      fhicl::Atom<art::InputTag>     chCollTag        {Name("chCollTag"        ), Comment("ComboHit collection Name"    ) };
      fhicl::Atom<art::InputTag>     chfCollTag       {Name("chfCollTag"       ), Comment("StrawHitFlag collection Name") };
      fhicl::Atom<art::InputTag>     sdmcCollTag      {Name("sdmcCollTag"      ), Comment("StrawDigiMC collection Name" ) };
      fhicl::Atom<art::InputTag>     tpeakCollTag     {Name("tpeakCollTag"     ), Comment("Time peak collection Name"   ) };
      fhicl::Atom<int>               useTimePeaks     {Name("useTimePeaks"     ), Comment("to use time peaks set to 1"  ) };
      fhicl::Atom<int>               debugLevel       {Name("debugLevel"       ), Comment("debug level"                 ) };
      fhicl::Atom<int>               diagLevel        {Name("diagLevel"        ), Comment("diag level"                  ) };
      fhicl::Atom<int>               printErrors      {Name("printErrors"      ), Comment("print errors"                ) };
      fhicl::Atom<float>             minCaloDt        {Name("minCaloDt"        ), Comment("min Calo Dt"                 ) };
      fhicl::Atom<float>             maxCaloDt        {Name("maxCaloDt"        ), Comment("max Calo Dt"                 ) };
      fhicl::Atom<float>             meanPitchAngle   {Name("meanPitchAngle"   ), Comment("mean pitch angle"            ) };
      fhicl::Atom<float>             minHitTime       {Name("minHitTime"       ), Comment("min hit time"                ) };
      fhicl::Atom<int>               minNFacesWithHits{Name("minNFacesWithHits"), Comment("min N faces with hits"       ) };
      fhicl::Atom<int>               minNSeeds        {Name("minNSeeds"        ), Comment("min N seeds in a delta cand" ) };
      fhicl::Atom<int>               minDeltaNHits    {Name("minDeltaNHits"    ), Comment("min N combo  hits in a delta") };
      fhicl::Atom<float>             maxEleHitEnergy  {Name("maxEleHitEnergy"  ), Comment("max electron hit energy"     ) };
      fhicl::Atom<float>             minimumTime      {Name("minimumTime"      ), Comment("minimum time"                ) };
      fhicl::Atom<float>             maximumTime      {Name("maximumTime"      ), Comment("maximum time"                ) };
      fhicl::Atom<float>             maxHitSeedDt     {Name("maxHitSeedDt"     ), Comment("max DT(hit-seed)"            ) };
      fhicl::Atom<float>             maxChi2Seed      {Name("maxChi2Seed"      ), Comment("max seed chi2 (stereo)"      ) };
      fhicl::Atom<float>             maxChi2Radial    {Name("maxChi2Radial"    ), Comment("max chi2 (radial)"           ) };
      fhicl::Atom<float>             maxChi2All       {Name("maxChi2All"       ), Comment("max chi2 (all)"              ) };
      fhicl::Atom<float>             seedRes          {Name("seedRes"          ), Comment("stereo seed resolution"      ) };
      fhicl::Atom<float>             maxDxy           {Name("maxDxy"           ), Comment("max Dxy"                     ) };
      fhicl::Atom<int>               maxGap           {Name("maxGap"           ), Comment("max Gap"                     ) };
      fhicl::Atom<float>             sigmaR           {Name("sigmaR"           ), Comment("sigmaR"                      ) };
      fhicl::Atom<float>             maxDriftTime     {Name("maxDriftTime"     ), Comment("maxDriftTime"                ) };
      fhicl::Atom<float>             maxStrawDt       {Name("maxStrawDt"       ), Comment("max straw Dt"                ) };
      fhicl::Atom<float>             maxDtDs          {Name("maxDtDs"          ), Comment("max Dt/Dstation"             ) };
      fhicl::Atom<float>             maxDtDc          {Name("maxDtDc"          ), Comment("max deltaT between deltas"   ) };
      fhicl::Atom<int>               writeStrawHits   {Name("writeStrawHits"   ), Comment("if 1, write SCH coll"        ) };
      fhicl::Atom<int>               filter           {Name("filter"           ), Comment("if 1, write only nonDelta CH") };
      fhicl::Atom<int>               testOrder        {Name("testOrder"        ), Comment("if 1, test order"            ) };

      fhicl::Table<DeltaFinderTypes::Config> diagPlugin{Name("diagPlugin"       ), Comment("Diag plugin") };
    };

  protected:
    struct ChannelID {
      int Station;
      int Plane;
      int Face;
      int Panel;
      int Layer;
    };
//-----------------------------------------------------------------------------
// talk-to parameters: input collections and algorithm parameters
//-----------------------------------------------------------------------------
    art::InputTag   _shCollTag;
    art::InputTag   _chCollTag;
    art::InputTag   _chfCollTag;
    art::InputTag   _sdmcCollTag;
    art::InputTag   _tpeakCollTag;

    int             _useTimePeaks;
    float           _minCaloDt;
    float           _maxCaloDt;
    float           _meanPitchAngle;
    float           _minHitTime;           // min hit time
    int             _minNFacesWithHits;    // per station per seed
    int             _minNSeeds;            // min number of seeds in the delta electron cluster
    int             _minDeltaNHits;        // min number of hits of a delta candidate
    float           _maxEleHitEnergy;      //
    float           _minT;
    float           _maxT;
    float           _maxHitSeedDt;         //
    float           _maxChi2Seed;          //
    float           _maxChi2Neighbor;      //
    float           _maxChi2Radial;        //
    float           _maxChi2All;           // max chi2/N of a seed
    float           _seedRes;              //
    float           _maxDxy;
    int             _maxGap;
    float           _sigmaR;
    float           _sigmaR2;              // _sigmaR^2
    float           _maxDriftTime;
    float           _maxStrawDt;
    float           _maxDtDs;              // low-P electron travel time between two stations
    float           _maxDtDc;              // max deltaT between two delta candiates
    int             _writeStrawHits;
    int             _filter;

    int             _debugLevel;
    int             _diagLevel;
    int             _printErrors;
    int             _testOrder;

    std::unique_ptr<ModuleHistToolBase> _hmanager;
//-----------------------------------------------------------------------------
// cache event/geometry objects
//-----------------------------------------------------------------------------
    const StrawHitCollection*    _shColl ;

    const Tracker*               _tracker;
    const DiskCalorimeter*       _calorimeter;

    float                        _tdbuff; // following Dave - time division buffer

    DeltaFinderTypes::Data_t     _data;              // all data used
    int                          _testOrderPrinted;

    float                        _stationToCaloTOF[2][20];
    float                        _faceTOF[80];

    int                          _nComboHits;
    int                          _nStrawHits;

    vector<const ComboHit*>      _v;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  public:
    explicit DeltaFinder(const art::EDProducer::Table<Config>& config);

  private:

    void         orderID      (ChannelID* X, ChannelID* Ordered);
    void         deOrderID    (ChannelID* X, ChannelID* Ordered);
    void         testOrderID  ();
    void         testdeOrderID();

    bool         findData            (const art::Event&  Evt);
    int          checkDuplicates     (int Station,
                                      int Face1, const HitData_t* Hit1,
                                      int Face2, const HitData_t* Hit2);

    void         completeSeed        (DeltaSeed* Seed);
    void         connectSeeds        ();
    void         findSeeds           (int Station, int Face);
    void         findSeeds           ();
    void         initTimeCluster     (DeltaCandidate* Delta, TimeCluster* Tc);
    int          mergeDeltaCandidates();
    int          orderHits           ();
    void         pruneSeeds          (int Station);
    int          recoverMissingHits  ();
    int          recoverStation      (DeltaCandidate* Delta, int Station, int UseUsedHits, int RecoverSeeds);
    void         runDeltaFinder      ();

//-----------------------------------------------------------------------------
// overloaded methods of the module class
//-----------------------------------------------------------------------------
    void         beginJob() override;
    void         beginRun(art::Run& ARun) override;
    void         produce(art::Event& e) override;
  };

//-----------------------------------------------------------------------------
  DeltaFinder::DeltaFinder(const art::EDProducer::Table<Config>& config):
    art::EDProducer{config},
    _shCollTag             (config().shCollTag()        ),
    _chCollTag             (config().chCollTag()        ),
    _chfCollTag            (config().chfCollTag()       ),
    _sdmcCollTag           (config().sdmcCollTag()      ),
    _tpeakCollTag          (config().tpeakCollTag()     ),
    _useTimePeaks          (config().useTimePeaks()     ),
    _minCaloDt             (config().minCaloDt()        ),
    _maxCaloDt             (config().maxCaloDt()        ),
    _meanPitchAngle        (config().meanPitchAngle()   ),
    _minHitTime            (config().minHitTime()       ),
    _minNFacesWithHits     (config().minNFacesWithHits()),
    _minNSeeds             (config().minNSeeds()        ),
    _minDeltaNHits         (config().minDeltaNHits()    ),
    _maxEleHitEnergy       (config().maxEleHitEnergy()  ),
    _minT                  (config().minimumTime()      ), // nsec
    _maxT                  (config().maximumTime()      ), // nsec
    _maxHitSeedDt          (config().maxHitSeedDt()     ), // nsec
    _maxChi2Seed           (config().maxChi2Seed()      ),
    _maxChi2Radial         (config().maxChi2Radial()    ),
    _maxChi2All            (config().maxChi2All()       ),
    _seedRes               (config().seedRes()          ),
    _maxDxy                (config().maxDxy()           ),
    _maxGap                (config().maxGap()           ),
    _sigmaR                (config().sigmaR()           ),
    _maxDriftTime          (config().maxDriftTime()     ),
    _maxStrawDt            (config().maxStrawDt()       ),
    _maxDtDs               (config().maxDtDs()          ),
    _maxDtDc               (config().maxDtDc()          ),
    _writeStrawHits        (config().writeStrawHits()   ),
    _filter                (config().filter()           ),
    _debugLevel            (config().debugLevel()       ),
    _diagLevel             (config().diagLevel()        ),
    _printErrors           (config().printErrors()      ),
    _testOrder             (config().testOrder()        )
  {
    consumesMany<ComboHitCollection>(); // Necessary because fillStrawHitIndices calls getManyByType.

    produces<StrawHitFlagCollection>("ComboHits");
    if (_writeStrawHits == 1) produces<StrawHitFlagCollection>("StrawHits");
    if (_filter             ) produces<ComboHitCollection>();

                                        // this is a list of delta-electron candidates
    produces<TimeClusterCollection>();

    _testOrderPrinted = 0;
    _tdbuff           = 80.; // mm ... about less than 1 ns
    _sigmaR2          = _sigmaR*_sigmaR;

    if (_diagLevel != 0) _hmanager = art::make_tool<ModuleHistToolBase>(config().diagPlugin.get_PSet());
    else                 _hmanager = std::make_unique<ModuleHistToolBase>();

    _data.chCollTag   = _chCollTag;
    _data.chfCollTag  = _chfCollTag;
    _data.sdmcCollTag = _sdmcCollTag;
  }

  //-----------------------------------------------------------------------------
  void DeltaFinder::beginJob() {
    if (_diagLevel > 0) {
      art::ServiceHandle<art::TFileService> tfs;
      _hmanager->bookHistograms(tfs);
    }
  }

//-----------------------------------------------------------------------------
// create a Z-ordered map of the tracker
//-----------------------------------------------------------------------------
  void DeltaFinder::beginRun(art::Run& aRun) {
    mu2e::GeomHandle<mu2e::Tracker> tHandle;
    _tracker      = tHandle.get();
    _data.tracker = _tracker;

    mu2e::GeomHandle<mu2e::DiskCalorimeter> ch;
    _calorimeter = ch.get();

    ChannelID cx, co;
    int       nDisks    = _calorimeter->nDisk();
    double    disk_z[2] = {0};//given in the tracker frame

    for (int i=0; i<nDisks; ++i){
      Hep3Vector gpos = _calorimeter->disk(i).geomInfo().origin();
      Hep3Vector tpos = _calorimeter->geomUtil().mu2eToTracker(gpos);
      disk_z[i] = tpos.z();
    }

    float     z_tracker_center(0.);
    int       nPlanesPerStation(2);
    double    station_z(0);

    for (unsigned planeId=0; planeId<_tracker->nPlanes(); planeId++) {
      const Plane* pln = &_tracker->getPlane(planeId);
      int  ist = planeId/nPlanesPerStation;
      int  ipl = planeId % nPlanesPerStation;
      //calculate the time-of-flight between the station and each calorimeter disk
      //for a typical Conversion Electron
      if (ipl == 0) {
        station_z = pln->origin().z();
      }
      else {
        station_z = (station_z + pln->origin().z())/2.;
        for (int iDisk=0; iDisk<nDisks; ++iDisk){
          _stationToCaloTOF[iDisk][ist] = (disk_z[iDisk] - station_z)/sin(_meanPitchAngle)/CLHEP::c_light;
        }
      }

      for (unsigned ipn=0; ipn<pln->nPanels(); ipn++) {
        const Panel* panel = &pln->getPanel(ipn);
        int face;
        if (panel->id().getPanel() % 2 == 0) face = 0;
        else                                 face = 1;
        for (unsigned il=0; il<panel->nLayers(); ++il) {
          cx.Station   = ist;
          cx.Plane     = ipl;
          cx.Face      = face;
          cx.Panel     = ipn;
          cx.Layer     = il;
          orderID (&cx, &co);
          int os       = co.Station;
          int of       = co.Face;
          int op       = co.Panel;
          PanelZ_t* pz = &_data.oTracker[os][of][op];
          pz->fPanel   = panel;
//-----------------------------------------------------------------------------
// panel caches phi of its center and the z
//-----------------------------------------------------------------------------
          pz->wx  = panel->straw0Direction().x();
          pz->wy  = panel->straw0Direction().y();
          pz->phi = panel->straw0MidPoint().phi();
          pz->z   = (panel->getStraw(0).getMidPoint().z()+panel->getStraw(1).getMidPoint().z())/2.;
          int  uniqueFaceId = ipl*mu2e::StrawId::_nfaces + of;
          _faceTOF[uniqueFaceId] = (z_tracker_center - pz->z)/sin(_meanPitchAngle)/CLHEP::c_light;
        }
      }
      _data.stationUsed[ist] = 1;
    }
//-----------------------------------------------------------------------------
// it is enough to print that once
//-----------------------------------------------------------------------------
    if (_testOrder && (_testOrderPrinted == 0)) {
      testOrderID  ();
      testdeOrderID();
      _testOrderPrinted = 1;
    }

    if (_diagLevel != 0) _hmanager->debug(&_data,1);
  }

//-----------------------------------------------------------------------------
// make sure the two hits used to make a new seed are not a part of an already found seed
//-----------------------------------------------------------------------------
  int DeltaFinder::checkDuplicates(int Station, int Face1, const HitData_t* Hit1, int Face2, const HitData_t* Hit2) {

    int rc(0);

    int nseeds = _data.listOfSeeds[Station].size();
    for (int i=0; i<nseeds; i++) {
      //      bool h1_found(false), h2_found(false);

      DeltaSeed* seed = _data.listOfSeeds[Station][i];

      const HitData_t* h1 = seed->hitData[Face1];
      if (h1 != Hit1)                                                continue;
//-----------------------------------------------------------------------------
// Hit1 was found, check the same seed for Hit2 in Face2
//-----------------------------------------------------------------------------
      const HitData_t* h2 = seed->hitData[Face2];
      if (h2 == Hit2) {
//-----------------------------------------------------------------------------
// both Hit1 and Hit2 were found within the same seed, done
//-----------------------------------------------------------------------------
        rc = 1;
        break;
      }
    }
    return rc;
  }

  // unflagging preseed hits if seed is not completed?
  // start with object, fill in as it goes, then decide whether or not to keep it

  // move to other faces, use phi of preseed pos and phi of panels to determine which panel to check
  // loop through hits with time constraint
  // for chi2, use distance along the wire/resolution and distance across the wire/TBD(1 cm?)
  // find a way to accommodate for preseed size

  // Find "average straw" of all candidates in both directions, then find intersection of these two average straws

//-----------------------------------------------------------------------------
// assume that the seed has two hits
//-----------------------------------------------------------------------------
  void DeltaFinder::completeSeed(DeltaSeed* Seed) {

    assert ((Seed->SFace(1) >= 0) and (Seed->NHits() == 2));
//-----------------------------------------------------------------------------
// loop over remaining faces, 'f2' - face in question
//-----------------------------------------------------------------------------
    int station = Seed->Station();

    for (int face=0; face<kNFaces; face++) {
      if (Seed->fFaceProcessed[face] == 1)                            continue;
//-----------------------------------------------------------------------------
// face is different from the two first faces used
//-----------------------------------------------------------------------------
      for (int p2=0; p2<3; ++p2) {
        PanelZ_t* panelz = &_data.oTracker[station][face][p2];
        double dphi      = Seed->Phi()-panelz->phi;
        if (dphi < -M_PI) dphi += 2*M_PI;
        if (dphi >  M_PI) dphi -= 2*M_PI;
        if (fabs(dphi) >= M_PI/3)                                     continue;
//-----------------------------------------------------------------------------
// panel overlaps with the seed, look at its hits
//-----------------------------------------------------------------------------
        double nx    = panelz->wx;
        double ny    = panelz->wy;

        float sxy_dot_w = Seed->CofM.x()*nx+Seed->CofM.y()*ny;

        assert(sxy_dot_w != 1.e10);             // test
        if (sxy_dot_w > 1.e5) printf("emoe\n"); // test

        int    nhits = panelz->fHitData.size();
        for (int ih=0; ih<nhits; ih++) {
//-----------------------------------------------------------------------------
// 2017-10-05 PM: consider all hits
// hit time should be consistent with the already existing times - the difference
// between any two measured hit times should not exceed _maxDriftTime
// (_maxDriftTime represents the maximal drift time in the straw, should there be some tolerance?)
//-----------------------------------------------------------------------------
          HitData_t* hd            = &panelz->fHitData[ih];
          float corr_time          = hd->fCorrTime;

          if (corr_time-Seed->T0Max() > _maxHitSeedDt)                break;
          if (Seed->T0Min()-corr_time > _maxHitSeedDt)                continue;

          const ComboHit*   ch     = hd->fHit;
          double x0                = ch->pos().x();
          double y0                = ch->pos().y();
          double dx                = x0-Seed->CofM.x();
          double dy                = y0-Seed->CofM.y();
//-----------------------------------------------------------------------------
// split into wire parallel and perpendicular components
// assume wire direction vector is normalized to 1
//-----------------------------------------------------------------------------
          float dxy_dot_wdir       = dx*nx+dy*ny;

          float dx_perp            = dx-dxy_dot_wdir*nx;
          float dy_perp            = dy-dxy_dot_wdir*ny;
          float d_perp_2           = dx_perp*dx_perp+dy_perp*dy_perp;

          // add 2cm^2 as an uncertainty on he intersection, can do better :

          float chi2_par           = (dxy_dot_wdir*dxy_dot_wdir)/(hd->fSigW2+_seedRes*_seedRes);
          float chi2_perp          = d_perp_2/_sigmaR2;
          float chi2               = chi2_par + chi2_perp;
          if (chi2 >= _maxChi2Radial)                           continue;
//-----------------------------------------------------------------------------
// add hit
//-----------------------------------------------------------------------------
          hd->fChi2Min             = chi2;
          Seed->AddHit(hd,face);
        }
      }
    }
//-----------------------------------------------------------------------------
// update seed time and X and Y coordinates, accurate knowledge of Z is not very relevant
//-----------------------------------------------------------------------------
    Seed->CalculateCogAndChi2(_sigmaR2);
  }

//-----------------------------------------------------------------------------
// loop over stations
// start with a seed
// move to next station
// loop over seeds, look for one with similar xy and time (split into components impractical?)
// if multiple, select one with best chi2
// continue, incrementing over stations until all finished
// what to do if there are gaps in the path?
// update center of mass?
//-----------------------------------------------------------------------------
  void DeltaFinder::connectSeeds() {

    for (int is=0; is<kNStations; is++) {
//-----------------------------------------------------------------------------
// 1. loop over existing seeds and match them to existing delta candidates
//-----------------------------------------------------------------------------
      int nseeds = _data.listOfSeeds[is].size();
      for (int ids=0; ids<nseeds; ids++) {
        DeltaSeed* seed = _data.listOfSeeds[is][ids];

        if (! seed->Good() )                                          continue;
        if (seed->Used()   )                                          continue;
        if (seed->fNFacesWithHits < _minNFacesWithHits)               continue;
//-----------------------------------------------------------------------------
// first, loop over existing delta candidates and try to associate the seed
// with one of them
//-----------------------------------------------------------------------------
        DeltaCandidate* closest(nullptr);
        float           rho2min (_maxDxy*_maxDxy);

        int ndelta = _data.listOfDeltaCandidates.size();
        for (int idc=0; idc<ndelta; idc++) {
          DeltaCandidate* dc = _data.deltaCandidate(idc);
//-----------------------------------------------------------------------------
// skip candidates already merged with others
//-----------------------------------------------------------------------------
          if (dc->Active() == 0      )                                continue;
          if (dc->seed[is] != nullptr)                                continue;
          int last = dc->LastStation();
//-----------------------------------------------------------------------------
// a delta candidate starting from a seed in a previous station may already have
// a seed in this station found, skip such a candidate
//-----------------------------------------------------------------------------
          if (last == is)                                             continue;
          int gap  = is-last;
          if (gap > _maxGap)                                          continue;
          dc->fT0Min[is] = dc->fT0Min[last]-_maxDtDs*gap;
          dc->fT0Max[is] = dc->fT0Max[last]+_maxDtDs*gap;
          if (dc->fT0Min[is] > seed->T0Max() + _maxDriftTime)         continue;
          if (dc->fT0Max[is] < seed->T0Min() - _maxDriftTime)         continue;
//-----------------------------------------------------------------------------
// the time is OK'ish - checks should be implemented more accurately (FIXME)
// look at the coordinates
//-----------------------------------------------------------------------------
          float dx   = dc->CofM.x()-seed->CofM.x();
          float dy   = dc->CofM.y()-seed->CofM.y();
          float rho2 = dx*dx+dy*dy;
          if (rho2 >= rho2min)                                        continue;
//-----------------------------------------------------------------------------
// everything matches, add the seed to the candidate
//-----------------------------------------------------------------------------
          closest = dc;
          rho2min = rho2;
        }
//-----------------------------------------------------------------------------
// if a DeltaSeed has been "attached" to a DeltaCandidate, this is it.
//-----------------------------------------------------------------------------
        if (closest) {
          closest->AddSeed(seed,is);
                                                                      continue;
        }
//-----------------------------------------------------------------------------
// DeltaSeed has not been linked to any existing delta candidate, create
// a new delta candidate and see if it is good enough
//-----------------------------------------------------------------------------
        DeltaCandidate delta(ndelta,seed,is);
//-----------------------------------------------------------------------------
// first, try to extend it backwards, in a compact way, to pick missing single hits
// seeds should've been picked up !
//-----------------------------------------------------------------------------
        for (int is2=is-1; is2>=0; is2--) {
          delta.fT0Min[is2] = delta.fT0Min[is]-_maxDtDs;
          delta.fT0Max[is2] = delta.fT0Max[is]+_maxDtDs;
          recoverStation(&delta,is2,1,0);
//-----------------------------------------------------------------------------
// continue only if found something, allow one gap
//-----------------------------------------------------------------------------
          if (delta.fFirstStation-is2 > 1) break;
        }
//-----------------------------------------------------------------------------
// next, try to extend it forward, by one step, use of seeds is allowed
//-----------------------------------------------------------------------------
        if (is < kNStations-1) {
          delta.fT0Min[is+1] = delta.fT0Min[is]-_maxDtDs;
          delta.fT0Max[is+1] = delta.fT0Max[is]+_maxDtDs;
          recoverStation(&delta,is+1,1,1);
        }
//-----------------------------------------------------------------------------
// store only delta candidates with hits in more than 2 stations
// for each station define expected T0min and T0max
// to keep _minNSeeds=2 need to look forward...
//-----------------------------------------------------------------------------
        if (delta.n_seeds >= _minNSeeds) {
          _data.listOfDeltaCandidates.push_back(delta);
        }
        else {
//-----------------------------------------------------------------------------
// mark all seeds as unassigned, this should be happening only in 1-seed case
//-----------------------------------------------------------------------------
          for (int is=delta.fFirstStation; is<=delta.fLastStation; is++) {
            DeltaSeed* ds = delta.seed[is];
            if (ds) ds->fDeltaIndex = -1;
          }
        }
      }
//-----------------------------------------------------------------------------
// all seeds in a given station processed
// loop over existing delta candidates which do not have seeds in a given station
// and see if can pick up single hits
//-----------------------------------------------------------------------------
      int ndelta = _data.listOfDeltaCandidates.size();
      for (int idc=0; idc<ndelta; idc++) {
        DeltaCandidate* dc = _data.deltaCandidate(idc);
        int last = dc->LastStation();
        if (last != is-1)                                           continue;
//-----------------------------------------------------------------------------
// if a delta candidate has been created in this routine, time limits
// may not be defined. Make sure they are
//-----------------------------------------------------------------------------
        dc->fT0Min[is] = dc->fT0Min[last]-_maxDtDs;
        dc->fT0Max[is] = dc->fT0Max[last]+_maxDtDs;
        recoverStation(dc,is,1,0);
      }
    }

//-----------------------------------------------------------------------------
// at this point we have a set of delta candidates, which might need to be merged
//-----------------------------------------------------------------------------
    mergeDeltaCandidates();
  }

//-----------------------------------------------------------------------------
  void DeltaFinder::deOrderID(ChannelID* X, ChannelID* O) {

    X->Station = O->Station;

    X->Plane   = O->Plane;

    if(O->Station % 2 ==  0) {
      if(O->Plane == 0) X->Face = 1 - O->Face;
      else X->Face = O->Face - 2;
    }
    else {
      if(O->Plane == 0) X->Face = O->Face;
      else X->Face = 3 - O->Face;
    }

    if(X->Face == 0) X->Panel = O->Panel * 2;
    else X->Panel = 1 + (O->Panel * 2);

    int n = X->Station + X->Plane + X->Face;
    if(n % 2 == 0) X->Layer = 1 - O->Layer;
    else X->Layer = O->Layer;
  }

//-----------------------------------------------------------------------------
  bool DeltaFinder::findData(const art::Event& Evt) {
    _data.chcol    = nullptr;
    _data.tpeakcol = nullptr;

    if (_useTimePeaks == 1){
      auto tpeakH = Evt.getValidHandle<mu2e::TimeClusterCollection>(_tpeakCollTag);
      _data.tpeakcol = tpeakH.product();
    }

    auto chcH   = Evt.getValidHandle<mu2e::ComboHitCollection>(_chCollTag);
    _data.chcol = chcH.product();

    auto shcH = Evt.getValidHandle<mu2e::StrawHitCollection>(_shCollTag);
    _shColl   = shcH.product();

    return (_data.chcol != nullptr) and (_shColl != nullptr);
  }

//-----------------------------------------------------------------------------
// find delta electron seeds in 'Station' with hits in faces 'f' and 'f+1'
// do not consider proton hits with eDep > _minHitEnergy
//-----------------------------------------------------------------------------
  void DeltaFinder::findSeeds(int Station, int Face) {

    for (int p=0; p<3; ++p) {                        // loop over panels in this face
      PanelZ_t* panelz1 = &_data.oTracker[Station][Face][p];
      double nx1        = panelz1->wx;
      double ny1        = panelz1->wy;
      int    nh1        = panelz1->fHitData.size();
      for (int h1=0; h1<nh1; ++h1) {
//-----------------------------------------------------------------------------
// hit has not been used yet to start a seed, however it could've been used as a second seed
//-----------------------------------------------------------------------------
        HitData_t*      hd1 = &panelz1->fHitData[h1];
        float           ct1 = hd1->fCorrTime;
        //        if (ct1 < _minHitTime)                                        continue;
        double x1           = hd1->fHit->pos().x();
        double y1           = hd1->fHit->pos().y();

        int counter         = 0;                // number of stereo candidate hits close to set up counter
//-----------------------------------------------------------------------------
// loop over the second faces
//-----------------------------------------------------------------------------
        for (int f2=Face+1; f2<kNFaces; f2++) {
          for (int p2=0; p2<3; ++p2) {         // loop over panels
            PanelZ_t* panelz2 = &_data.oTracker[Station][f2][p2];
//-----------------------------------------------------------------------------
// check if the two panels overlap in XY
// 2D angle between the vectors pointing to the panel centers, can't be greater than 2*pi/3
//-----------------------------------------------------------------------------
            float dphi = panelz2->phi - panelz1->phi;
            if (dphi < -M_PI) dphi += 2*M_PI;
            if (dphi >  M_PI) dphi -= 2*M_PI;
            if (abs(dphi) >= 2*M_PI/3.)                                continue;
//-----------------------------------------------------------------------------
// panels do overlap, check the time. tmin and tmax also detect panels w/o hits
//-----------------------------------------------------------------------------
            if (panelz2->tmin - ct1 > _maxDriftTime)                   continue;
            if (ct1 - panelz2->tmax > _maxDriftTime)                   continue;

            double nx2   = panelz2->wx;
            double ny2   = panelz2->wy;
            double n1n2  = nx1*nx2+ny1*ny2;
            double q12   = 1-n1n2*n1n2;
            double res_z = (panelz1->z+panelz2->z)/2;

            int    nh2 = panelz2->fHitData.size();
            for (int h2=0; h2<nh2;++h2) {
              HitData_t* hd2 = &panelz2->fHitData[h2];
              float      ct2 = hd2->fCorrTime;
//-----------------------------------------------------------------------------
// hits are ordered in time, so if ct2-ct > _maxDriftTime, can proceed with the next panel
//-----------------------------------------------------------------------------
              if (ct2 - ct1 > _maxDriftTime)                           break;
              if (ct1 - ct2 > _maxDriftTime)                           continue;
              ++counter;                                            // number of hits close to the first one
//-----------------------------------------------------------------------------
// intersect the two straws, we need coordinates of the intersection point and
// two distances from hits to the intersection point, 4 numbers in total
//-----------------------------------------------------------------------------
              double x2    = hd2->fHit->pos().x();
              double y2    = hd2->fHit->pos().y();

              double r12n1 = (x1-x2)*nx1+(y1-y2)*ny1;
              double r12n2 = (x1-x2)*nx2+(y1-y2)*ny2;

              double wd1   = -(r12n2*n1n2-r12n1)/q12;

              double res_x = x1-nx1*wd1;
              double res_y = y1-ny1*wd1;

              double wd2   = -(r12n2-n1n2*r12n1)/q12;
//-----------------------------------------------------------------------------
// require both hits to be close enough to the intersection point
//-----------------------------------------------------------------------------
              float hd1_chi2 = wd1*wd1/hd1->fSigW2;
              float hd2_chi2 = wd2*wd2/hd2->fSigW2;
              if (hd1_chi2 > _maxChi2Seed)                            continue;
              if (hd2_chi2 > _maxChi2Seed)                            continue;
              if ((hd1_chi2+hd2_chi2)/2 > _maxChi2Seed)               continue;
//-----------------------------------------------------------------------------
// check whether there already is a seed containing both hits
//-----------------------------------------------------------------------------
              int is_duplicate = checkDuplicates(Station,Face,hd1,f2,hd2);
              if (is_duplicate)                               continue;
//-----------------------------------------------------------------------------
// new seed : an intersection of two wires coresponsing to close in time combo hits
//-----------------------------------------------------------------------------
              hd1->fChi2Min    = hd1_chi2;
              hd2->fChi2Min    = hd2_chi2;

              int nseeds = _data.listOfSeeds[Station].size();
              DeltaSeed* seed  = new DeltaSeed(nseeds,Station,Face,hd1,f2,hd2);

              seed->CofM.SetXYZ(res_x,res_y,res_z);
              seed->fPhi = polyAtan2(res_y,res_x);
//-----------------------------------------------------------------------------
// mark both hits as a part of a seed, so they would not be used individually
// - see HitData_t::Used()
//-----------------------------------------------------------------------------
              hd1->fSeed  = seed;
              hd2->fSeed  = seed;
//-----------------------------------------------------------------------------
// complete search for hits of this seed
//-----------------------------------------------------------------------------
              completeSeed(seed);
//-----------------------------------------------------------------------------
// book-keeping: increment total number of found seeds
//-----------------------------------------------------------------------------
              _data.listOfSeeds[Station].push_back(seed);
              _data.nseeds                      += 1;
              _data.nseeds_per_station[Station] += 1;
            }
          }
        }
 //-----------------------------------------------------------------------------
 // this is needed for diagnostics only
 //-----------------------------------------------------------------------------
        if (_diagLevel > 0) {
          hd1->fNSecondHits  = counter ;
        }
      }
    }
  }

//-----------------------------------------------------------------------------
// TODO: update the time as more hits are added
//-----------------------------------------------------------------------------
  void DeltaFinder::findSeeds() {

    for (int s=0; s<kNStations; ++s) {
      for (int face=0; face<kNFaces-1; face++) {
        findSeeds(s,face);
      }
      pruneSeeds(s);
    }
  }

//-----------------------------------------------------------------------------
// define the time cluster parameters starting from a DeltaCandidate
//-----------------------------------------------------------------------------
  void DeltaFinder::initTimeCluster(DeltaCandidate* Dc, TimeCluster* Tc) {
  }

//-----------------------------------------------------------------------------
// merge Delta Candidates : check for duplicates !
//-----------------------------------------------------------------------------
  int DeltaFinder::mergeDeltaCandidates() {
    int rc(0);
    float max_d2(20*20);  // mm^2, to be adjusted FIXME

    int ndelta = _data.listOfDeltaCandidates.size();

    for (int i1=0; i1<ndelta-1; i1++) {
      DeltaCandidate* dc1 = _data.deltaCandidate(i1);
      if (dc1->Active() == 0)                                          continue;
      float x1 = dc1->CofM.x();
      float y1 = dc1->CofM.y();
      for (int i2=i1+1; i2<ndelta; i2++) {
        DeltaCandidate* dc2 = _data.deltaCandidate(i2);
        if (dc2->Active() == 0)                                        continue;
        float x2 = dc2->CofM.x();
        float y2 = dc2->CofM.y();
        float d2 = (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
        if (d2 > max_d2)                                               continue;
//-----------------------------------------------------------------------------
// too lazy to extrapolate the time to the same Z...  ** FIXME
//-----------------------------------------------------------------------------
        float t1 = dc1->Time(dc1->LastStation());
        float t2 = dc2->Time(dc2->FirstStation());
//-----------------------------------------------------------------------------
// time check could be done more intelligently - compare time at the same Z
//-----------------------------------------------------------------------------
        if (fabs(t1-t2) > _maxDtDc)                                    continue;

        int dds = dc2->FirstStation()-dc1->LastStation();
        if (dds < 0) {
          if (_printErrors) {
            printf("ERROR in DeltaFinder::%s:",__func__);
            printf("i1, i2, dc1->LastStation, dc2->FirstStation: %2i %2i %2i %2i \n",
                   i1,i2,dc1->LastStation(),dc2->FirstStation());
          }
                                                                       continue;
        }
        else if (dds > _maxGap)                                        continue;
//-----------------------------------------------------------------------------
// merge two delta candidates, not too far from each other in Z,
// leave dc1 active, mark dc2 as not active
//-----------------------------------------------------------------------------
        dc1->MergeDeltaCandidate(dc2,_printErrors);
        dc2->SetIndex(-1000-dc1->Index());
      }
    }
    return rc;
  }

//-----------------------------------------------------------------------------
// Custom comparator to sort in ascending order
//-----------------------------------------------------------------------------
  bool comparator(const ComboHit*& a, const ComboHit*& b) {
    return a->correctedTime() < b->correctedTime();
  }

//------------------------------------------------------------------------------
// I'd love to use the hit flags, however that is confusing:
// - hits with very large deltaT get placed to the middle of the wire and not flagged,
// - however, some hits within the fiducial get flagged with the ::radsel flag...
// use only "good" hits
//-----------------------------------------------------------------------------

  int DeltaFinder::orderHits() {
    ChannelID cx, co;

    _v.clear();
    // _v.resize(_nComboHits);

    for (int i=0; i<_nComboHits; i++) {
      _v.push_back(&(*_data.chcol)[i]); //  = &(*_chcol)[i];
    }

    std::sort(_v.begin(), _v.end(), comparator);
//-----------------------------------------------------------------------------
// at this point all hits are ordered in time
//-----------------------------------------------------------------------------
    for (int h=0; h<_nComboHits; ++h) {
      //      const ComboHit* ch = &(*_data.chcol)[h];
      const ComboHit* ch = _v[h];
      float corr_time    = ch->correctedTime();

      if (ch->energyDep() > _maxEleHitEnergy             )  continue;
      if ((corr_time      < _minT) || (corr_time > _maxT))  continue;

      cx.Station                 = ch->strawId().station();
      cx.Plane                   = ch->strawId().plane() % 2;
      cx.Face                    = -1;
      cx.Panel                   = ch->strawId().panel();
      cx.Layer                   = ch->strawId().layer();

                                              // get Z-ordered location
      orderID(&cx, &co);

      int os       = co.Station;
      int of       = co.Face;
      int op       = co.Panel;
      int ol       = co.Layer;

      if (_useTimePeaks == 1) {
        bool               intime(false);
        int                nTPeaks  = _data.tpeakcol->size();
        double             hitTime  = ch->correctedTime();
        const CaloCluster* cl(nullptr);
        int                iDisk(-1);

        for (int i=0; i<nTPeaks; ++i) {
          cl    = _data.tpeakcol->at(i).caloCluster().get();
          if (cl == nullptr) {
            printf(">>> DeltaFinder::orderHits() no CaloCluster found within the time peak %i\n", i);
            continue;
          }
          iDisk = cl->diskID();
          double    dt = cl->time() - (hitTime + _stationToCaloTOF[iDisk][os]);
          if ( (dt < _maxCaloDt) && (dt > _minCaloDt) ) {
            intime = true;
            break;
          }
        }
        if (!intime)                                    continue;
      }

      PanelZ_t* pz = &_data.oTracker[os][of][op];

      if (_printErrors) {
        if ((os < 0) || (os >= kNStations     )) printf(" >>> ERROR: wrong station number: %i\n",os);
        if ((of < 0) || (of >= kNFaces        )) printf(" >>> ERROR: wrong face    number: %i\n",of);
        if ((op < 0) || (op >= kNPanelsPerFace)) printf(" >>> ERROR: wrong panel   number: %i\n",op);
        if ((ol < 0) || (ol >= 2              )) printf(" >>> ERROR: wrong layer   number: %i\n",ol);
      }

      // float sigw = ch->posRes(ComboHit::wire);
      pz->fHitData.push_back(HitData_t(ch)); // ,sigw));

      if (pz->tmin > corr_time) pz->tmin = corr_time;
      if (pz->tmax < corr_time) pz->tmax = corr_time;
    }

    return 0;
  }

//-----------------------------------------------------------------------------
  void DeltaFinder::orderID(ChannelID* X, ChannelID* O) {
    if (X->Panel % 2 == 0) X->Face = 0;
    else                   X->Face = 1; // define original face

    O->Station = X->Station; // stations already ordered
    O->Plane   = X->Plane;   // planes already ordered, but not necessary for ordered construct

    if (X->Station % 2 == 0) {
      if (X->Plane == 0) O->Face = 1 - X->Face;
      else               O->Face = X->Face + 2;
    }
    else {
      if (X->Plane == 0) O->Face = X->Face;
      else               O->Face = 3 - X->Face; // order face
    }

    O->Panel = int(X->Panel/2);                // order panel

    int n = X->Station + X->Plane + X->Face;   // pattern has no intrinsic meaning, just works
    if (n % 2 == 0) O->Layer = 1 - X->Layer;
    else            O->Layer = X->Layer;       // order layer
  }

//-----------------------------------------------------------------------------
  void DeltaFinder::produce(art::Event& Event) {

    if (_debugLevel) printf(">>> DeltaFinder::produce  event number: %10i\n",Event.event());
//-----------------------------------------------------------------------------
// clear memory in the beginning of event processing and cache event pointer
//-----------------------------------------------------------------------------
    _data.event  = &Event;

    _data.nseeds = 0;
    _data.debugLevel = _debugLevel;

    for (int is=0; is<kNStations; is++) {
      _data.nseeds_per_station[is] = 0;

      for (auto ds=_data.listOfSeeds[is].begin(); ds!=_data.listOfSeeds[is].end(); ds++) {
        delete *ds;
      }
      _data.listOfSeeds[is].clear();
    }

    _data.listOfDeltaCandidates.clear();
//-----------------------------------------------------------------------------
// process event
//-----------------------------------------------------------------------------
    if (! findData(Event)) {
      const char* message = "mu2e::DeltaFinder_module::produce: data missing or incomplete";
      throw cet::exception("RECO")<< message << endl;
    }

    _nComboHits = _data.chcol->size();
    _nStrawHits = _shColl->size();

    runDeltaFinder();
//-----------------------------------------------------------------------------
// form output - flag combo hits
//-----------------------------------------------------------------------------
    unique_ptr<StrawHitFlagCollection> up_chfcol(new StrawHitFlagCollection(_nComboHits));
    _data.chfcol = up_chfcol.get();

    for (int i=0; i<_nComboHits; i++) {
      StrawHitFlag flag;
      const ComboHit* ch = &(*_data.chcol)[i];
      int ind            = ch->indexArray().at(0);
      const StrawHit* sh = &_shColl->at(ind);
//-----------------------------------------------------------------------------
// make decision based on the first straw hit
//-----------------------------------------------------------------------------
      if (sh->energyDep() < _maxEleHitEnergy) flag.merge(StrawHitFlag::energysel);
      (*_data.chfcol)[i] = flag;
    }

    const ComboHit* ch0(0);
    if (_nComboHits > 0) ch0 = &_data.chcol->at(0);

    StrawHitFlag deltamask(StrawHitFlag::bkg);

    unique_ptr<TimeClusterCollection>  tcColl(new TimeClusterCollection);

    int ndeltas = _data.listOfDeltaCandidates.size();

    for (int i=0; i<ndeltas; i++) {
      DeltaCandidate* dc = &_data.listOfDeltaCandidates.at(i);
//-----------------------------------------------------------------------------
// skip merged in delta candidates
// also require a delta candidate to have at least 5 hits
//-----------------------------------------------------------------------------
      if (dc->Active() == 0)                                          continue;
      if (dc->NHits () < _minDeltaNHits)                              continue;
      for (int station=dc->fFirstStation; station<=dc->fLastStation; station++) {
        DeltaSeed* ds = dc->seed[station];
        if (ds != nullptr) {
//-----------------------------------------------------------------------------
// loop over the hits and flag each of them as delta
//-----------------------------------------------------------------------------
          for (int face=0; face<kNFaces; face++) {
            const HitData_t* hd = ds->HitData(face);
            if (hd == nullptr)                                        continue;
            int loc = hd->fHit-ch0;
            _data.chfcol->at(loc).merge(deltamask);
          }
        }
      }
//-----------------------------------------------------------------------------
// make a time cluster out of each active DeltaCandidate
//-----------------------------------------------------------------------------
      TimeCluster new_tc;
      initTimeCluster(dc,&new_tc);
      tcColl->push_back(new_tc);
    }

    Event.put(std::move(tcColl));
//-----------------------------------------------------------------------------
// in the end of event processing fill diagnostic histograms
//-----------------------------------------------------------------------------
    if (_diagLevel  > 0) _hmanager->fillHistograms(&_data);
    if (_debugLevel > 0) _hmanager->debug(&_data,2);
//-----------------------------------------------------------------------------
// create the collection of StrawHitFlag for the StrawHitCollection
//-----------------------------------------------------------------------------
    if (_writeStrawHits == 1) {
                                        // first, copy over the original flags

      std::unique_ptr<StrawHitFlagCollection> shfcol(new StrawHitFlagCollection(_nStrawHits));

      for(int ich=0; ich<_nComboHits; ich++) {
        const ComboHit* ch   = &(*_data.chcol )[ich];
        StrawHitFlag    flag =  (*_data.chfcol)[ich];
        flag.merge(ch->flag());
        for (auto ish : ch->indexArray()) {
          (*shfcol)[ish] = flag;
        }
      }

      Event.put(std::move(shfcol),"StrawHits");
    }
//-----------------------------------------------------------------------------
// moving in the end, after diagnostics plugin routines have been called - move
// invalidates the original pointer...
//-----------------------------------------------------------------------------
    Event.put(std::move(up_chfcol),"ComboHits");
  }

//-----------------------------------------------------------------------------
// some of found seeds could be duplicates or ghosts
// in case two DeltaSeeds share the first seed hit, leave only the best one
// the seeds we're loooping over have been reconstructed within the same station
// also reject seeds with Chi2Tot > _maxChi2Tot=10
//-----------------------------------------------------------------------------
  void DeltaFinder::pruneSeeds(int Station) {
    int nseeds =  _data.listOfSeeds[Station].size();

    for (int i1=0; i1<nseeds-1; i1++) {
      DeltaSeed* ds1 = _data.listOfSeeds[Station][i1];
      if (ds1->fGood < 0)                                             continue;

      if (ds1->Chi2AllN() > _maxChi2All) {
        ds1->fGood = -1000-i1;
                                                                      continue;
      }

      float tmean1 = ds1->Time();

      for (int i2=i1+1; i2<nseeds; i2++) {
        DeltaSeed* ds2 = _data.listOfSeeds[Station][i2];
        if (ds2->fGood < 0)                                           continue;

        if (ds2->Chi2AllN() > _maxChi2All) {
          ds2->fGood = -1000-i2;
                                                                      continue;
        }

        float tmean2 = ds2->Time();

        if (fabs(tmean1-tmean2) > _maxDriftTime)                      continue;
//-----------------------------------------------------------------------------
// the two segments are close in time , both have acceptable chi2's
// *FIXME* didn't check distance !!!!!
// so far, allow duplicates during the search
// the two DeltaSeeds share could have significantly overlapping hit content
//-----------------------------------------------------------------------------
        int noverlap            = 0;
        int nfaces_with_overlap = 0;
        for (int face=0; face<kNFaces; face++) {
          const HitData_t* hh1 = ds1->hitData[face];
          const HitData_t* hh2 = ds2->hitData[face];
          if (hh1 and (hh1 == hh2)) {
            noverlap            += 1;
            nfaces_with_overlap += 1;
          }
        }

        if (nfaces_with_overlap > 1) {
//-----------------------------------------------------------------------------
// overlap significant, leave in only one DeltaSeed - which one?
//-----------------------------------------------------------------------------
          if (ds1->fNFacesWithHits > ds2->fNFacesWithHits) {
            ds2->fGood = -1000-i1;
          }
          else if (ds2->fNFacesWithHits > ds1->fNFacesWithHits) {
            ds1->fGood = -1000-i2;
            break;
          }
          else {
//-----------------------------------------------------------------------------
//both seeds have the same number of hits - compare chi2's
//-----------------------------------------------------------------------------
            if (ds1->Chi2AllN() <  ds2->Chi2AllN()) {
              ds2->fGood = -1000-i1;
            }
            else {
              ds1->fGood = -1000-i2;
              break;
            }
          }
        }
        else if (nfaces_with_overlap > 0) {
//-----------------------------------------------------------------------------
// only one overlapping hit
// special treatment of 2-hit seeds to reduce the number of ghosts
//-----------------------------------------------------------------------------
          if (ds1->fNFacesWithHits == 2) {
            if (ds2->fNFacesWithHits > 2) {
              ds1->fGood = -1000-i2;
              break;
            }
            else {
//-----------------------------------------------------------------------------
// the second seed also has 2 faces with hits
//-----------------------------------------------------------------------------
              if (ds1->Chi2AllN() <  ds2->Chi2AllN()) ds2->fGood = -1000-i1;
              else {
                ds1->fGood = -1000-i2;
                break;
              }
            }
          }
          else {
//-----------------------------------------------------------------------------
// the first seed has N>2 hits
//-----------------------------------------------------------------------------
            if (ds2->fNFacesWithHits == 2) {
//-----------------------------------------------------------------------------
// the 2nd seed has only 2 hits and there is an overlap
//-----------------------------------------------------------------------------
              ds2->fGood = -1000-i1;
            }
            else {
//-----------------------------------------------------------------------------
// the second seed also has N>2 faces with hits, but there is only one overlap
// leave both seeds in
//-----------------------------------------------------------------------------
            }
          }
        }
      }
    }
  }

//------------------------------------------------------------------------------
// start from looking at the "holes" in the seed pattern
// delta candidates in the list are already required to have at least 2 segments
// extend them outwards by one station
//-----------------------------------------------------------------------------
  int DeltaFinder::recoverMissingHits() {

    int ndelta = _data.listOfDeltaCandidates.size();
    for (int idelta=0; idelta<ndelta; idelta++) {
      DeltaCandidate* dc = &_data.listOfDeltaCandidates[idelta];
//-----------------------------------------------------------------------------
// don't extend candidates made out of one segment - but there is no such
// start from the first station to define limits
//-----------------------------------------------------------------------------
      int s1 = dc->fFirstStation;
      int s2 = dc->fLastStation-1;
      int last(-1);
      float t0min(-1.), t0max(-1.);
//-----------------------------------------------------------------------------
// first check inside "holes"
//-----------------------------------------------------------------------------
      for (int i=s1; i<=s2; i++) {
        if (dc->seed[i] != nullptr) {
          last  = i;
          t0min = dc->seed[i]->T0Min();
          t0max = dc->seed[i]->T0Max();
          continue;
        }
//-----------------------------------------------------------------------------
// define expected T0 limits
//-----------------------------------------------------------------------------
        dc->fT0Min[i] = t0min-_maxDtDs*(i-last);
        dc->fT0Max[i] = t0max+_maxDtDs*(i-last);
        recoverStation(dc,i,1,0);
      }

      last  = dc->fFirstStation;
      for (int i=last-1; i>=0; i--) {
//-----------------------------------------------------------------------------
// skip empty stations
//-----------------------------------------------------------------------------
        if (dc->fFirstStation -i > _maxGap) break;
        dc->fT0Min[i] = dc->fT0Min[dc->fFirstStation]-_maxDtDs*(dc->fFirstStation-i);
        dc->fT0Max[i] = dc->fT0Max[dc->fFirstStation]+_maxDtDs*(dc->fFirstStation-i);
        recoverStation(dc,i,1,0);
      }

      last = dc->fLastStation;
      for (int i=last+1; i<kNStations; i++) {
//-----------------------------------------------------------------------------
// skip empty stations
//-----------------------------------------------------------------------------
        if (i-dc->fLastStation > _maxGap) break;
        dc->fT0Min[i] = dc->fT0Min[dc->fLastStation]-_maxDtDs*(i-dc->fLastStation);
        dc->fT0Max[i] = dc->fT0Max[dc->fLastStation]+_maxDtDs*(i-dc->fLastStation);
        recoverStation(dc,i,1,0);
      }
    }

    return 0;
  }

//------------------------------------------------------------------------------
// try to recover hits of a 'Delta' candidate in a given 'Station'
// the delta candidate doesn't have hits in this station, check all hits here
// when predicting time, use the same value of Z for both layers of a given face
//-----------------------------------------------------------------------------
  int DeltaFinder::recoverStation(DeltaCandidate* Delta, int Station, int UseUsedHits, int RecoverSeeds) {

                                        // predicted time range for this station
    float t0min  = Delta->T0Min(Station);
    float t0max  = Delta->T0Max(Station);
    float xdelta = Delta->CofM.x();
    float ydelta = Delta->CofM.y();
//-----------------------------------------------------------------------------
// first, loop over the existing seeds - need for the forward step
//-----------------------------------------------------------------------------
    if (RecoverSeeds) {
      float      chi2min(1.);
      DeltaSeed* closest_seed(nullptr);

      int nseeds = _data.listOfSeeds[Station].size();
      for (int i=0; i<nseeds; i++) {
        DeltaSeed* seed =  _data.deltaSeed(Station,i);
        if (seed->Good() == 0)                                          continue;
        if (seed->Used()     )                                          continue;
//-----------------------------------------------------------------------------
// one might need some safety here, but not the _maxDriftTime
//-----------------------------------------------------------------------------
        if (t0min > seed->T0Max())                                      continue;
        if (t0max < seed->T0Min())                                      continue;

        float dx   = xdelta-seed->CofM.x();
        float dy   = ydelta-seed->CofM.y();
        float chi2 = (dx*dx+dy*dy)/(_maxDxy*_maxDxy);
        if (chi2 >= chi2min)                                            continue;
        closest_seed = seed;
        chi2min      = chi2;
      }

      if (closest_seed) {
//-----------------------------------------------------------------------------
// the closest seed found, add it to the delta candidate and exit
// it is marked as associated with the delta candidate in DeltaCandidate::AddSeed
//-----------------------------------------------------------------------------
        Delta->AddSeed(closest_seed,Station);
        return 0;
      }
    }
//-----------------------------------------------------------------------------
// no seeds found, look for single hits
//-----------------------------------------------------------------------------
    DeltaSeed*  new_seed (nullptr);

    for (int face=0; face<kNFaces; face++) {
      for (int ip=0; ip<kNPanelsPerFace; ip++) {
        PanelZ_t* panelz = &_data.oTracker[Station][face][ip];
        double dphi      = Delta->phi-panelz->phi;

        if (dphi < -M_PI) dphi += 2*M_PI;
        if (dphi >  M_PI) dphi -= 2*M_PI;
        if (fabs(dphi) > M_PI/3)                                      continue;

        if (t0min > panelz->tmax)                                     continue;
        if (t0max < panelz->tmin)                                     continue;
//-----------------------------------------------------------------------------
// panel and Delta overlap in phi and time, loop over hits
//-----------------------------------------------------------------------------
        int nhits = panelz->fHitData.size();
        for (int h=0; h<nhits; ++h) {
          HitData_t* hd = &panelz->fHitData[h];
//-----------------------------------------------------------------------------
// don't skip hits already included into seeds - a two-hit stereo seed
// could be random
//-----------------------------------------------------------------------------
          if ((UseUsedHits == 0) and hd->Used())                      continue;
          float corr_time     = hd->fCorrTime;
//-----------------------------------------------------------------------------
// predicted time is the particle time, the drift time should be larger
//-----------------------------------------------------------------------------
          if (corr_time > t0max)                                      break;
          if (corr_time < t0min)                                      continue;

          const ComboHit*  ch = hd->fHit;
          double dx  = ch->pos().x()-xdelta;
          double dy  = ch->pos().y()-ydelta;
          double dw  = dx*panelz->wx+dy*panelz->wy; // distance along the wire
          double dxx = dx-panelz->wx*dw;
          double dyy = dy-panelz->wy*dw;

          double chi2_par  = (dw*dw)/hd->fSigW2;
          double chi2_perp = (dxx*dxx+dyy*dyy)/_sigmaR2;
          double chi2      = chi2_par + chi2_perp;

          if (chi2 >= _maxChi2Radial)                                 continue;
          if (hd->Used()) {
//-----------------------------------------------------------------------------
// hit is a part of a seed. if the seed has 2 or less hits, don't check the chi2
// - that could be a random overlap
// if the seed has 3 or more hits, check the chi2
//-----------------------------------------------------------------------------
            int nh = hd->fSeed->NHits();
            if ((nh >= 3) and (chi2 > hd->fChi2Min))                   continue;
          }
//-----------------------------------------------------------------------------
// new hit needs to be added, create a special 1-hit seed for that
// in most cases, expect this seed not to have the second hit, but it may
// such a seed has its own CofM undefined
// ** FIXME ..in principle, at this point may want to check if the hit was used...
//-----------------------------------------------------------------------------
          if (new_seed == nullptr) {
            hd->fChi2Min = chi2;
            new_seed = new DeltaSeed(-1,Station,face,hd,-1,nullptr);
          }
          else {
            if (face == new_seed->SFace(0)) {
//-----------------------------------------------------------------------------
// another close hit in the same panel, choose the best
//-----------------------------------------------------------------------------
              if (chi2 >= new_seed->HitData(face)->fChi2Min)          continue;
//-----------------------------------------------------------------------------
// new best hit in the same face
//-----------------------------------------------------------------------------
              hd->fChi2Min = chi2;
              new_seed->ReplaceFirstHit(hd);
            }
            else {
              if (_printErrors) {
                printf("ERROR in DeltaFinder::recoverStation: ");
                printf("station=%2i - shouldn\'t be getting here, printout of new_seed and hd follows\n",Station);
                printf("chi2_par, chi2_perp, chi2: %8.2f %8.2f %8.2f\n",chi2_par, chi2_perp, chi2);

                printf("DELTA:\n");
                _data.printDeltaCandidate(Delta,"");
                printf("SEED:\n");
                _data.printDeltaSeed(new_seed,"");
                printf("HIT:\n");
                _data.printHitData  (hd      ,"");
              }

              new_seed->hitData[face]    = hd;
              new_seed->fNFacesWithHits += 1;
              new_seed->fNHits          += 1;
              new_seed->fNStrawHits     += ch->nStrawHits();
            }

            if (corr_time < new_seed->fMinHitTime) new_seed->fMinHitTime = corr_time;
            if (corr_time > new_seed->fMaxHitTime) new_seed->fMaxHitTime = corr_time;
          }
        }
      }
      if (new_seed) new_seed->fFaceProcessed[face] = 1;
    }
//-----------------------------------------------------------------------------
// station is processed, see if anything has been found
// some parameters of seeds found in a recovery mode are not defined because
// there was no pre-seeding, for example
//-----------------------------------------------------------------------------
    int rc(0);
    if (new_seed) {
      new_seed->fIndex     = _data.listOfSeeds[Station].size();
      _data.listOfSeeds[Station].push_back(new_seed);
      _data.nseeds_per_station[Station] += 1;
      int face0 = new_seed->SFace(0);
      new_seed->HitData(face0)->fSeed = new_seed;

      Delta->AddSeed(new_seed,Station);
      rc = 1;
    }
                                        // return 1 if hits were added
    return rc;
  }

//-----------------------------------------------------------------------------
  void  DeltaFinder::runDeltaFinder() {

    for (int s=0; s<kNStations; ++s) {
      for (int f=0; f<kNFaces; ++f) {
        for (int p=0; p<3; ++p) {
          PanelZ_t* panelz = &_data.oTracker[s][f][p];
          panelz->fHitData.clear() ;
          panelz->tmin =  1.e6;
          panelz->tmax = -1.e6;
        }
      }
    }
    orderHits();
//-----------------------------------------------------------------------------
// loop over all stations and find delta seeds - 2-3-4 combo hit stubs
// a seed is always a stereo object
//-----------------------------------------------------------------------------
    findSeeds();
//-----------------------------------------------------------------------------
// connect seeds and create delta candidates
// at this stage, extend seeds to pick up single its in neighbor stations
// for single hits do not allo gaps
//-----------------------------------------------------------------------------
    connectSeeds();
//-----------------------------------------------------------------------------
// for existing delta candidates, pick up single gap-separated single hits
// no new candidates is created at this step
//-----------------------------------------------------------------------------
    recoverMissingHits();
//-----------------------------------------------------------------------------
// after recovery of missing hits, it is possible that some of delta candidates
// may need to be merged - try again
//-----------------------------------------------------------------------------
    mergeDeltaCandidates();
  }

//-----------------------------------------------------------------------------
// testOrderID & testdeOrderID not used in module, only were used to make sure OrderID and deOrderID worked as intended
//-----------------------------------------------------------------------------
  void DeltaFinder::testOrderID() {

    ChannelID x, o;

    for (int s=0; s<2; ++s) {
      for (int pl=0; pl<2; ++pl) {
        for (int pa=0; pa<6; ++pa) {
          for (int l=0; l<2; ++l) {
            x.Station = s;
            x.Plane   = pl;
            x.Panel   = pa;
            x.Layer   = l;
            orderID(&x, &o);
            printf(" testOrderID: Initial(station = %i, plane = %i, face = %i, panel = %i, layer = %i)",
                   x.Station, x.Plane, x.Face, x.Panel, x.Layer);
            printf("  Ordered(station = %i, plane = %i, face = %i, panel = %i, layer = %i)\n",
                   o.Station, o.Plane, o.Face, o.Panel, o.Layer);
          }
        }
      }
    }
  }

//-----------------------------------------------------------------------------
  void DeltaFinder::testdeOrderID() {

    ChannelID x, o;

    for (int s=0; s<2; ++s) {
      for (int f=0; f<4; ++f) {
        for (int pa=0; pa<3; ++pa) {
          for (int l=0; l<2; ++l) {

            o.Station          = s;
            o.Face             = f;
            if (f < 2) o.Plane = 0;
            else       o.Plane = 1;
            o.Panel            = pa;
            o.Layer            = l;

            deOrderID(&x, &o);

            printf(" testdeOrderID: Initial(station = %i, plane = %i, face = %i, panel = %i, layer = %i)",
                   x.Station,x.Plane,x.Face,x.Panel,x.Layer);
            printf("  Ordered(station = %i, plane = %i, face = %i, panel = %i, layer = %i)\n",
                   o.Station,o.Plane,o.Face,o.Panel,o.Layer);
          }
        }
      }
    }
  }

}

//-----------------------------------------------------------------------------
// magic that makes this class a module.
//-----------------------------------------------------------------------------
DEFINE_ART_MODULE(mu2e::DeltaFinder)
//-----------------------------------------------------------------------------
// done
//-----------------------------------------------------------------------------
