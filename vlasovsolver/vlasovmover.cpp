/*
 * This file is part of Vlasiator.
 * Copyright 2010-2016 Finnish Meteorological Institute
 *
 * For details of usage, see the COPYING file and read the "Rules of the Road"
 * at http://www.physics.helsinki.fi/vlasiator/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <stdint.h>

#ifdef _OPENMP
   #include <omp.h>
#endif
#include <zoltan.h>
#include <dccrg.hpp>
#include <phiprof.hpp>

#include "../spatial_cell.hpp"
#include "../vlasovmover.h"
#include "../grid.h"
#include "../definitions.h"
#include "../object_wrapper.h"
#include "../mpiconversion.h"

#include "cpu_moments.h"
#include "cpu_acc_semilag.hpp"
#include "cpu_trans_map.hpp"
#include "cpu_trans_map_amr.hpp"

using namespace std;
using namespace spatial_cell;

creal ZERO    = 0.0;
creal HALF    = 0.5;
creal FOURTH  = 1.0/4.0;
creal SIXTH   = 1.0/6.0;
creal ONE     = 1.0;
creal TWO     = 2.0;
creal EPSILON = 1.0e-25;

/** Propagates the distribution function in spatial space. 
    
    Based on SLICE-3D algorithm: Zerroukat, M., and T. Allen. "A
    three‐dimensional monotone and conservative semi‐Lagrangian scheme
    (SLICE‐3D) for transport problems." Quarterly Journal of the Royal
    Meteorological Society 138.667 (2012): 1640-1651.
  
 */
