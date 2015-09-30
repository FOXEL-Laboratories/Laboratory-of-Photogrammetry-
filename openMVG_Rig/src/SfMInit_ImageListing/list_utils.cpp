/*
 * list_utils
 *
 * Copyright (c) 2014-2015 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 *
 * Author(s):
 *
 *      Stéphane Flotron <s.flotron@foxel.ch>
 *
 *
 * This file is part of the FOXEL project <http://foxel.ch>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 *      You are required to preserve legal notices and author attributions in
 *      that material or in the Appropriate Legal Notices displayed by works
 *      containing it.
 *
 *      You are required to attribute the work as explained in the "Usage and
 *      Attribution" section of <http://foxel.ch/license>.
 */

#include <fastcal-all.h>
#include <tools.hpp>
#include <list_utils.hpp>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>
#include "json.hpp"
#include <openMVG/sfm/sfm.hpp>
#include <openMVG/stl/split.hpp>
#include <progress/progress.hpp>
#include <stlplus3/filesystemSimplified/file_system.hpp>

#undef OPENMVG_USE_OPENMP

using namespace std;

/*******************************************************************************
* verify that given timestamp range is valid
*
********************************************************************************
*/

/*! \brief Check input data validity
*
* This function checks that given timestamp range is valid
*
* \param sTimestampLow  Lower bound for timestamp value
* \param sTimestampUp   Upper bound for timestampe value
*
* \return bool value indicating if the timestamp range is valid or not
*/

bool isRangeValid(  const std::string& sTimestampLow,
                    const std::string& sTimestampUp)
{

  // split timestamp in order to check format
  std::vector <std::string>  splitLow;
  std::vector <std::string>  splitUp;

  stl::split( sTimestampLow, "_", splitLow );
  stl::split( sTimestampUp, "_", splitUp );

  // check that each character of timestamp are digits
  bool  bIsAlpha = false ;

  // check that all informations of timestamp bound are digits, excepts the separator
  for( size_t l = 0 ; l < 2 ; ++l )
  {
      int i = 0 ;
      // check that lower bound timestamp is composed of digits only
      while (splitLow[l][i])
      {
          if ( !std::isdigit(splitLow[l][i]))
          {
              bIsAlpha = true ;
          }
          i++;
      }

      // check that upper bound timestamp is composed of digits only
      i=0;
      while (splitUp[l][i])
      {
          if ( !std::isdigit(splitUp[l][i]))
          {
              bIsAlpha = true ;
          }
          i++;
      }
  }

  // check that the two timestamps are in the format 0123456789_012345
  if(  splitLow.size() == 2              // if timestamp is composed of two blocks of integer
       && splitUp.size() == 2            // if timestamp is composed of two blocks of integer
       && splitLow[0].size() == 10       // if first block is of size 10
       && splitLow[1].size() == 6        // if second block is of size 6
       && splitUp[0].size()  == 10       // if first block is of size 10
       && splitUp[1].size()  == 6        // if second block is of size 6
       && !bIsAlpha                      // if timestamp contains only digits
    )
  {
      // check that lower bound is less than upper
      if( sTimestampLow < sTimestampUp )
      {
          return true;
      }
      else
      {
          std::cerr << "\nTimestamp Upper bound is less than lower bound " << std::endl;
          return false;
      }

  }
  else
  {
      std::cerr << "\nThe provided timestamp are not in the right format " << std::endl;
      return false;
  }

}

/*******************************************************************************
* verify that software argument are valid
*
********************************************************************************
*/

