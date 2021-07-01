//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
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
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <algorithm>
#include <utility>

#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
//#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include "osmstats/replication.hh"
#include "osmstats/changeset.hh"
#include "osmstats/osmchange.hh"
#include "data/underpass.hh"

#include "log.hh"
using namespace logger;

namespace underpass {

// logger::LogFile& dbglogfile = logger::LogFile::getDefaultInstance();
// Underpass::Underpass(void)
// {
//     frequency_tags[replication::minutely] = "minute";
//     frequency_tags[replication::hourly] = "hour";
//     frequency_tags[replication::daily] = "day";
//     frequency_tags[replication::changeset] = "changeset";
// }

Underpass::Underpass(const std::string &dbname)
{
    if (dbname.empty()) {
        // Validate environment variable is defined.
        char *tmp = std::getenv("UNDERPASS_DB_URL");
        db_url = tmp;
    } else {
        db_url = "dbname = " + dbname;
    }
    connect(dbname);
};

Underpass::~Underpass(void)
{
    // db->disconnect();        // close the database connection
    if (sdb) {
        if (sdb->is_open()) {
            sdb->close();            // close the database connection
        }
    }
}

// Dump internal data to the terminal, used only for debugging
void
Underpass::dump(void)
{
    log_debug(_("Database url: %1%"), db_url);
}

bool
Underpass::connect(void)
{
    return connect("underpass");
}

bool
Underpass::connect(const std::string &dbname)
{
    if (dbname.empty()) {
	log_error(_("ERROR: need to specify database name!"));
    }
    try {
        std::string args = "dbname = " + dbname;
        sdb = std::make_shared<pqxx::connection>(args);
        if (sdb->is_open()) {
            log_debug(_("Opened database connection to %1%"), dbname);
            return true;
        } else {
            return false;
        }
    } catch (const std::exception &e) {
	log_error(_("Couldn't open database connection to %1% %2%"), dbname, e.what());
	return false;
    }
}

// Update the creator table to track editor statistics
bool
Underpass::updateCreator(long user_id, long change_id, const std::string &editor)
{
    std::string query = "INSERT INTO creators(user_id, change_id, editor) VALUES(";
    query += std::to_string(user_id) + "," + std::to_string(change_id);
    query += ",\'" + changeset::fixString(editor);
    query += "\') ON CONFLICT DO NOTHING;";
    log_debug(_("QUERY: %1%"), query);
#if 0
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();
#endif
    return false;
}

std::shared_ptr<replication::StateFile>
Underpass::getState(replication::frequency_t freq, const std::string &path)
{
    auto state = std::make_shared<replication::StateFile>();
    if (!sdb->is_open()) {
        log_error(_("database not connected!"));
        return state;
    }
    std::vector<std::string> nodes;
    std::string tmp;
    boost::split(nodes, path, boost::is_any_of("/"));
    if (nodes[0] == "https:") {
        tmp = nodes[5] + "/" + nodes[6] + "/" + nodes[7];
    } else {
        tmp = path;
    }
    //db_mutex.lock();
    std::string query = "SELECT timestamp,path,sequence,frequency FROM states WHERE path=\'";
    query += tmp + "\' AND frequency=\'" + frequency_tags[freq] + "\'";
    log_debug(_("QUERY: %1%"), query);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();
    if (result.size() > 0) {
        state->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        state->path = pqxx::to_string(result[0][1]);
        state->sequence = result[0][2].as(int(0));
        state->frequency =  pqxx::to_string(result[0][3]);
    }
    //db_mutex.unlock();
    return state;
}

// Get the state.txt date by timestamp
std::shared_ptr<replication::StateFile>
Underpass::getState(replication::frequency_t freq, ptime &tstamp)
{
    auto state = std::make_shared<replication::StateFile>();
    if (tstamp == boost::posix_time::not_a_date_time) {
        log_error(_("ERROR: bad timestamp!"));
        exit(1);
    }
    
    if (sdb > 0) {
        if (!sdb->is_open()) {
            log_error(_("database not connected!"));
            return state;
        }
    } else {
        log_error(_("database not connected!"));
        return state;
    }

    ptime other = tstamp + minutes(1);
    std::string query = "SELECT timestamp,path,created_at,closed_at FROM states WHERE timestamp BETWEEN ";
    query += "\'" + to_simple_string(tstamp) + "\' AND ";
    query += "\'" + to_simple_string(other) + "\' ";
    query += " AND frequency=";
    query += "\'" + frequency_tags[freq] + "\'";
    query += " ORDER BY timestamp ASC LIMIT 1;";
    log_debug(_("QUERY: %1%"), query);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();
    if (result.size() > 0) {
        // Sometimes these two fields are reversed
        try {
            state->timestamp = time_from_string(result[0][0].c_str());
            state->path = pqxx::to_string(result[0][1]);
        } catch (std::exception& e) {
            log_error(_("Couldn't parse StateFile %1%"), e.what());
            state->path = pqxx::to_string(result[0][0]);
            state->timestamp = time_from_string(result[0][1].c_str());
            state->created_at = time_from_string(pqxx::to_string(result[0][2]));
            state->closed_at = time_from_string(pqxx::to_string(result[0][3]));
        }
        state->sequence = result[0][2].as(int(0));
        state->frequency = freq;
    } else {
#if 0
        // FIXME: this does not work yet
        ptime start = time_from_string("2012-09-12 13:22");
        boost::posix_time::time_duration delta = tstamp - start;
        state->timestamp = tstamp;
        state->sequence = 0;
        state->path = "https://planet.openstreetmap.org/replication";
        state->path += "\'" + frequency_tags[freq] + "\'";
        boost::format fmt("%03d");
        int next;
        fmt % (next);
        state->path += fmt.str();
#endif
    }
    return state;
}

/// Write the stored data on the directories and timestamps
/// on the planet server.
bool
Underpass::writeState(replication::StateFile &state)
{
    std::string query;

    if (state.created_at != boost::posix_time::not_a_date_time) {
        query = "INSERT INTO states(timestamp, sequence, path, frequency, created_at, closed_at) VALUES(";
    } else {
        query = "INSERT INTO states(timestamp, sequence, path, frequency) VALUES(";
    }
    query += "\'" + to_simple_string(state.timestamp) + "\',";
    query += std::to_string(state.sequence);
    std::vector<std::string> nodes;
    boost::split(nodes, state.path, boost::is_any_of("/"));
    std::string tmp;
    if (nodes[0] == "https:") {
        tmp = nodes[5] + '/' + nodes[6] + '/' + nodes[7];
    } else {
        tmp = state.path;
    }
    
    query += ",\'" + tmp + "\'";
    if (state.path.find("changeset") != std::string::npos) {
        query += ", \'changeset\'";
    } else if (state.path.find("minute") != std::string::npos) {
        query += ", \'minute\'";
    } else if (state.path.find("hour") != std::string::npos) {
        query += ", \'hour\'";
    } else if (state.path.find("day") != std::string::npos) {
        query += ", \'day\'";
    }
    if (state.created_at != boost::posix_time::not_a_date_time) {
        query += ", \'" + to_simple_string(state.created_at) + "\'";
        query += ", \'" + to_simple_string(state.closed_at) + "\'";
    }
    query += ") ON CONFLICT DO NOTHING;";
    // log_debug(_("QUERY: " << query);
    //db_mutex.lock();
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    return false;
}

/// Get the maximum timestamp for the state.txt data
std::shared_ptr<replication::StateFile>
Underpass::getLastState(replication::frequency_t freq)
{
    pqxx::work worker(*sdb);
//    std::string query = "SELECT timestamp,sequence,path,frequency,created_at,closed_at FROM states";
    std::string query = "SELECT timestamp,sequence,path FROM states";
    query += " WHERE frequency=";
    if (freq == replication::changeset) {
        query += "\'changeset\'";
    } else if (freq == replication::minutely) {
        query += "\'minute\'";
    } else if (freq == replication::hourly) {
        query += "\'hour\'";
    } else if (freq == replication::daily) {
        query += "\'day\'";
    }
    query +=" ORDER BY timestamp DESC LIMIT 1;";
    log_debug(_("QUERY: %1%"), query);
    pqxx::result result = worker.exec(query);
    auto last = std::make_shared<replication::StateFile>();
    if (result.size() > 0) {
        last->timestamp = time_from_string(pqxx::to_string(result[0][0]));
        last->sequence = result[0][1].as(int(0));
        last->path = pqxx::to_string(result[0][2]);
        last->frequency = freq;
        //last->created_at = time_from_string(pqxx::to_string(result[0][3]));
        //last->closed_at = time_from_string(pqxx::to_string(result[0][4]));
    }

    worker.commit();

    return last;
}

// Get the minimum timestamp for the state.txt data. As hashtags didn't
// appear until late 2014, we don't care as much about the older data.
std::shared_ptr<replication::StateFile>
Underpass::getFirstState(replication::frequency_t freq)
{
    pqxx::work worker(*sdb);
    std::string query = "SELECT timestamp,sequence,path FROM states ORDER BY timestamp ASC LIMIT 1;";
    // log_debug(_("QUERY: %1%", query);
    pqxx::result result = worker.exec(query);
    auto first = std::make_shared<replication::StateFile>();
    first->timestamp = time_from_string(pqxx::to_string(result[0][0]));
    first->sequence = result[0][1].as(int(0));
    first->path = pqxx::to_string(result[0][2]);
    first->frequency = freq;
    // first->created_at = time_from_string(pqxx::to_string(result[0][3]));
    // first->closed_at = time_from_string(pqxx::to_string(result[0][4]));

    worker.commit();

    return first;
}

std::shared_ptr<osmstats::RawCountry>
Underpass::getCountry(double max_lat, double max_lon, double min_lat, double min_lon)
{
    std::string query = "SELECT cid,name,other_tags->\'name:iso_w2\' FROM geoboundaries WHERE ST_WITHIN(";
    query += "ST_MakePolygon(ST_GeomFromText(\'LINESTRING(";
    query += std::to_string(min_lon) + " " + std::to_string(max_lat);
    query += "," + std::to_string(max_lon) + " " + std::to_string(max_lat);
    query += "," + std::to_string(max_lon) + " " + std::to_string(min_lat);
    query += "," + std::to_string(min_lon) + " " + std::to_string(min_lat);
    query += "," + std::to_string(min_lon) + " " + std::to_string(max_lat);
    query += ")\', 4326)), wkb)";
    log_debug(_("QUERY: %1%"), query);
#if 0
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();
    if (result.size() > 0) {
        return std::make_shared<osmstats::RawCountry>(result[0][0].as(int(0)),
                                                      result[0][1].c_str(),
                                                      result[0][2].c_str());
    } else {
        return std::make_shared<osmstats::RawCountry>();
    }
#else
    return std::make_shared<osmstats::RawCountry>();
#endif
}

} // EOF replication namespace