void calculateSpatialTranslation(
//         dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,
//         const vector<CellID>& localCells,
//         const vector<CellID>& local_propagated_cells,
//         const vector<CellID>& local_target_cells,
//         const vector<CellID>& remoteTargetCellsx,
//         const vector<CellID>& remoteTargetCellsy,
//         const vector<CellID>& remoteTargetCellsz,
//         creal dt,
//         const uint popID) {
        dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,

        const vector<CellID>& local_propagated_cells_x,
        const vector<CellID>& local_propagated_cells_y,
        const vector<CellID>& local_propagated_cells_z,
	const vector<CellID>& remoteTargetCellsx,
	const vector<CellID>& remoteTargetCellsy,
	const vector<CellID>& remoteTargetCellsz,
        vector<uint>& nPencils,
        creal dt,
        const uint popID,
        Real &time
) {

    int trans_timer;
    //bool localTargetGridGenerated = false;
    
    double t1;
    
    int myRank;
    MPI_Comm_rank(MPI_COMM_WORLD,&myRank);

    /** Now restructured: Update all ghost cells including corners,
	then perform piecewise translation for all local cells
	and also some required ghost cells
    **/

    std::cerr<<"MPI "<<myRank<<" dt "<<dt<<" time "<<time<<" pop "<<popID<<" npencils "<<nPencils.size()<<std::endl;
    std::cerr<<"lengths "<<local_propagated_cells_x.size()
	     <<" "<<local_propagated_cells_y.size()
	     <<" "<<local_propagated_cells_z.size()
	     <<" "<<remoteTargetCellsx.size()
	     <<" "<<remoteTargetCellsy.size()
	     <<" "<<remoteTargetCellsz.size()<<std::endl;
    trans_timer=phiprof::initializeTimer("transfer-stencil-data-all","MPI");
    phiprof::start(trans_timer);
    //SpatialCell::set_mpi_transfer_type(Transfer::VEL_BLOCK_DATA);
    SpatialCell::set_mpi_transfer_type(Transfer::ALL_SPATIAL_DATA);
    // Field solver neighborhood is simple
    //mpiGrid.update_copies_of_remote_neighbors(FIELD_SOLVER_NEIGHBORHOOD_ID);
    //mpiGrid.update_copies_of_remote_neighbors(SYSBOUNDARIES_NEIGHBORHOOD_ID);
    //mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL);
    std::cerr<<"BARRIER "<<myRank<<std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    // std::cerr<<"Update FULL_NEIGHBORHOOD "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(FULL_NEIGHBORHOOD_ID);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // std::cerr<<"Update NEAREST_NEIGHBORHOOD_ID "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(NEAREST_NEIGHBORHOOD_ID);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // MPI_Barrier(MPI_COMM_WORLD);

    // std::cerr<<"Update VLASOV_ALLPROPLOCAL_MEDIUM "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL_MEDIUM1);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // MPI_Barrier(MPI_COMM_WORLD);

    // std::cerr<<"Update VLASOV_ALLPROPLOCAL_MEDIUM2 "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL_MEDIUM2);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // MPI_Barrier(MPI_COMM_WORLD);

    // std::cerr<<"Update VLASOV_ALLPROPLOCAL_MEDIUM3 "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL_MEDIUM3);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // MPI_Barrier(MPI_COMM_WORLD);

    // std::cerr<<"Update VLASOV_ALLPROPLOCAL_MEDIUM4 "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL_MEDIUM4);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    // MPI_Barrier(MPI_COMM_WORLD);
    // std::cerr<<"Update VLASOV_ALLPROPLOCAL_OUTER "<<myRank<<std::endl;
    // mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL_OUTER);
    // std::cerr<<"DONE "<<myRank<<std::endl;

    std::cerr<<"Update VLASOV_ALLPROPLOCAL "<<myRank<<std::endl;
    mpiGrid.update_copies_of_remote_neighbors(VLASOV_ALLPROPLOCAL);
    std::cerr<<"DONE "<<myRank<<std::endl;

    phiprof::stop(trans_timer);
    
    std::cerr<<"Z"<<std::endl;
    // ------------- SLICE - map dist function in Z --------------- //
    if(P::zcells_ini > 1){

      t1 = MPI_Wtime();
      phiprof::start("compute-mapping-z");
      if(P::amrMaxSpatialRefLevel == 0) {
         trans_map_1d(mpiGrid,local_propagated_cells_z, remoteTargetCellsz, 2, dt,popID); // map along z//
      } else {
	trans_map_1d_amr(mpiGrid,local_propagated_cells_z, remoteTargetCellsz, nPencils, 2, dt,popID); // map along z//
      }
      phiprof::stop("compute-mapping-z");
      time += MPI_Wtime() - t1;
   }

//   bt=phiprof::initializeTimer("barrier-trans-pre-x","Barriers","MPI");
//   phiprof::start(bt);
//   MPI_Barrier(MPI_COMM_WORLD);
//   phiprof::stop(bt);
   
   // ------------- SLICE - map dist function in X --------------- //
    std::cerr<<"X"<<std::endl;
   if(P::xcells_ini > 1){
      t1 = MPI_Wtime();
      phiprof::start("compute-mapping-x");
      if(P::amrMaxSpatialRefLevel == 0) {
         trans_map_1d(mpiGrid,local_propagated_cells_x, remoteTargetCellsx, 0,dt,popID); // map along x//
      } else {
         trans_map_1d_amr(mpiGrid,local_propagated_cells_x, remoteTargetCellsx, nPencils, 0, dt,popID); // map along x//
      }
      phiprof::stop("compute-mapping-x");
      time += MPI_Wtime() - t1;
   }

//   bt=phiprof::initializeTimer("barrier-trans-pre-y","Barriers","MPI");
//   phiprof::start(bt);
//   MPI_Barrier(MPI_COMM_WORLD);
//   phiprof::stop(bt);

   // ------------- SLICE - map dist function in Y --------------- //
    std::cerr<<"Y"<<std::endl;
   if(P::ycells_ini > 1) {
      t1 = MPI_Wtime();
      phiprof::start("compute-mapping-y");
      if(P::amrMaxSpatialRefLevel == 0) {
         trans_map_1d(mpiGrid,local_propagated_cells_y, remoteTargetCellsy, 1,dt,popID); // map along y//
      } else {
         trans_map_1d_amr(mpiGrid,local_propagated_cells_y, remoteTargetCellsy, nPencils, 1,dt,popID); // map along y//      
      }
      phiprof::stop("compute-mapping-y");
      time += MPI_Wtime() - t1;
   }

//   bt=phiprof::initializeTimer("barrier-trans-post-trans","Barriers","MPI");
//   phiprof::start(bt);
//   MPI_Barrier(MPI_COMM_WORLD);
//   phiprof::stop(bt);

   // MPI_Barrier(MPI_COMM_WORLD);
   // bailout(true, "", __FILE__, __LINE__);
}