bool isInputValid( const char*  softName,
                   const std::string& sImageDir,
                   const std::string& sOutputDir,
                   const std::string& smacAddress,
                   const std::string& sMountPoint,
                   const std::string& sChannelFile,
                   const bool & bRigidRig,
                   const bool & bUseCalibPrincipalPoint,
                   const double & focalPix,
                   const std::string& sTimestampLow,
                   const std::string& sTimestampUp)
{

    std::cout << " You called : " <<std::endl
              << softName << std::endl
              << "--imageDirectory " << sImageDir << std::endl
              << "--macAddress " << smacAddress << std::endl
              << "--outputDirectory " << sOutputDir << std::endl
              << "--mountPoint " << sMountPoint << std::endl
              << "--channelFile " << sChannelFile << std::endl
              << "--rigidRig " << bRigidRig << std::endl
              << "--useCalibPrincipalPoint " << bUseCalibPrincipalPoint << std::endl
              << "--focal " << focalPix << std::endl
              << "--lowerBound " << sTimestampLow << std::endl
              << "--upperBound " << sTimestampUp << std::endl;

    // check if image dir exists
    if ( !stlplus::folder_exists( sImageDir ) )
    {
      std::cerr << "\nThe input directory doesn't exist" << std::endl;
      return false;
    }
    else
    {
        // check if output dir is given
        if (sOutputDir.empty())
        {
          std::cerr << "\nInvalid output directory" << std::endl;
          return false;
        }
        else
        {
            // if output dir is empty, create it
            if ( !stlplus::folder_exists( sOutputDir ) )
            {
                if( !stlplus::folder_create ( sOutputDir ) )
                {
                    std::cerr << "\nCannot create output directory" << std::endl;
                    return false;
                }
                else
                {
                    // check if mac address is given
                    if( smacAddress.empty() )
                    {
                      std::cerr << "\n No mac address given " << std::endl;
                      return false;
                    }
                    else
                    {
                      // check if mount point is given
                      if( sMountPoint.empty() )
                      {
                        std::cerr << "\n No mount point given " << std::endl;
                        return false;
                      }
                      else
                        return true;
                    }
                }
            }
            else
            {
                // check if mac address is given
                if( smacAddress.empty() )
                {
                  std::cerr << "\n No mac address given " << std::endl;
                  return false;
                }
                else
                {
                    // check if mount point is given
                    if( sMountPoint.empty() )
                    {
                      std::cerr << "\n No mount point given " << std::endl;
                      return false;
                    }
                    else
                      return true;
                }
            }
        }
    }
}

/*********************************************************************
*  load calibration data related to elphel cameras
*
*********************************************************************/

bool  loadCalibrationData(  std::vector< sensorData >  & vec_sensorData,
                            const std::string & sMountPoint,
                            const std::string & smacAddress)
{
      /* Key/value-file descriptor */
      lf_Descriptor_t lfDesc;
      lf_Size_t       lfChannels=0;

      /* Creation and verification of the descriptor */
      char *c_data = new char[sMountPoint.length() + 1];
      std::strcpy(c_data, sMountPoint.c_str());

      char *c_mac = new char[smacAddress.length() + 1];
      std::strcpy(c_mac, smacAddress.c_str());

      if ( lf_parse( (unsigned char*)c_mac, (unsigned char*)c_data, & lfDesc ) == LF_TRUE ) {

          /* Query number of camera channels */
          lfChannels = lf_query_channels( & lfDesc );

          /* iter on sucamera */
          for( li_Size_t sensor_index = 0 ; sensor_index < lfChannels ; ++sensor_index )
          {
            sensorData  sD;

            /* Query number width and height of sensor image */
            sD.lfWidth  = lf_query_pixelCorrectionWidth ( sensor_index, & lfDesc );
            sD.lfHeight = lf_query_pixelCorrectionHeight( sensor_index, & lfDesc );

            /* Query focal length of camera sensor index */
            sD.lfFocalLength = lf_query_focalLength( sensor_index , & lfDesc );
            sD.lfPixelSize   = lf_query_pixelSize  ( sensor_index , & lfDesc );

            /* Query angles used for gnomonic rotation */
            sD.lfAzimuth    = lf_query_azimuth    ( sensor_index , & lfDesc );
            sD.lfHeading    = lf_query_heading    ( sensor_index , & lfDesc );
            sD.lfElevation  = lf_query_elevation  ( sensor_index , & lfDesc );
            sD.lfRoll       = lf_query_roll       ( sensor_index , & lfDesc );

            // compute rotation and store it.
            computeRotationEl ( &sD.R[0] , sD.lfAzimuth , sD.lfHeading, sD.lfElevation, sD.lfRoll );

            /* Query principal point */
            sD.lfpx0 = lf_query_px0 ( sensor_index , & lfDesc );
            sD.lfpy0 = lf_query_py0 ( sensor_index , & lfDesc );

            /* Query information related to entrance pupil center */
            sD.lfRadius   = lf_query_radius               ( sensor_index , & lfDesc );
            sD.lfCheight  = lf_query_height               ( sensor_index , & lfDesc );
            sD.lfEntrance = lf_query_entrancePupilForward ( sensor_index , & lfDesc );

            // compute optical center in camera coordinate and store it
            getOpticalCenter ( &sD.C[0] , sD.lfRadius, sD.lfCheight, sD.lfAzimuth, sD.R, sD.lfEntrance );

            vec_sensorData.push_back(sD);
          }

          /* Release descriptor */
          lf_release( & lfDesc );

          /* calibration information are loaded*/
          return true;
      }
      else
      {
          std::cerr << " Could not read calibration data. Exit" << std::endl;
          return false;
      }
      delete [] c_data;
      delete [] c_mac;
}

