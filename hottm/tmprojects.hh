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

#ifndef __TMPROJECTS_HH__
#define __TMPROJECTS_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "hotconfig.h"
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

namespace tmdb {

class TMProject
{
public:
    TMProject(pqxx::result::const_iterator &row);
// private:
    int id;
    int status;
    ptime created;
    int priority;
    std::string default_locale;
    long author_id;
    int mapper_level;
    
    int mapping_permission;
    int validation_permission;
    
    // enforce_random_task_selection
    // bool private;
    bool featured;
    std::string entities_to_map;
    std::string changeset_comment;
    std::string osmcha_filter_id;
    ptime due_date;
    std::string imagery;
    std::string josm_preset;
    std::string id_presets;
    ptime last_updated;
    int license_id;
    //  geometry public.geometry(MultiPolygon,4326),
    // centroid public.geometry(Point,4326),
    std::string  country;
    int task_creation_mode;
    int organisation_id;
    int mapping_types;
    int mapping_editors;
    int validation_editors;
    int total_tasks;
    int tasks_mapped;
    int tasks_validated;
    int tasks_bad_imagery;
};

} // EOF tmdb namespace
#endif  // EOF __TMPROJECTS_HH__