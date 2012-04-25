// Free function to create world mother volume and partly fill it with
// dirt around the formal hall box.
//
// $Id: constructWorldVolume.cc,v 1.7 2012/04/25 18:19:14 gandr Exp $
// $Author: gandr $
// $Date: 2012/04/25 18:19:14 $
//
// Original author KLG based on Mu2eWorld constructDirt
// Updated by Andrei Gaponenko.

// Mu2e includes.
#include "Mu2eG4/inc/constructWorldVolume.hh"
#include "G4Helper/inc/VolumeInfo.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/WorldG4.hh"
#include "Mu2eBuildingGeom/inc/BuildingBasics.hh"
#include "Mu2eBuildingGeom/inc/Mu2eBuilding.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "Mu2eG4/inc/MaterialFinder.hh"
#include "Mu2eG4/inc/nestBox.hh"
#include "Mu2eG4/inc/finishNesting.hh"

// G4 includes
#include "G4Material.hh"
#include "G4Color.hh"
#include "G4Box.hh"

using namespace std;

namespace mu2e {

  VolumeInfo constructWorldVolume(const SimpleConfig &config) {
    // A helper class.
    MaterialFinder materialFinder(config);

    // Dimensions and material of the world.
    G4Material* worldMaterial = materialFinder.get("world.materialName");
    G4Material* dirtMaterial = MaterialFinder(config).get("dirt.overburdenMaterialName");

    const bool worldBoxVisible = config.getBool("world.boxVisible");
    const bool worldBoxSolid   = config.getBool("world.boxSolid");

    const bool dirtVisible    = config.getBool("world.dirt.visible");
    const bool dirtSolid      = config.getBool("world.dirt.solid");

    const bool forceAuxEdgeVisible = config.getBool("g4.forceAuxEdgeVisible");
    const bool doSurfaceCheck = config.getBool("g4.doSurfaceCheck");
    const bool placePV             = true;

    GeomHandle<WorldG4> world;
    GeomHandle<BuildingBasics> basics;
    GeomHandle<Mu2eBuilding> building;

    VolumeInfo worldInfo = nestBox("World", world->halfLengths(),
                                   worldMaterial, 0, G4ThreeVector(),
                                   0,
                                   0, worldBoxVisible, G4Colour::Red(), worldBoxSolid,
                                   forceAuxEdgeVisible, placePV, false);

    //----------------------------------------------------------------
    // Here we create dirt slabs and place them in the World volume
    // around the formal "hall" box.

    // Dirt slab at the bottom
    {
      std::vector<double> hs(world->halfLengths());
      // world coordinates
      const double dirtYmin = -world->halfLengths()[1];
      const double dirtYmax = -world->hallFormalHalfSize()[1] + world->hallFormalCenterInWorld().y();
      hs[1] = (dirtYmax - dirtYmin)/2;

      CLHEP::Hep3Vector centerInWorld(3);
      centerInWorld[0] = 0.;
      centerInWorld[1] = (dirtYmax + dirtYmin)/2;
      centerInWorld[2] = 0;

      nestBox("worldDirtBottom", hs, dirtMaterial, 0, centerInWorld,
              worldInfo,
              0, dirtVisible, G4Color::Magenta(), dirtSolid,
              forceAuxEdgeVisible, placePV, doSurfaceCheck);
    }

    // The height parameters are common to all 4 side slabs
    const double dirtYmin = -world->hallFormalHalfSize()[1] + world->hallFormalCenterInWorld().y();
    const double dirtYmax = basics->yFlatEarth() + world->mu2eOriginInWorld().y();
    const double dirtCenterY = (dirtYmax + dirtYmin)/2;
    const double dirtHalfSizeY = (dirtYmax - dirtYmin)/2;

    // NW: slab covering the North corner and extending to the West
    {
      const double xmax = world->halfLengths()[0];
      const double xmin = world->hallFormalCenterInWorld().x() - world->hallFormalHalfSize()[0];
      const double zmin = -world->halfLengths()[2];
      const double zmax = world->hallFormalCenterInWorld().z() - world->hallFormalHalfSize()[2];
      std::vector<double> hs(3);
      hs[0] = (xmax - xmin)/2;
      hs[1] = dirtHalfSizeY;
      hs[2] = (zmax - zmin)/2;

      nestBox("worldDirtNW", hs, dirtMaterial, 0,
              CLHEP::Hep3Vector((xmax+xmin)/2, dirtCenterY, (zmax+zmin)/2),
              worldInfo,
              0, dirtVisible, G4Color::Magenta(), dirtSolid,
              forceAuxEdgeVisible, placePV, doSurfaceCheck);
    }

    // SW: slab covering the West corner and extending to the South
    {
      const double xmin = -world->halfLengths()[0];
      const double xmax =  world->hallFormalCenterInWorld().x() - world->hallFormalHalfSize()[0];
      const double zmin = -world->halfLengths()[2];
      const double zmax = world->hallFormalCenterInWorld().z() + world->hallFormalHalfSize()[2];
      std::vector<double> hs(3);
      hs[0] = (xmax - xmin)/2;
      hs[1] = dirtHalfSizeY;
      hs[2] = (zmax - zmin)/2;

      nestBox("worldDirtSW", hs, dirtMaterial, 0,
              CLHEP::Hep3Vector((xmax+xmin)/2, dirtCenterY, (zmax+zmin)/2),
              worldInfo,
              0, dirtVisible, G4Color::Magenta(), dirtSolid,
              forceAuxEdgeVisible, placePV, doSurfaceCheck);
    }

    // SE: slab covering the South corner and extending to the East
    {
      const double xmin = -world->halfLengths()[0];
      const double xmax =  world->hallFormalCenterInWorld().x() + world->hallFormalHalfSize()[0];
      const double zmax = +world->halfLengths()[2];
      const double zmin =  world->hallFormalCenterInWorld().z() + world->hallFormalHalfSize()[2];
      std::vector<double> hs(3);
      hs[0] = (xmax - xmin)/2;
      hs[1] = dirtHalfSizeY;
      hs[2] = (zmax - zmin)/2;

      nestBox("worldDirtSE", hs, dirtMaterial, 0,
              CLHEP::Hep3Vector((xmax+xmin)/2, dirtCenterY, (zmax+zmin)/2),
              worldInfo,
              0, dirtVisible, G4Color::Magenta(), dirtSolid,
              forceAuxEdgeVisible, placePV, doSurfaceCheck);
    }

    // NE: slab covering the East corner and extending to the North
    {
      const double xmax = +world->halfLengths()[0];
      const double xmin =  world->hallFormalCenterInWorld().x() + world->hallFormalHalfSize()[0];
      const double zmax = +world->halfLengths()[2];
      const double zmin =  world->hallFormalCenterInWorld().z() - world->hallFormalHalfSize()[2];
      std::vector<double> hs(3);
      hs[0] = (xmax - xmin)/2;
      hs[1] = dirtHalfSizeY;
      hs[2] = (zmax - zmin)/2;

      nestBox("worldDirtNE", hs, dirtMaterial, 0,
              CLHEP::Hep3Vector((xmax+xmin)/2, dirtCenterY, (zmax+zmin)/2),
              worldInfo,
              0, dirtVisible, G4Color::Magenta(), dirtSolid,
              forceAuxEdgeVisible, placePV, doSurfaceCheck);
    }

    //----------------------------------------------------------------
    return worldInfo;

  } // constructWorldVolume()

} // namespace mu2e
