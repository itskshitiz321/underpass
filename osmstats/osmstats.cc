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
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <assert.h>
#include <pqxx/pqxx>
#include "pqxx/nontransaction"

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

using namespace boost::math;
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "osmstats/osmstats.hh"
#include "osmstats/changeset.hh"
#include "data/osmobjects.hh"
#include "data/underpass.hh"

using namespace apidb;

#include "log.hh"
using namespace logger;

/// \namespace osmstats
namespace osmstats {

QueryOSMStats::QueryOSMStats(void)
{
}

QueryOSMStats::QueryOSMStats(const std::string &dbname)
{
    if (dbname.empty()) {
        // Validate environment variable is defined.
        char *tmp = std::getenv("STATS_DB_URL");
        db_url = tmp;
    } else {
        db_url = "dbname = " + dbname;
    }
    connect(dbname);
};

bool
QueryOSMStats::connect(void)
{
    return connect("localhost/osmstats");
}

bool
QueryOSMStats::connect(const std::string &dburl)
{
    if (dburl.empty()) {
        log_error(_(" need to specify URL connection string!"));
    }

    std::string dbuser;
    std::string dbpass;
    std::string dbhost;
    std::string dbname = "dbname=";
    std::size_t apos = dburl.find('@');
    if (apos != std::string::npos) {
        dbuser = "user=";
        std::size_t cpos = dburl.find(':');
        if (cpos != std::string::npos) {
            dbuser += dburl.substr(0, cpos);
            dbpass = "password=";
            dbpass += dburl.substr(cpos+1, apos-cpos-1);
        } else {
            dbuser += dburl.substr(0, apos);
        }
    }

    std::vector<std::string> result;
    if (apos != std::string::npos) {
        boost::split(result, dburl.substr(apos+1), boost::is_any_of("/"));
    } else {
        boost::split(result, dburl, boost::is_any_of("/"));
    }
    if (result.size() == 1) {
        dbname += result[0];
        dbhost = "host=localhost";
    } else if (result.size() == 2) {
        if (result[0] != "localhost") {
            dbhost = "host=";
            dbhost += result[0];
        }
        dbname += result[1];
    }
    std::string args = dbhost + " " + dbname + " " + dbuser + " " + dbpass;
    // log_debug(args);

    try {
        sdb = std::make_shared<pqxx::connection>(args);
        if (sdb->is_open()) {
            log_debug(_("Opened database connection to %1%"), dburl);
            return true;
        } else {
            return false;
        }
    } catch (const std::exception &e) {
	    log_error(_(" Couldn't open database connection to %1% %2%"), dburl, e.what());
	    return false;
   }
}

// long
// QueryOSMStats::updateCounters(std::map<const std::string &, long> data)
// {
//     for (auto it = std::begin(data); it != std::end(data); ++it) {
//     }
// }

int
QueryOSMStats::lookupHashtag(const std::string &hashtag)
{
    std::string query = "SELECT id FROM taw_hashtags WHERE hashtag=\'";
    query += hashtag + "\';";
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    // There is only one value returned
    return result[0][0].as(int(0));
}

bool
QueryOSMStats::applyChange(osmchange::ChangeStats &change)
{
    std::cout << "Applying OsmChange data" << std::endl;

    if (hasHashtag(change.change_id)) {
        std::cout << "Has hashtag for id: " << change.change_id << std::endl;
    } else {
        std::cerr << "No hashtag for id: " << change.change_id << std::endl;
    }

    std::string ahstore;
    if (change.added.size() > 0) {
        ahstore = "HSTORE(ARRAY[";
        for (auto it = std::begin(change.added); it != std::end(change.added); ++it) {
            if (it->first.empty()) {
                return true;
            }
            if (it->second > 0) {
                ahstore += " ARRAY[\'" + it->first + "\',\'" + std::to_string((int)it->second) +"\'],";
            }
        }
        ahstore.erase(ahstore.size() - 1);
        ahstore += "])";
    } else {
        ahstore.clear();
    }
    if (change.modified.size() > 0) {
        ahstore = "HSTORE(ARRAY[";
        for (auto it = std::begin(change.modified); it != std::end(change.modified); ++it) {
            if (it->first.empty()) {
                return true;
            }
            if (it->second > 0) {
                ahstore += " ARRAY[\'" + it->first + "\',\'" + std::to_string(it->second) +"\'],";
            }
        }
        ahstore.erase(ahstore.size() - 1);
        ahstore += "])";
    }

    // Some of the data field in the changset come from a different file,
    // which may not be downloaded yet.
    ptime now = boost::posix_time::microsec_clock::local_time();
    std::string aquery;
    if (ahstore.size() > 0) {
        if (change.added.size() > 0) {
            aquery = "INSERT INTO changesets (id, user_id, closed_at, updated_at, added)";
        } else if (change.modified.size() > 0) {
            aquery = "INSERT INTO changesets (id, user_id, closed_at, updated_at, modified)";
        }
    } else {
        aquery = "INSERT INTO changesets (id, user_id, updated_at)";
    }
    aquery += " VALUES(" + std::to_string(change.change_id) + ", ";
    aquery += std::to_string(change.user_id) + ", ";
    aquery += "\'" + to_simple_string(change.closed_at) + "\', ";
    aquery += "\'" + to_simple_string(now) + "\', ";
    if (ahstore.size() > 0) {
        if (change.added.size() > 0) {
            aquery += ahstore + ") ON CONFLICT (id) DO UPDATE SET added = " + ahstore + ",";
        } else {
            aquery += ahstore + ") ON CONFLICT (id) DO UPDATE SET modified = " + ahstore + ",";
        }
    } else {
        aquery.erase(aquery.size() - 2);
        aquery += ") ON CONFLICT (id) DO UPDATE SET";
    }

    aquery += " closed_at = \'" + to_simple_string(change.closed_at) + "\',";
    aquery += " updated_at = \'" + to_simple_string(now) + "\'";
    aquery += " WHERE changesets.id=" + std::to_string(change.change_id);

    // log_debug(_("QUERY stats: %1%"), aquery);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(aquery);

    worker.commit();

    // FIXME: return something
}

bool
QueryOSMStats::applyChange(changeset::ChangeSet &change)
{
    log_debug(_("Applying ChangeSet data"));
    change.dump();

    // Some old changefiles have no user information
    std::string query = "INSERT INTO users VALUES(";
    query += std::to_string(change.uid) + ",\'" + sdb->esc(change.user);
    query += "\') ON CONFLICT DO NOTHING;";
    // log_debug(_("QUERY: %1%"), query);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    //worker.commit();

    // If there are no hashtags in this changset, then it isn't part
    // of an organized map campaign, so we don't need to store those
    // statistics except for editor usage.
#if 0
    underpass::Underpass under;
    under.connect();
    if (change.hashtags.size() == 0) {
        log_debug(_("No hashtags in change id: %1%"), change.id);
        under.updateCreator(change.uid, change.id, change.editor);
        worker.commit();
        // return true;
    } else {
        log_debug(_("Found hashtags for change id: %1%"), change.id);
    }
#endif
    // Add changeset data
    // road_km_added | road_km_modified | waterway_km_added | waterway_km_modified | roads_added | roads_modified | waterways_added | waterways_modified | buildings_added | buildings_modified | pois_added | pois_modified | updated_at
    // the updated_at timestamp is set after the change data has been processed

    // osmstats=# UPDATE raw_changesets SET road_km_added = (SELECT road_km_added + 10.0 FROM raw_changesets WHERE road_km_added>0 AND user_id=649260 LIMIT 1) WHERE user_id=649260;

    query = "INSERT INTO changesets (id, editor, user_id, created_at";
    if (change.hashtags.size() > 0) {
        query += ", hashtags ";
    }
    if (!change.source.empty()) {
        query += ", source ";
    }
    query += ", bbox) VALUES(";
    query += std::to_string(change.id) + ",\'" + change.editor + "\',\'";\
    query += std::to_string(change.uid) + "\',\'";
    query += to_simple_string(change.created_at) + "\'";
    // if (!change.open) {
    //     query += ",\'" + to_simple_string(change.closed_at) + "\'";
    // }
    // Hashtags are only used in mapping campaigns using Tasking Manager
    if (change.hashtags.size() > 0) {
        query += ", \'{ ";
        for (auto it = std::begin(change.hashtags); it != std::end(change.hashtags); ++it) {
            boost::algorithm::replace_all(*it, "\"", "&quot;");
            query += "\"" + sdb->esc(*it) + "\"" + ", ";
        }
        // drop the last comma
        query.erase(query.size() - 2);
        query += " }\'";
    }
    // The source field is not always present
    if (!change.source.empty()) {
        query += ",\'" + change.source += "\'";
    }

    // Store the current values as they may get changed to expand very short
    // lines or POIs so they have a bounding box big enough for postgis to use.
    double min_lat = change.min_lat;
    double max_lat = change.max_lat;
    double min_lon = change.min_lon;
    double max_lon = change.max_lon;
    // FIXME: There are bugs in the bounding box coordinates in some of the older
    // changeset files, but for now ignore these.
    double fff = relative_difference(max_lon, min_lon);
    //std::cout << "FIXME: Float diff " << fff << std::endl;
    int fudge = 0.0001;
    if (fff < fudge) {
        log_debug(_("FIXME: line too short! "), fff);
        return false;
    }
    // a changeset with a single node in it doesn't draw a line
    if (change.max_lon < 0 && change.min_lat < 0) {
        // log_error(_("WARNING: single point! %1%"), change.id);
        min_lat = change.min_lat + (fudge/2);
        max_lat = change.max_lat + (fudge/2);
        min_lon = change.min_lon - (fudge/2);
        max_lon = change.max_lon - (fudge/2);
        //return false;
    }
    if (max_lon == min_lon || max_lat == min_lat) {
        // log_error(_("WARNING: not a line! %1%"), change.id);
        min_lat = change.min_lat + (fudge/2);
        max_lat = change.max_lat + (fudge/2);
        min_lon = change.min_lon - (fudge/2);
        max_lon = change.max_lon - (fudge/2);
        // return false;
    }
    if (max_lon < 0 && min_lat < 0) {
        log_error(_("WARNING: single point! "), change.id);
        min_lat = change.min_lat + (fudge/2);
        max_lat = change.max_lat + (fudge/2);
        min_lon = change.min_lon - (fudge/2);
        max_lon = change.max_lon - (fudge/2);
        // return false;
    }
    if (change.num_changes == 0) {
        log_error(_("WARNING: no changes! "), change.id);
        return false;
    }
    // Add the bounding box of the changeset here next
        // long_high,lat_high,
        // long_low,lat_high,
        // long_low,lat_low,
        // long_high,lat_low,
        // long_high,lat_high
    std::string bbox;
    bbox += ", ST_MULTI(ST_GeomFromEWKT(\'SRID=4326;POLYGON((";
    // Upper left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Upper right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Lower right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Lower left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Close the polygon
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ")";

    query += bbox;

    query += ")\')";
    //query += ")) ON CONFLICT DO NOTHING;";
    query += ")) ON CONFLICT (id) DO UPDATE SET editor=\'" + change.editor;
    query += "\', created_at=\'" + to_simple_string(change.created_at);
    // if (!change.open) {
    // 	query += "\', closed_at=\'" + to_simple_string(change.closed_at);
    // }
    query += "\', bbox=" + bbox.substr(2) + ")'))";
    log_debug(_("QUERY: %1%"), query);
    result = worker.exec(query);

    // Commit the results to the database
    worker.commit();

    return true;
}

bool
QueryOSMStats::hasHashtag(long changeid)
{
#if 0
    std::string query = "SELECT COUNT(hashtag_id) FROM changesets_hashtags WHERE changeset_id=" + std::to_string(changeid) + ";";
    log_debug(_("QUERY: %1%"), query);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    if (result[0][0].as(int(0)) > 0 ) {
        return true;
    }
#endif
    return false;
}

// Get the timestamp of the last update in the database
ptime
QueryOSMStats::getLastUpdate(void)
{
    std::string query = "SELECT MAX(created_at) FROM changesets;";
    // log_debug(_("QUERY: %1%"), query);
    // auto worker = std::make_shared<pqxx::work>(*db);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    ptime last;
    if (result[0][0].size() > 0) {
        last = time_from_string(result[0][0].c_str());
        return last;
    }
    return not_a_date_time;
}

void
RawChangeset::dump(void)
{
    log_debug("-----------------------------------");
    log_debug(_("changeset id: \t\t %1%"), std::to_string(id));
    log_debug(_("Editor: \t\t %1%"), editor);
    log_debug(_("User ID: \t\t %1%"), std::to_string(user_id));
    log_debug(_("Created At: \t\t %1%"), to_simple_string(created_at));
    log_debug(_("Closed At: \t\t %1%"), to_simple_string(closed_at));
    log_debug(_("Verified: \t\t %1%"), verified);
    //log_debug(_("Updated At: \t\t %1%"). to_simple_string(updated_at));
}

}       // EOF osmstatsdb

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