/*!
  
  Propagates the distribution function in spatial space. 
  
  Based on SLICE-3D algorithm: Zerroukat, M., and T. Allen. "A
  three‐dimensional monotone and conservative semi‐Lagrangian scheme
  (SLICE‐3D) for transport problems." Quarterly Journal of the Royal
  Meteorological Society 138.667 (2012): 1640-1651.

*/
void calculateSpatialTranslation(
        dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,
        creal dt) {
   typedef Parameters P;
   
   phiprof::start("semilag-trans");
   
   double t1 = MPI_Wtime();

   const vector<CellID>& localCells = getLocalCells();
    vector<CellID> remoteTargetCellsx;
    vector<CellID> remoteTargetCellsy;
    vector<CellID> remoteTargetCellsz;
//    vector<CellID> local_propagated_cells;
   vector<CellID> local_propagated_cells_x;
   vector<CellID> local_propagated_cells_y;
   vector<CellID> local_propagated_cells_z;
   vector<uint> nPencils;
   Real time=0.0;
   
   // If dt=0 we are either initializing or distribution functions are not translated. 
   // In both cases go to the end of this function and calculate the moments.
   if (dt == 0.0) goto momentCalculation;
   
   phiprof::start("compute_cell_lists");
   remoteTargetCellsx = mpiGrid.get_remote_cells_on_process_boundary(VLASOV_ALLPROPLOCAL_X);
   remoteTargetCellsy = mpiGrid.get_remote_cells_on_process_boundary(VLASOV_ALLPROPLOCAL_Y);
   remoteTargetCellsz = mpiGrid.get_remote_cells_on_process_boundary(VLASOV_ALLPROPLOCAL_Z);
   
   // Figure out which cells (+ ghost cells) need to be translated in each
   // direction for correct results
   // Translation order (dimensions) is 1: z 2: x 3: y
   /*
     for final y-translation, we need to have translate y for cells y\pm 1. 
     For this, correct values need to exist in cells y\pm(1+VLASOV_STENCIL)
     remote target cells need to be y\pm2

     Thus, x-translation must happen for cells y\pm(1+VLASOV_STENCIL) x\pm1     
     For this, correct values need to exist in cells y\pm(1+VLASOV_STENCIL) x\pm(1+VLASOV_STENCIL)
     remote target cells need to be y\pm(1+VLASOV_STENCIL) x\pm2

     Thus, z-translation must happen for cells y\pm(1+VLASOV_STENCIL) x\pm(1+VLASOV_STENCIL) z\pm1
     For this, correct values need to exist in cells y\pm(1+VLASOV_STENCIL) x\pm(1+VLASOV_STENCIL) z\pm(1+VLASOV_STENCIL) (but the z\pm(1+VLASOV_STENCIL) already is ok)
     and remote target cells need to be y\pm(1+VLASOV_STENCIL) x\pm(1+VLASOV_STENCIL) z\pm2
    */
   
   // result independent of particle species.
   for (size_t c=0; c<localCells.size(); ++c) {
      if (do_translate_cell(mpiGrid[localCells[c]])) {
	 local_propagated_cells_y.push_back(localCells[c]);
      }
      // Add first ghost cells in y direction to get rid of remote updates
      const auto faceNbrs = mpiGrid.get_face_neighbors_of(localCells[c]);      
      for (const auto nbr : faceNbrs) {
	 if (nbr.first==NULL) continue;
	 if (mpiGrid.is_local(nbr.first)) continue;
	 if (!do_translate_cell(mpiGrid[nbr.first])) continue;
	 if ((abs(nbr.second) == 2) && (std::find(local_propagated_cells_y.begin(),
						  local_propagated_cells_y.end(), nbr.first) == local_propagated_cells_y.end())) {
	   local_propagated_cells_y.push_back(nbr.first);
	 }
      }      
   }  
   
   local_propagated_cells_x = local_propagated_cells_y; // is a deep copy
   // Add VLASOV_STENCIL cells in the y-direction
   for (uint vs=0; vs<VLASOV_STENCIL_WIDTH; ++vs) {
     for (size_t c=0; c<local_propagated_cells_x.size(); ++c) {
       const auto faceNbrs = mpiGrid.get_face_neighbors_of(local_propagated_cells_x[c]);      
       for (const auto nbr : faceNbrs) {
	 if (nbr.first==NULL) continue;
	 if (mpiGrid.is_local(nbr.first)) continue;
	 if (!do_translate_cell(mpiGrid[nbr.first])) {
	   //std::cerr<<"X_y "<<vs<<" "<< local_propagated_cells_x[c]<<" do not translate "<<nbr.second<<" nbr  "<<nbr.first<<std::endl; //MCB
	   continue;
	 }
	 if ((abs(nbr.second) == 2) && (std::find(local_propagated_cells_x.begin(),
						  local_propagated_cells_x.end(), nbr.first) == local_propagated_cells_x.end())) {
	   local_propagated_cells_x.push_back(nbr.first);
	   //std::cerr<<"X_y "<<vs<<" "<< local_propagated_cells_x[c]<<" add "<<nbr.second<<" nbr "<<nbr.first<<std::endl; //MCB
	 }
       }
     }
   }
   // Add \pm 1 cells in x-direction
   for (size_t c=0; c<local_propagated_cells_x.size(); ++c) {
     const auto faceNbrs = mpiGrid.get_face_neighbors_of(local_propagated_cells_x[c]);      
     for (const auto nbr : faceNbrs) {
       if (nbr.first==NULL) continue;
       if (mpiGrid.is_local(nbr.first)) continue;
       if (!do_translate_cell(mpiGrid[nbr.first])) continue;
       if ((abs(nbr.second) == 1) && (std::find(local_propagated_cells_x.begin(),
						local_propagated_cells_x.end(), nbr.first) == local_propagated_cells_x.end())) {
	 local_propagated_cells_x.push_back(nbr.first);
	 //std::cerr<<"X_x "<< local_propagated_cells_x[c]<<" add "<<nbr.second<<" nbr "<<nbr.first<<std::endl; //MCB
       }
     }
   }
     
   local_propagated_cells_z = local_propagated_cells_x;
   // Add VLASOV_STENCIL cells in the x-direction
   for (uint vs=0; vs<VLASOV_STENCIL_WIDTH; ++vs) {
     for (size_t c=0; c<local_propagated_cells_z.size(); ++c) {
       const auto faceNbrs = mpiGrid.get_face_neighbors_of(local_propagated_cells_z[c]);      
       for (const auto nbr : faceNbrs) {
	 if (nbr.first==NULL) continue;
	 if (mpiGrid.is_local(nbr.first)) continue;
	 if (!do_translate_cell(mpiGrid[nbr.first])) continue;
	 if ((abs(nbr.second) == 1) && (std::find(local_propagated_cells_z.begin(),
						  local_propagated_cells_z.end(), nbr.first) == local_propagated_cells_z.end())) {
	   local_propagated_cells_z.push_back(nbr.first);
	 }
       }
     }
   }
   // Add \pm 1 cells in z-direction
   for (size_t c=0; c<local_propagated_cells_z.size(); ++c) {
     const auto faceNbrs = mpiGrid.get_face_neighbors_of(local_propagated_cells_z[c]);      
     for (const auto nbr : faceNbrs) {
       if (nbr.first==NULL) continue;
       if (mpiGrid.is_local(nbr.first)) continue;
       if (!do_translate_cell(mpiGrid[nbr.first])) continue;
       if ((abs(nbr.second) == 3) && (std::find(local_propagated_cells_z.begin(),
						local_propagated_cells_z.end(), nbr.first) == local_propagated_cells_z.end())) {
	 local_propagated_cells_z.push_back(nbr.first);
       }
     }
   }

   if (P::prepareForRebalance == true && P::amrMaxSpatialRefLevel != 0) {
      // One more element to count the sums
      for (size_t c=0; c<local_propagated_cells_z.size()+1; c++) {
         nPencils.push_back(0);
      }
   }
   phiprof::stop("compute_cell_lists");
   
   // Translate all particle species
   for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
      string profName = "translate "+getObjectWrapper().particleSpecies[popID].name;
      phiprof::start(profName);
      SpatialCell::setCommunicatedSpecies(popID);
      //      std::cout << "I am at line " << __LINE__ << " of " << __FILE__ << std::endl;
//       calculateSpatialTranslation(mpiGrid,localCells,local_propagated_cells,
//                                   local_target_cells,remoteTargetCellsx,remoteTargetCellsy,
//                                   remoteTargetCellsz,dt,popID);
      calculateSpatialTranslation(
				  mpiGrid,
				  local_propagated_cells_x,
				  local_propagated_cells_y,
				  local_propagated_cells_z,
				  remoteTargetCellsx,
				  remoteTargetCellsy,
				  remoteTargetCellsz,
				  nPencils,
				  dt,
				  popID,
				  time
				  );
      phiprof::stop(profName);
   }


   if (Parameters::prepareForRebalance == true) {
     for (size_t c=0; c<localCells.size(); ++c) {
       for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
	 mpiGrid[localCells[c]]->parameters[CellParams::LBWEIGHTCOUNTER] += mpiGrid[localCells[c]]->get_number_of_velocity_blocks(popID);
       }
     }
   }
   /*
   if (Parameters::prepareForRebalance == true) {
      if(P::amrMaxSpatialRefLevel == 0) {
//          const double deltat = (MPI_Wtime() - t1) / local_propagated_cells.size();
         for (size_t c=0; c<localCells.size(); ++c) {
//            mpiGrid[localCells[c]]->parameters[CellParams::LBWEIGHTCOUNTER] += time / localCells.size();
            for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
               mpiGrid[localCells[c]]->parameters[CellParams::LBWEIGHTCOUNTER] += mpiGrid[localCells[c]]->get_number_of_velocity_blocks(popID);
            }
         }
      } else {
//          const double deltat = MPI_Wtime() - t1;
         for (size_t c=0; c<local_propagated_cells_z.size(); ++c) {
            Real counter = 0;
            for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
               counter += mpiGrid[local_propagated_cells[c]]->get_number_of_velocity_blocks(popID);
            }
            mpiGrid[local_propagated_cells[c]]->parameters[CellParams::LBWEIGHTCOUNTER] += nPencils[c] * counter;
//            mpiGrid[localCells[c]]->parameters[CellParams::LBWEIGHTCOUNTER] += time / localCells.size();
         }
      } 
   }*/
   
   // Mapping complete, update moments and maximum dt limits //