/*********************************************************************
*  read channel file (if exists )
*
*********************************************************************/

void loadChannelFile( std::set< li_Size_t >  & keptChan,
                      const std::string & sChannelFile )
{
    // if channel file is given, read it
    if( !sChannelFile.empty() )
    {
        std::ifstream  inFile(sChannelFile.c_str());
        li_Size_t chan;

        if( inFile.is_open() )
        {
            while( inFile >> chan ) // one channel per line
            {
                keptChan.insert(chan);
            }

            if( keptChan.size() == 0 )
            {
                std::cerr << "Warning:: No channels are read in file, use all image instead" << std::endl;
            }
        }
        else
        {
            std::cerr << "Warning :: Could not open channel file. Use all image instead" << std::endl;
        }

        inFile.close();
    }
    else
    {
        std::cout << "\n Keep all channels " << std::endl;
    }

}

/*********************************************************************
*  compute image intrinsic parameter
*
*********************************************************************/

void computeImageIntrinsic(
                camInformation & camInfo,
                const std::vector < sensorData > & vec_sensorData,
                const std::string & timestamp,
                const size_t   & sensor_index,
                const double   & focalPix,
                const bool     & bUseCalibPrincipalPoint,
                const bool     & bRigidRig )
{
    // affect width and height
    camInfo.width  = vec_sensorData[sensor_index].lfWidth;
    camInfo.height = vec_sensorData[sensor_index].lfHeight;

    // Create list if focal is given
    if( focalPix > 0.0 )
    {
       camInfo.focal = focalPix;
    }
    // Create list if full calibration data are used
    else
    {
        li_Real_t focalPix = vec_sensorData[sensor_index].lfFocalLength / vec_sensorData[sensor_index].lfPixelSize;
        camInfo.focal = focalPix;
     }

    if(!bUseCalibPrincipalPoint)
    {
        camInfo.px0   = camInfo.width  / 2.0;
        camInfo.py0   = camInfo.height / 2.0;
    }
    else
    {
        camInfo.px0   = vec_sensorData[sensor_index].lfpx0;
        camInfo.py0   = vec_sensorData[sensor_index].lfpy0;
    }

    // export channel
    camInfo.subChan = sensor_index;

    // if rigid rig, add some informations
    if(bRigidRig)
    {
        // export rig index
        camInfo.sRigName = timestamp;

        // export rotation
        camInfo.R[0] = vec_sensorData[sensor_index].R[0];
        camInfo.R[1] = vec_sensorData[sensor_index].R[1];
        camInfo.R[2] = vec_sensorData[sensor_index].R[2];
        camInfo.R[3] = vec_sensorData[sensor_index].R[3];
        camInfo.R[4] = vec_sensorData[sensor_index].R[4];
        camInfo.R[5] = vec_sensorData[sensor_index].R[5];
        camInfo.R[6] = vec_sensorData[sensor_index].R[6];
        camInfo.R[7] = vec_sensorData[sensor_index].R[7];
        camInfo.R[8] = vec_sensorData[sensor_index].R[8];

        // export translation
        camInfo.C[0] = vec_sensorData[sensor_index].C[0];
        camInfo.C[1] = vec_sensorData[sensor_index].C[1];
        camInfo.C[2] = vec_sensorData[sensor_index].C[2];
    }
}

/*********************************************************************
*  keep only rigs in timestamp range
*
*********************************************************************/

void keepRigsInTimestampRange(
           std::set <string> & imageToRemove,
           const std::map<std::string, std::vector<string> > & mapSubcamPerTimestamp,
           const std::string& sTimestampLower,
           const std::string& sTimestampUpper )
{
      // Initialize variables
      li_Size_t               k = 0;
      li_Size_t               i = 0;
      std::map < std::string, std::vector<string> >::const_iterator iter_timestamp = mapSubcamPerTimestamp.begin();

      // check timestamp range validity
      bool validRange = isRangeValid(sTimestampLower, sTimestampUpper);
      if( !validRange )
      {
          std::cerr << "\n[keepRepresentativeRigs::WARNING] openMVG will use all images, timestamp range is not valid " << std::endl;
      }

      // remove rig not having the max occurrence subcam number
      for( k = 0 ; k < mapSubcamPerTimestamp.size(); ++k )
      {
          //advance iterator
          iter_timestamp = mapSubcamPerTimestamp.begin();
          std::advance(iter_timestamp, k);

          //update map
          if( ( iter_timestamp->first < sTimestampLower && validRange )
              || ( iter_timestamp->first > sTimestampUpper && validRange )
            )
          {
              for( i  = 0 ; i < iter_timestamp->second.size() ; ++i )
              {
                  imageToRemove.insert( iter_timestamp->second[i] );
              }
          }
      }
}


