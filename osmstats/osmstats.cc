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
#include "pqxx/nontransaction"

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "osmstats/osmstats.hh"
#include "osmstats/changeset.hh"

using namespace apidb;

namespace osmstats {

QueryOSMStats::QueryOSMStats(void)
{
    std::string database = "osmstats";
    connect(database);
    database = "tmsnap";
    QueryDB::connect(database);
}

bool
QueryOSMStats::connect(const std::string &dbname)
{
    std::string args;
    if (dbname.empty()) {
	args = "dbname = osmstats";
    } else {
	args = "dbname = " + dbname;
    }
    
    try {
	db = new pqxx::connection(args);
	if (db->is_open()) {
            // worker = new pqxx::work(*db);
	    return true;
	} else {
	    return false;
	}
    } catch (const std::exception &e) {
	std::cerr << e.what() << std::endl;
	return false;
   }    
}

bool
QueryOSMStats::applyChange(changeset::ChangeSet &change)
{
    // change.dump();
    worker = new pqxx::work(*db);
    // Add user data
    addUser(change.uid, change.user);
    std::string query = "INSERT INTO raw_users VALUES(";
    query += std::to_string(change.uid) + ",\'" + change.user;
    query += "\') ON CONFLICT DO NOTHING;";
    pqxx::result result = worker->exec(query);

    // Add hashtags. The id is added by the database
    for (auto it = std::begin(change.hashtags); it != std::end(change.hashtags); ++it) {
        bool key_exists = hashtags.find(*it) != hashtags.end();
        if (!key_exists) { 
            std::cout << *it << " doesn't exist " << std::endl;
            addHashtag(0, *it);
            query = "INSERT INTO raw_hashtags (id, hashtag) VALUES(nextval(\'raw_hashtags_id_seq\'),\'";
            query += *it + "\') ON CONFLICT DO NOTHING;";
            pqxx::result result = worker->exec(query);
            addHashtag(0, *it);
        } else {
            std::cout << *it << " does exist " << std::endl;
        }
    }

    // Add changeset data
    // road_km_added | road_km_modified | waterway_km_added | waterway_km_modified | roads_added | roads_modified | waterways_added | waterways_modified | buildings_added | buildings_modified | pois_added | pois_modified | verified | ugmented_diffs | updated_at
    query = "INSERT INTO raw_changesets (id, editor, user_id, created_at, closed_at) VALUES(";
    query += std::to_string(change.id) + ",\'" + change.editor + "\',\'" + std::to_string(change.uid) + "\',\'";
    query += to_simple_string(change.created_at) + "\',\'" + to_simple_string(change.closed_at) + "\')";
    query += " ON CONFLICT DO NOTHING;";
    std::cout << "QUERY: " << query << std::endl;
    result = worker->exec(query);

    // Commit the results to the database
    worker->commit();
}

// Populate internal storage of a few heavily used data, namely
// the indexes for each user, country, or hashtag.
bool
QueryOSMStats::populate(void)
{
    worker = new pqxx::work(*db);
    // Get the country ID from the raw_countries table
    std::string query = "SELECT id,name,code FROM raw_countries;";
    pqxx::result result = worker->exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawCountry rc(it);
        // addCountry(rc);
        // long id = std::stol(it[0].c_str());
        countries.push_back(rc);
    };

    query = "SELECT id,name FROM raw_users;";
    result = worker->exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawUser ru(it);
        users.push_back(ru);
    };

    query = "SELECT id,hashtag FROM raw_hashtags;";
    result = worker->exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawHashtag rh(it);
        hashtags[rh.name] = rh;
    };

    // ptime start = time_from_string("2010-07-08 13:29:46");
    // ptime end = second_clock::local_time();
    // long roadsAdded = QueryStats::getCount(QueryStats::highway, 0,
    //                                        QueryStats::totals, start, end);
    // long roadKMAdded = QueryStats::getLength(QueryStats::highway, 0,
    //                                          start, end);
    // long waterwaysAdded = QueryStats::getCount(QueryStats::waterway, 0,
    //                                            QueryStats::totals, start, end);
    // long waterwaysKMAdded = QueryStats::getLength(QueryStats::waterway, 0,
    //                                               start, end);
    // long buildingsAdded = QueryStats::getCount(QueryStats::waterway, 0,
    //                                            QueryStats::totals, start, end);

    worker->commit();
};

bool
QueryOSMStats::getRawChangeSet(std::vector<long> &changeset_ids)
{
    worker = new pqxx::work(*db);
    std::string sql = "SELECT id,road_km_added,road_km_modified,waterway_km_added,waterway_km_modified,roads_added,roads_modified,waterways_added,waterways_modified,buildings_added,buildings_modified,pois_added,pois_modified,editor,user_id,created_at,closed_at,verified,augmented_diffs,updated_at FROM raw_changesets WHERE id=ANY(ARRAY[";
    // Build an array string of the IDs
    for (auto it = std::begin(changeset_ids); it != std::end(changeset_ids); ++it) {
        sql += std::to_string(*it);
        if (*it != changeset_ids.back()) {
            sql += ",";
        }
    }
    sql += "]);";

    std::cout << "QUERY: " << sql << std::endl;
    pqxx::result result = worker->exec(sql);
    std::cout << "SIZE: " << result.size() <<std::endl;
    OsmStats stats(result);
    worker->commit();

    for (auto it = std::begin(result); it != std::end(result); ++it) {
        OsmStats os(it);
        ostats.push_back(os);
    }
}

