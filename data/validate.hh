//
// Copyright (c) 2020, Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __VALIDATE_HH__
#define __VALIDATE_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
#endif

#include <string>
#include <vector>
#include <iostream>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "data/osmobjects.hh"
#include "timer.hh"

/// \file validate.hh
/// \brief This class tries to validate the OSM objects
///
/// This class analyzes an OSM object to look for errors. This may
/// include lack of tags on a POI node or a way. This is not an
/// exhaustive test, mostly just a fast sanity-check.

// JOSM validator
//   Crossing ways
//   Duplicate Ways
//   Duplicate nodes
//   Duplicate relations
//   Duplicated way nodes
//   Orphan nodes
//   No square building corners

// OSMInspector
//   Empty tag key
//   Unknown highway type
//   Single node way
//   Interescting ways

// OSMose
//   Overlapping buildings
//   orphan nodes
//   Duplicate geomtry
//   Highway not connected
//   Missing tags
//   Duplicate object
//   
//

/// \namespace validate
namespace validate {

typedef enum {notags, isbuilding } errortype_t;

class Validate
{
public:
    Validate() {};

    /// Check a POI for tags. A node that is part of a way shouldn't have any
    /// tags, this is to check actual POIs, like a school.
    bool checkPOI(osmobjects::OsmNode &node);

    /// This checks a way. A way should always have some tags. Often a polygon
    /// is a building 
    bool checkWay(osmobjects::OsmWay &way);

    bool checkTags (std::map<std::string, std::string> tags) {
        bool result;
        for (auto it = std::begin(tags); it != std::end(tags); ++it) {
            result = checkTag(it->first, it->second);
        }
        return result;
    };

    bool checkTag(const std::string &key, const std::string &value);

private:
    std::vector<long> buildings;       ///< 
    std::vector<long> node_errors;     ///< 
    std::vector<long> way_errors;      ///< 
    std::vector<long> relation_errors; ///< 
};

} // EOF validate namespace

#endif  // EOF __VALIDATE_HH__