momentCalculation:
   calculateMoments_R(mpiGrid,localCells,true);
   
   phiprof::stop("semilag-trans");
}

/*
  --------------------------------------------------
  Acceleration (velocity space propagation)
  --------------------------------------------------
*/

/** Accelerate the given population to new time t+dt.
 * This function is AMR safe.
 * @param popID Particle population ID.
 * @param globalMaxSubcycles Number of times acceleration is subcycled.
 * @param step The current subcycle step.
 * @param mpiGrid Parallel grid library.
 * @param propagatedCells List of cells in which the population is accelerated.
 * @param dt Timestep.*/
void calculateAcceleration(const uint popID,const uint globalMaxSubcycles,const uint step,
                           dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,
                           const std::vector<CellID>& propagatedCells,
                           const Real& dt) {
   // Set active population
   SpatialCell::setCommunicatedSpecies(popID);
   
   // Calculate velocity moments, these are needed to 
   // calculate the transforms used in the accelerations.
   // Calculated moments are stored in the "_V" variables.
   calculateMoments_V(mpiGrid, propagatedCells, false);

   // Semi-Lagrangian acceleration for those cells which are subcycled
   #pragma omp parallel for schedule(dynamic,1)
   for (size_t c=0; c<propagatedCells.size(); ++c) {
      const CellID cellID = propagatedCells[c];
      const Real maxVdt = mpiGrid[cellID]->get_max_v_dt(popID);
      
      //compute subcycle dt. The length is maxVdt on all steps
      //except the last one. This is to keep the neighboring
      //spatial cells in sync, so that two neighboring cells with
      //different number of subcycles have similar timesteps,
      //except that one takes an additional short step. This keeps
      //spatial block neighbors as much in sync as possible for
      //adjust blocks.
      Real subcycleDt;
      if( (step + 1) * maxVdt > fabs(dt)) {
	 subcycleDt = max(fabs(dt) - step * maxVdt, 0.0);
      } else{
         subcycleDt = maxVdt;
      }
      if (dt<0) subcycleDt = -subcycleDt;
      
      //generate pseudo-random order which is always the same irrespective of parallelization, restarts, etc.
      char rngStateBuffer[256];
      random_data rngDataBuffer;

      // set seed, initialise generator and get value. The order is the same
      // for all cells, but varies with timestep.
      memset(&(rngDataBuffer), 0, sizeof(rngDataBuffer));
      #ifdef _AIX
         initstate_r(P::tstep, &(rngStateBuffer[0]), 256, NULL, &(rngDataBuffer));
         int64_t rndInt;
         random_r(&rndInt, &rngDataBuffer);
      #else
         initstate_r(P::tstep, &(rngStateBuffer[0]), 256, &(rngDataBuffer));
         int32_t rndInt;
         random_r(&rngDataBuffer, &rndInt);
      #endif
         
      uint map_order=rndInt%3;
      phiprof::start("cell-semilag-acc");
      cpu_accelerate_cell(mpiGrid[cellID],popID,map_order,subcycleDt);
      phiprof::stop("cell-semilag-acc");
   }

   //global adjust after each subcycle to keep number of blocks managable. Even the ones not
   //accelerating anyore participate. It is important to keep
   //the spatial dimension to make sure that we do not loose
   //stuff streaming in from other cells, perhaps not connected
   //to the existing distribution function in the cell.
   //- All cells update and communicate their lists of content blocks
   //- Only cells which were accerelated on this step need to be adjusted (blocks removed or added).
   //- Not done here on last step (done after loop)
   if(step < (globalMaxSubcycles - 1)) adjustVelocityBlocks(mpiGrid, propagatedCells, false, popID);
}