void
QueryOSMStats::dump(void)
{
    for (auto it = std::begin(ostats); it != std::end(ostats); ++it) {
        it->dump();
    }
}

/// Write the list of hashtags to the database
int
QueryOSMStats::updateRawHashtags(void)
{
//    INSERT INTO keys(name, value) SELECT 'blah', 'true' WHERE NOT EXISTS (SELECT 1 FROM keys WHERE name='blah');

    std::string query = "INSERT INTO raw_hashtags(hashtag) VALUES(";
    // query += "nextval('raw_hashtags_id_seq'), \'" + tag;
    // query += "\'" + tag;
    query += "\') ON CONFLICT DO NOTHING";
    std::cout << "QUERY: " << query << std::endl;
    pqxx::result result = worker->exec(query);

    return result.size();
}

int
QueryOSMStats::updateCountries(int id, int country_id)
{
    std::string query = "INSERT INTO raw_countries VALUES";
    query += std::to_string(id) + "," + std::to_string(country_id);
    query += ") ON CONFLICT DO NOTHING";
    pqxx::result result = worker->exec(query);
    return result.size();
}

OsmStats::OsmStats(pqxx::const_result_iterator &res)
{
    id = std::stol(res[0].c_str());
    road_km_added = std::stol(res[1].c_str());
    road_km_modified = std::stol(res[2].c_str());
    waterway_km_added = std::stol(res[3].c_str());
    waterway_km_modified = std::stol(res[4].c_str());;
    roads_added = std::stol(res[5].c_str());
    roads_modified = std::stol(res[6].c_str());
    waterways_added = std::stol(res[7].c_str());
    waterways_modified = std::stol(res[8].c_str());
    buildings_added = std::stol(res[9].c_str());
    buildings_modified = std::stol(res[10].c_str());
    pois_added = std::stol(res[11].c_str());
    pois_modified = std::stol(res[12].c_str());
    editor = pqxx::to_string(res[13]);
    user_id = std::stol(res[14].c_str());
    created_at = time_from_string(pqxx::to_string(res[15]));
    closed_at = time_from_string(pqxx::to_string(res[16]));
    // verified = res[17].bool();
    // augmented_diffs = res[18].num();
    updated_at = time_from_string(pqxx::to_string(res[19]));
}

OsmStats::OsmStats(const pqxx::result &res)
{
    id = res[0][0].num();
    road_km_added = res[0][1].num();
    road_km_modified = res[0][2].num();
    waterway_km_added = res[0][3].num();
    waterway_km_modified = res[0][4].num();
    roads_added = res[0][5].num();
    roads_modified = res[0][6].num();
    waterways_added = res[0][7].num();
    waterways_modified = res[0][8].num();
    buildings_added = res[0][9].num();
    buildings_modified = res[0][10].num();
    pois_added = res[0][11].num();
    pois_modified = res[0][12].num();
    editor = pqxx::to_string(res[0][13]);
    user_id = res[0][14].num();
    created_at = time_from_string(pqxx::to_string(res[0][15]));
    closed_at = time_from_string(pqxx::to_string(res[0][16]));
    // verified = res[0][17].bool();
    // augmented_diffs = res[0][18].num();
    updated_at = time_from_string(pqxx::to_string(res[0][19]));
}

void
OsmStats::dump(void)
{
    std::cout << "-----------------------------------" << std::endl;
    std::cout << "changeset id: \t\t " << id << std::endl;
    std::cout << "Roads Added (km): \t " << road_km_added << std::endl;
    std::cout << "Roads Modified (km):\t " <<road_km_modified << std::endl;
    std::cout << "Waterways Added (km): \t " << waterway_km_added << std::endl;
    std::cout << "Waterways Modified (km): " << waterway_km_modified << std::endl;
    std::cout << "Roads Added: \t\t " << roads_added << std::endl;
    std::cout << "Roads Modified: \t " << roads_modified << std::endl;
    std::cout << "Waterways Added: \t " << waterways_added << std::endl;
    std::cout << "Waterways Modified: \t " << waterways_modified << std::endl;
    std::cout << "Buildings added: \t " << buildings_added << std::endl;
    std::cout << "Buildings Modified: \t " << buildings_modified << std::endl;
    std::cout << "POIs added: \t\t " << pois_added << std::endl;
    std::cout << "POIs Modified: \t\t " << pois_modified << std::endl;
    std::cout << "Editor: \t\t " << editor << std::endl;
    std::cout << "User ID: \t\t "  << user_id << std::endl;
    std::cout << "Created At: \t\t " << created_at << std::endl;
    std::cout << "Closed At: \t\t " << closed_at << std::endl;
    std::cout << "Verified: \t\t " << verified << std::endl;
    // std::cout << augmented_diffs << std::endl;
    std::cout << "Updated At: \t\t " << updated_at << std::endl;
}

}       // EOF osmstatsdb

