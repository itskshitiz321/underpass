//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __GEOUTIL_HH__
#define __GEOUTIL_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>

#include "osmstats/osmstats.hh"
#include "data/osmobjects.hh"

/// \file geoutil.hh
/// \brief This file parses a data file containing boundaries
///
/// This uses GDAL to read and parse a file in any supported format
/// and loads it into a data structure. This is used to determine which
///country a change was made in.

/// \namespace geoutil
namespace geoutil {

/// \class GeoUtil
/// \brief Read in the priority area boundaries data file
///
/// This parses the data file in any GDAL supported format into a data
/// structure that can be used to determine which area a change was
/// made in. This is used from C++ to avoid a call to the database
/// when processing changes.
class GeoUtil
{
public:
    GeoUtil(void) {
        GDALAllRegister();
    };
    /// Read a file into internal storage so boost::geometry functions
    /// can be used to process simple geospatial calculations instead
    /// of using postgres. This data is is used to determine which
    /// country a change was made in, or filtering out part of the
    /// planet to reduce data size. Since this uses GDAL, any
    /// multi-polygon file of any supported format can be used.
    bool readFile(const std::string &filespec);
    
    /// See if this changeset is in a priority area. We ignore changsets in
    /// areas like North America to reduce the amount of data needed
    /// for calulations. This boundary can always be modified.
    bool inPriorityArea(double lat, double lon) {
        return inPriorityArea(point_t(lon, lat));
    };

    bool inPriorityArea(polygon_t poly) {
        point_t pt;
        boost::geometry::centroid(poly, pt);
        return inPriorityArea(pt);
    };
    bool inPriorityArea(point_t pt) {
        return boost::geometry::within(pt, boundary);
    };

// private:
    multipolygon_t boundary;
};
    
}       // EOF geoutil

#endif  // EOF __GEOUTIL_HH__