/** Accelerate all particle populations to new time t+dt. 
 * This function is AMR safe.
 * @param mpiGrid Parallel grid library.
 * @param dt Time step.*/
void calculateAcceleration(dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,
                           Real dt
                          ) {    
   typedef Parameters P;
   const vector<CellID>& cells = getLocalCells();

   int myRank;
   MPI_Comm_rank(MPI_COMM_WORLD,&myRank);
   
   if (dt == 0.0 && P::tstep > 0) {
      
      // Even if acceleration is turned off we need to adjust velocity blocks 
      // because the boundary conditions may have altered the velocity space, 
      // and to update changes in no-content blocks during translation.
      for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
         adjustVelocityBlocks(mpiGrid, cells, true, popID);
      }

      goto momentCalculation;
   }
   phiprof::start("semilag-acc");
    
   // Accelerate all particle species
    for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
       int maxSubcycles=0;
       int globalMaxSubcycles;

       // Set active population
       SpatialCell::setCommunicatedSpecies(popID);
       
       // Iterate through all local cells and collect cells to propagate.
       // Ghost cells (spatial cells at the boundary of the simulation 
       // volume) do not need to be propagated:
       vector<CellID> propagatedCells;
       for (size_t c=0; c<cells.size(); ++c) {
          SpatialCell* SC = mpiGrid[cells[c]];
          const vmesh::VelocityMesh<vmesh::GlobalID,vmesh::LocalID>& vmesh = SC->get_velocity_mesh(popID);
          // disregard boundary cells, in preparation for acceleration 
          if (SC->sysBoundaryFlag == sysboundarytype::NOT_SYSBOUNDARY ) {
             if(vmesh.size() != 0){
                //do not propagate spatial cells with no blocks
                propagatedCells.push_back(cells[c]);
             }
             //prepare for acceleration, updates max dt for each cell, it
             //needs to be set to somthing sensible for _all_ cells, even if
             //they are not propagated
             prepareAccelerateCell(SC, popID);
             //update max subcycles for all cells in this process
             maxSubcycles = max((int)getAccelerationSubcycles(SC, dt, popID), maxSubcycles);
             spatial_cell::Population& pop = SC->get_population(popID);
             pop.ACCSUBCYCLES = getAccelerationSubcycles(SC, dt, popID);
          }
       }

       // Compute global maximum for number of subcycles
       MPI_Allreduce(&maxSubcycles, &globalMaxSubcycles, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
       
       // substep global max times
       for(uint step=0; step<(uint)globalMaxSubcycles; ++step) {
          if(step > 0) {
             // prune list of cells to propagate to only contained those which are now subcycled
             vector<CellID> temp;
             for (const auto& cell: propagatedCells) {
                if (step < getAccelerationSubcycles(mpiGrid[cell], dt, popID) ) {
                   temp.push_back(cell);
                }
             }
             
             propagatedCells.swap(temp);
          }
          // Accelerate population over one subcycle step
          calculateAcceleration(popID,(uint)globalMaxSubcycles,step,mpiGrid,propagatedCells,dt);
       } // for-loop over acceleration substeps
       
       // final adjust for all cells, also fixing remote cells.
       adjustVelocityBlocks(mpiGrid, cells, true, popID);
    } // for-loop over particle species

    phiprof::stop("semilag-acc");

   // Recalculate "_V" velocity moments
