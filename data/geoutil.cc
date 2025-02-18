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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <pqxx/pqxx>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "data/geoutil.hh"
#include "galaxy/changeset.hh"
#include "galaxy/galaxy.hh"
#include <ogr_geometry.h>
#include <ogrsf_frmts.h>

#include "log.hh"
using namespace logger;

namespace geoutil {

bool
GeoUtil::readFile(const std::string &filespec)
{
    std::filesystem::path boundary_file = filespec;
    if (filespec.front() == '.' || filespec.front() == '/') {
	if (!std::filesystem::exists(boundary_file)) {
	    log_error(_("Geo boundary file %1% doesn't exist!"), boundary_file);
	}
    } else {
	if (!std::filesystem::exists(boundary_file)) {
	    boundary_file = SRCDIR;
	    boundary_file += "/data/" + filespec;
	    if (!std::filesystem::exists(boundary_file)) {
		boundary_file = PKGLIBDIR;
		boundary_file += "/" + filespec;		
	    }
	}
    }
    if (!std::filesystem::exists(boundary_file)) {
	log_error(_("Boundary file %1% doesn't exist!"), boundary_file);
	return false;
    }
    
    log_debug(_("Opening geo data file: %1%"), boundary_file);
    std::string foo = boundary_file.string();
    GDALDataset *poDS = (GDALDataset *)GDALOpenEx(foo.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
    if (poDS == 0) {
        log_error(_("couldn't open %1%"), boundary_file);
        return false;
    }

    OGRLayer *layer;
    layer = poDS->GetLayerByName(boundary_file.stem().c_str());
    if (layer == 0) {
        log_error(_("Couldn't get layer \"%1%\""),boundary_file.stem());
        return false;
    }

    if (layer != 0) {
        for (auto &feature : layer) {
            const OGRGeometry *geom = feature->GetGeometryRef();
            const OGRMultiPolygon *mp = geom->toMultiPolygon();
            char *wkt = NULL;
            int bar = mp->exportToWkt(&wkt);
            boost::geometry::read_wkt(wkt, boundary);
            CPLFree(wkt);
        }
    }

    return true;
}

/// Read an EWKT string as the boundary, instead of a file.
bool
GeoUtil::readPoly(const std::string &wkt)
{
    boost::geometry::read_wkt(wkt, boundary);
}

} // namespace geoutil

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