/*********************************************************************
*  keep only most representative rigs
*
*********************************************************************/

void keepRepresentativeRigs(
          std::set <string> & imageToRemove,
          const std::map<std::string, std::vector<string> > & mapSubcamPerTimestamp )
{
    // check the number of subcamera per rig. Remove less representated rigs.
    li_Size_t timeStamp = 0;
    std::map < std::string, std::vector<string> >::const_iterator iter_timestamp = mapSubcamPerTimestamp.begin();
    std::pair< std::map < li_Size_t , li_Size_t >::iterator, bool > insert_check;
    std::map < li_Size_t , li_Size_t >  occurencePerSubcamNumber;

    // compute occurrence of each subcamera number
    for( timeStamp = 0 ; timeStamp < mapSubcamPerTimestamp.size(); ++timeStamp )
    {
        //advance iterator
        iter_timestamp = mapSubcamPerTimestamp.begin();
        std::advance(iter_timestamp, timeStamp);

        //update map
        insert_check = occurencePerSubcamNumber.insert (
        std::pair<li_Size_t, li_Size_t>( iter_timestamp->second.size(), occurencePerSubcamNumber.size()) );

        if(insert_check.second == true )
        {
            occurencePerSubcamNumber[iter_timestamp->second.size() ] = 1 ;
        }
        else
        {
            occurencePerSubcamNumber[iter_timestamp->second.size() ] += 1 ;
        }
    }

    // compute most representative number of subcamera
    li_Size_t               k = 0;
    li_Size_t               i = 0;
    li_Size_t   max_Occurence = 0;
    li_Size_t   subCamNumber  = 0;

    std::map < li_Size_t , li_Size_t >::const_iterator iter_occurrence = occurencePerSubcamNumber.begin();

    for( k = 0 ; k < occurencePerSubcamNumber.size(); ++k )
    {
        //advance iterator
        iter_occurrence = occurencePerSubcamNumber.begin();
        std::advance(iter_occurrence, k);

        //update map
        if( iter_occurrence->second > max_Occurence )
        {
            max_Occurence = iter_occurrence->second;
            subCamNumber  = iter_occurrence->first;
        }

    }

    // remove rig not having the max occurrence subcam number
    for( k = 0 ; k < mapSubcamPerTimestamp.size(); ++k )
    {
        //advance iterator
        iter_timestamp = mapSubcamPerTimestamp.begin();
        std::advance(iter_timestamp, k);

        //update map
        if(    iter_timestamp->second.size() != subCamNumber )
        {
            for( i  = 0 ; i < iter_timestamp->second.size() ; ++i )
            {
                imageToRemove.insert( iter_timestamp->second[i] );
            }
        }
    }

    std::cout << "\n\n Max rig occurence is " << max_Occurence
          << ", rig number of sub cameras is " << subCamNumber
          << std::endl;
}

/*********************************************************************
*  compute camera and rig intrinsic parameters
*
*********************************************************************/