momentCalculation:
   calculateMoments_V(mpiGrid,cells,true);

   // Set CellParams::MAXVDT to be the minimum dt of all per-species values
   #pragma omp parallel for
   for (size_t c=0; c<cells.size(); ++c) {
      SpatialCell* cell = mpiGrid[cells[c]];
      cell->parameters[CellParams::MAXVDT] = numeric_limits<Real>::max();
      for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
         cell->parameters[CellParams::MAXVDT]
           = min(cell->get_max_v_dt(popID), cell->parameters[CellParams::MAXVDT]);
      }
   }
}

/*--------------------------------------------------
  Functions for computing moments
  --------------------------------------------------*/

void calculateInterpolatedVelocityMoments(
   dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid,
   const int cp_rhom,
   const int cp_vx,
   const int cp_vy,
   const int cp_vz,
   const int cp_rhoq,
   const int cp_p11,
   const int cp_p22,
   const int cp_p33
) {
   const vector<CellID>& cells = getLocalCells();
   
   //Iterate through all local cells
    #pragma omp parallel for
   for (size_t c=0; c<cells.size(); ++c) {
      const CellID cellID = cells[c];
      SpatialCell* SC = mpiGrid[cellID];
      SC->parameters[cp_rhom  ] = 0.5* ( SC->parameters[CellParams::RHOM_R] + SC->parameters[CellParams::RHOM_V] );
      SC->parameters[cp_vx] = 0.5* ( SC->parameters[CellParams::VX_R] + SC->parameters[CellParams::VX_V] );
      SC->parameters[cp_vy] = 0.5* ( SC->parameters[CellParams::VY_R] + SC->parameters[CellParams::VY_V] );
      SC->parameters[cp_vz] = 0.5* ( SC->parameters[CellParams::VZ_R] + SC->parameters[CellParams::VZ_V] );
      SC->parameters[cp_rhoq  ] = 0.5* ( SC->parameters[CellParams::RHOQ_R] + SC->parameters[CellParams::RHOQ_V] );
      SC->parameters[cp_p11]   = 0.5* ( SC->parameters[CellParams::P_11_R] + SC->parameters[CellParams::P_11_V] );
      SC->parameters[cp_p22]   = 0.5* ( SC->parameters[CellParams::P_22_R] + SC->parameters[CellParams::P_22_V] );
      SC->parameters[cp_p33]   = 0.5* ( SC->parameters[CellParams::P_33_R] + SC->parameters[CellParams::P_33_V] );

      for (uint popID=0; popID<getObjectWrapper().particleSpecies.size(); ++popID) {
         spatial_cell::Population& pop = SC->get_population(popID);
         pop.RHO = 0.5 * ( pop.RHO_R + pop.RHO_V );
         for(int i=0; i<3; i++) {
            pop.V[i] = 0.5 * ( pop.V_R[i] + pop.V_V[i] );
            pop.P[i]    = 0.5 * ( pop.P_R[i] + pop.P_V[i] );
         }
      }
   }
}