bool computeInstrinsicPerImages(
              std::vector<std::string> & vec_image,
              const std::vector< sensorData > & vec_sensorData,
              const std::set< li_Size_t >  & keptChan,
              const std::string & sImageDir,
              const std::string & sOutputDir,
              const std::string & sGpsFile,
              const double & focalPix,
              const bool & bUsePrincipalPoint,
              const bool & bUseRigidRig,
              const bool & bUseGPS,
              std::string& sTimestampLower,
              std::string& sTimestampUpper)
{
  //initialize rig map
  std::map<std::string, li_Size_t>  mapRigPerImage;
  std::sort(vec_image.begin(), vec_image.end());

  // create output
  std::map<std::string, camInformation> camAndIntrinsics;

  C_Progress_display my_progress_bar_image( vec_image.size(),
  std::cout, "\n List computation progression:\n");

  // declare variable before loop
  std::vector<std::string>::const_iterator iter_image = vec_image.begin();
  std::pair< std::map<std::string, li_Size_t>::iterator, bool > ret;
  std::map<std::string, std::vector<string> >  mapSubcamPerTimestamp;
  std::vector<string>     splitted_name;
  std::string             timestamp;
  std::string             sImageFilename;
  std::string             minTimestamp="";
  std::string             maxTimestamp="";

  li_Size_t sensor_index  = 0;
  li_Size_t i             = 0;
  li_Size_t idx           = 0;
  bool      bKeepChannel  = false;

  // do a parallel loop to improve CPU TIME
  #ifdef OPENMVG_USE_OPENMP
    #pragma omp parallel firstprivate(iter_image, splitted_name, timestamp, sensor_index, i, bKeepChannel, sImageFilename )
    #pragma omp for schedule(dynamic)
  #endif
  for ( idx = 0; idx < vec_image.size(); ++idx)
  {
      //advance iterator
      iter_image = vec_image.begin();
      std::advance(iter_image, idx);

      sImageFilename = stlplus::create_filespec( sImageDir, *iter_image );

      // Test if the image format is supported:
      if (openMVG::image::GetFormat(sImageFilename.c_str()) == openMVG::image::Unknown)
      {
          std::cerr << " Warning : image " << sImageFilename << "\'s format is not supported." << std::endl;
      }
      else
      {
          // extract channel information from image name
          splitted_name.clear();

#ifdef OLD_IMAGE_FILENAME_FORMAT
          stl::split( *iter_image, "-", splitted_name );
          sensor_index = atoi(splitted_name[1].c_str());

          // now load image information and keep channel index and timestamp
          timestamp = splitted_name[0];
#else // NEW filename 
          stl::split( *iter_image, "_", splitted_name );
          sensor_index = atoi(splitted_name[2].c_str());

          // now load image information and keep channel index and timestamp
          timestamp = splitted_name[0] + "_" + splitted_name[1];
#endif

          #ifdef OPENMVG_USE_OPENMP
            #pragma omp critical
          #endif
          {
              // update min timestamp
              if( minTimestamp.empty() )
              {
                  minTimestamp = timestamp;
              }
              else
              {
                  minTimestamp = std::min( minTimestamp, timestamp);
              }

              // update max timestamp
              if( maxTimestamp.empty() )
              {
                  maxTimestamp = timestamp;
              }
              else
              {
                  maxTimestamp = std::max( maxTimestamp, timestamp);
              }
          }

          // if no channel file is given, keep all images
          bKeepChannel = false;

          if( keptChan.empty() )
          {
              bKeepChannel = true ;
          }
          else
          {
              bKeepChannel = keptChan.count(sensor_index) > 0;
          }

          // export only kept channel
          if( bKeepChannel )
          {
              #ifdef OPENMVG_USE_OPENMP
                #pragma omp critical
              #endif
              {
                  // insert timestamp in the map
                  ret = mapRigPerImage.insert ( std::pair<std::string, li_Size_t>(timestamp, mapRigPerImage.size()) );
                  if(ret.second == true )
                  {
                      mapRigPerImage[timestamp] = mapRigPerImage.size()-1;
                  }

                  //insert image in map timestamp -> subcam
                  mapSubcamPerTimestamp[timestamp].push_back( *iter_image );
              }

              // export camera information
              camInformation   camInfo;

              computeImageIntrinsic(
                  camInfo,
                  vec_sensorData,
                  timestamp,
                  sensor_index,
                  focalPix,
                  bUsePrincipalPoint,
                  bUseRigidRig);

              //export info
              #ifdef OPENMVG_USE_OPENMP
                #pragma omp critical
              #endif
              {
                  camAndIntrinsics[*iter_image] = camInfo;
              }

          };

          //update progress bar
          #ifdef OPENMVG_USE_OPENMP
            #pragma omp critical
          #endif
          {
              ++my_progress_bar_image;
          }

      } //endif format known

  }; // loop on vector image

  if( sTimestampLower.empty() )
      sTimestampLower = minTimestamp;

  if( sTimestampUpper.empty() )
      sTimestampUpper = maxTimestamp;

  // keep only most represented rig
  std::set < std::string >  imageToRemove;
  const bool    bKeepRepresentativeRigs = false;

  // if we use rigid rig structure, keep most representative rigs
  if( bUseRigidRig || bKeepRepresentativeRigs)
  {
      keepRepresentativeRigs( imageToRemove,
                              mapSubcamPerTimestamp );
  }

  // keep only poses in defined timestamp range
  keepRigsInTimestampRange( imageToRemove,
                            mapSubcamPerTimestamp,
                            sTimestampLower,
                            sTimestampUpper);

  std::cout << " OpenMVG will use " << camAndIntrinsics.size() - imageToRemove.size()
            << " of " << camAndIntrinsics.size() << " input images "
            << std::endl << std::endl;

  //load json file
  SfM_Gps_Data  loaded_GPS_Data;
  // create R/C map per timestamp
  std::map < std::string, Mat3 >  map_rotationPerTimestamp;
  std::map < std::string, Vec3 >  map_translationPerTimestamp;

  if( bUseGPS )
  {
    //load json file
    Load_gpsimu_cereal( loaded_GPS_Data, sGpsFile );

    // create R/C map per timestamp
    create_gps_imu_map( loaded_GPS_Data,
                        map_rotationPerTimestamp,
                        map_translationPerTimestamp );
  }

  //now create openMVG camera views on remaing rigs
  openMVG::sfm::SfM_Data sfm_data;
  sfm_data.s_root_path = sImageDir;
  size_t cpt = 0;

  // use only complete rigs
  std::set<std::string>::iterator  it = imageToRemove.end();

  for( const auto & iter : camAndIntrinsics)
  {
    // check if we have to keep this timestamp
    it = imageToRemove.find(iter.first);
    if ( it == imageToRemove.end() )
    {
        // extract camera information
        const camInformation    cam = iter.second;
        const std::string  img_name = iter.first;
        std::string  timestamp = cam.sRigName;

        // Build the view corresponding to the image
        if( bUseRigidRig )
        {
            sfm_data.views[cpt] = std::make_shared<openMVG::sfm::Rig_View>(img_name, cpt, cpt, mapRigPerImage[cam.sRigName], cam.width, cam.height, mapRigPerImage[cam.sRigName], cam.subChan);
        }
        else
        {
            sfm_data.views[cpt] = std::make_shared<openMVG::sfm::View>(img_name, cpt, cpt, cpt, cam.width, cam.height);
        }

        // update intrinsics map
        if ( bUseRigidRig )
        {
            // update intrinsic data base with a subpose field
            const openMVG::geometry::Pose3 pose(Mat3(cam.R).transpose(), Vec3(cam.C));
            sfm_data.intrinsics[cpt] = std::make_shared<openMVG::cameras::Rig_Pinhole_Intrinsic> (cam.width, cam.height, cam.focal, cam.px0, cam.py0, pose);
        }
        else
        {
            // No subpose to configure
            sfm_data.intrinsics[cpt] = std::make_shared<openMVG::cameras::Pinhole_Intrinsic> (cam.width, cam.height, cam.focal, cam.px0, cam.py0);
        }

        // Update pose prior if we have GPS informations
        // - usable only in rig case, since it will combined with the subpose on the fly
        if ( bUseRigidRig && bUseGPS && sfm_data.poses.count(mapRigPerImage[cam.sRigName]) == 0 )
        {
            // extract informations relative to image (timestamp, subcam rotation and optical center)
            const std::string timestamp = cam.sRigName;

            // now extract rig pose
            const Mat3 Rr =
              (map_rotationPerTimestamp.find(timestamp) == map_rotationPerTimestamp.end()) ?
                Mat3::Identity():
                map_rotationPerTimestamp.at(timestamp);

            const Vec3 Cr =
              (map_translationPerTimestamp.find(timestamp) == map_translationPerTimestamp.end())?
                Vec3::Zero():
                map_translationPerTimestamp.at(timestamp);

            sfm_data.poses[ mapRigPerImage[cam.sRigName] ] = openMVG::geometry::Pose3(Rr,Cr);
        }
        ++cpt;
    }
  }

  // Group intrinsics that share common properties
  {
    GroupSharedIntrinsics(sfm_data);
  }

  // Store SfM_Data views & intrinsic data
  if (Save(
        sfm_data,
        stlplus::create_filespec( sOutputDir, "sfm_data.json" ).c_str(),
        openMVG::sfm::ESfM_Data(openMVG::sfm::VIEWS|openMVG::sfm::INTRINSICS|openMVG::sfm::EXTRINSICS)) )
  {
      return true;
  }
  else
      return false;

}