void calculateInitialVelocityMoments(dccrg::Dccrg<SpatialCell,dccrg::Cartesian_Geometry>& mpiGrid) {
   const vector<CellID>& cells = getLocalCells();
   phiprof::start("Calculate moments");

   // Iterate through all local cells (incl. system boundary cells):
   #pragma omp parallel for
   for (size_t c=0; c<cells.size(); ++c) {
      const CellID cellID = cells[c];
      SpatialCell* SC = mpiGrid[cellID];
      calculateCellMoments(SC,true);

      // WARNING the following is sane as this function is only called by initializeGrid.
      // We need initialized _DT2 values for the dt=0 field propagation done in the beginning.
      // Later these will be set properly.
      SC->parameters[CellParams::RHOM_DT2] = SC->parameters[CellParams::RHOM];
      SC->parameters[CellParams::VX_DT2] = SC->parameters[CellParams::VX];
      SC->parameters[CellParams::VY_DT2] = SC->parameters[CellParams::VY];
      SC->parameters[CellParams::VZ_DT2] = SC->parameters[CellParams::VZ];
      SC->parameters[CellParams::RHOQ_DT2] = SC->parameters[CellParams::RHOQ];
      SC->parameters[CellParams::P_11_DT2] = SC->parameters[CellParams::P_11];
      SC->parameters[CellParams::P_22_DT2] = SC->parameters[CellParams::P_22];
      SC->parameters[CellParams::P_33_DT2] = SC->parameters[CellParams::P_33];
   } // for-loop over spatial cells
   phiprof::stop("Calculate moments"); 
}
