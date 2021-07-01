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
#include <iostream>
#include <mutex>
#include <range/v3/all.hpp>
#include <algorithm>
#include <iterator>
#include <thread>
#include <fstream>
#include <future>
#include <unistd.h>

#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;   // from <boost/asio/ssl.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

#include "timer.hh"
#include "data/threads.hh"
#include "osmstats/osmstats.hh"
#include "osmstats/osmchange.hh"
#include "osmstats/changeset.hh"
#include "osmstats/replication.hh"
#include "data/underpass.hh"
#include "data/validate.hh"
#include "log.hh"

std::mutex stream_mutex;

using namespace logger;

namespace threads {

// logger::LogFile& dbglogfile = logger::LogFile::getDefaultInstance();

// Starting with this URL, download the file, incrementing
void
startMonitor(const replication::RemoteURL &inr, const multipolygon_t &poly)
{
    underpass::Underpass under;
    under.connect();

    replication::RemoteURL remote = inr;
    auto planet = std::make_shared<replication::Planet>(remote);
    bool mainloop = true;
    while (mainloop) {
        // Look for the statefile first
#if 0
	std::shared_ptr<replication::StateFile> exists;
	exists = under.getState(remote.frequency, remote.subpath);
	if (exists) {
	    log_warning(_("Already stored in database: %1%"), remote.subpath);
	    break;
	} else {
	    log_info(_("Downloading StateFile: %1% %2%"), remote.subpath ".state.txt");
	    auto state = threadStateFile(planet->stream, remote.subpath + ".state.txt");
	    if (state->timestamp != boost::posix_time::not_a_date_time && (state->sequence != 0 && state->path.size() != 0)) {
		state->dump();
		under.writeState(*state);
		break;
	    }
        }
#endif
	remote.dump();
        if (remote.frequency == replication::changeset) {
            Timer timer;
            timer.startTimer();
            auto found = threadChangeSet(remote, poly);
            if (!found) {
                // planet->disconnectServer();
		std::this_thread::sleep_for(std::chrono::minutes{1});
                // planet.reset(new replication::Planet);
            } else {
                log_debug(_("Processed ChangeSet: %1%"), remote.url);
            }
            timer.endTimer("changeSet");
        } else {
            std::string file = remote.url + ".osc.gz";
            Timer timer;
            timer.startTimer();
            bool found = threadOsmChange(remote, poly);
            if (!found) {
                // planet->disconnectServer();
                if (remote.frequency == replication::minutely) {
                    std::this_thread::sleep_for(std::chrono::minutes{1});
                } else if (remote.frequency == replication::hourly) {
                    std::this_thread::sleep_for(std::chrono::hours{1});
                } else if (remote.frequency == replication::daily) {
                    std::this_thread::sleep_for(std::chrono::hours{24});
                }
                // planet.reset(new replication::Planet);
            } else {
                log_debug(_("Processed OsmChange: %1%"), remote.url);
            }
            timer.endTimer("osmChange");
        }
	remote.Increment();
	remote.dump();
        //planet->endTimer("change file");
    }
}

void
startStateThreads(const std::string &base, const std::string &file)
{
    // std::map<std::string, std::thread> thread_pool;

    //return;                     // FIXME:
    
    // boost::system::error_code ec;
    // underpass::Underpass under;
    // under.connect();
    // auto planet = std::make_shared<replication::Planet>();
    // planet->connectServer();

// #if 1
//     for (auto it = std::begin(files); it != std::end(files); ++it) {
//         // There are no state,txt file before this directory
//         // https://planet.openstreetmap.org/replication/changesets/002/008

//         //  state = [planet](const std::string &path)->bool {
//         std::string path = base + it->substr(0, 3);
//         std::shared_ptr<replication::StateFile> state = threadStateFile(planet->stream, path + ".state.txt");
//         if (!state->path.empty()) {
//             under.writeState(*state);
//             state->dump();
//         } else {
//             std::cerr << "ERROR: No StateFile returned: " << path << std::endl;
//             // planet.reset(new replication::Planet);
//             // planet.reset(new replication::Planet());
//             std::this_thread::sleep_for(std::chrono::seconds{1});
//             state = threadStateFile(planet->stream, path + ".state.txt");
//             if (!state->path.empty()) {
//                 under.writeState(*state);
//                 state->dump();
//             }
//         }
//     }
// #else
//     // boost::asio::thread_pool pool(20);
//     boost::asio::thread_pool pool(/* std::thread::hardware_concurrency() */ );

//     // Note this uses ranges, which only got added in C++20, so
//     // for now use the ranges-v3 library, which is the implementation.
//     // The planet server drops the network connection after 111
//     // GET requests, so break the 1000 strings into smaller chunks
//     // 144, 160, 176, 192, 208, 224
//     auto rng  = files | ranges::views::chunk(200);

//     // underpass::Underpass under;
//     // under.connect();
//     //Timer timer;
//     //timer.startTimer();
//     for (auto cit = std::begin(rng); cit != std::end(rng); ++cit) {
//         log_debug(_("Chunk data: %1%"), *cit));
//         for (auto it = std::begin(*cit); it != std::end(*cit); ++it) {
//             // There are no state,txt file before this directory
//             // https://planet.openstreetmap.org/replication/changesets/002/008
//             if (boost::filesystem::extension(*it) != ".txt") {
//                 continue;
//             }
//             std::string subpath = base + it->substr(0, it->size() - 10);
//             auto exists = under.getState(subpath);
//             if (!exists->path.empty()) {
//                 log_debug(_("Already stored: %1%"), subpath);
//                 continue;
//             }
//             // Add a thread to the pool for this file
//             if (!it->size() <= 1) {
// #ifdef USE_MULTI_LOADER
//                 boost::asio::post(pool, [subpath, state]{state(subpath);});
// #else
//                 auto state = threadStateFile(planet->stream, base + *it);
//                 if (!state->path.empty()) {
//                     // under.writeState(*state);
//                     state->dump();
//                     continue;
//                 } else {
//                     log_error(_("No StateFile returned"));
//                 }
// #endif
//             }
//         }
//         //timer.endTimer("chunk ");
//         // Don't hit the server too hard while testing, it's not polite
//         // std::this_thread::sleep_for(std::chrono::seconds{1});
//         planet->disconnectServer();
//         planet.reset(new replication::Planet);
//     }
// #ifdef USE_MULTI_LOADER
//     pool.join();
// #endif
//     planet->ioc.reset();
// #endif
//     // planet->stream.socket().shutdown(tcp::socket::shutdown_both, ec);
//     //timer.endTimer("directory ");
}

// This thread get started for every osmChange file
bool
threadOsmChange(const replication::RemoteURL &remote, const multipolygon_t &poly)
{
    // osmstats::QueryOSMStats ostats;
    std::vector<std::string> result;
    osmchange::OsmChangeFile osmchanges;

    auto data = std::make_shared<std::vector<unsigned char>>();
    // If the file is stored on disk, read it in instead of downloading
    if (boost::filesystem::exists(remote.filespec)) {
        log_debug(_("Reading osmChange: %1%"), remote.filespec);
        // Since we want to read in the entire file so it can be
        // decompressed, blow off C++ streaming and just load the
        // entire thing.
        int size = boost::filesystem::file_size(remote.filespec);
        data->reserve(size);
        data->resize(size);
        int fd = open(remote.filespec.c_str(), O_RDONLY);
        char *buf = new char[size];
        //memset(buf, 0, size);
        read(fd, buf, size);
        // FIXME: it would be nice to avoid this copy
        std::copy(buf, buf+size, data->begin());
        close(fd);
    } else {
        log_debug(_("Downloading osmChange: %1%"), remote.url);
 	replication::Planet planet(remote);
	data = planet.downloadFile(remote.url);
    }
    if (data->size() == 0) {
        log_error(_("osmChange file not found: %1% %2%"), remote.url, ".osc.gz");
        return false;
    } else {
#ifdef USE_CACHE
        if (!boost::filesystem::exists(remote.destdir)) {
	    boost::filesystem::create_directories(remote.destdir);
	}
	std::ofstream myfile;
	myfile.open(remote.filespec, std::ios::binary);
	myfile.write(reinterpret_cast<char *>(data->data()), data->size()-1);
	myfile.close();
#endif
        try {
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            boost::iostreams::array_source arrs{reinterpret_cast<char const*>(data->data()), data->size()};
            inbuf.push(arrs);
            std::istream instream(&inbuf);
            try {
                osmchanges.readXML(instream);
            } catch (std::exception& e) {
                log_error(_("Couldn't parse: %1%"), remote.url);
                std::cerr << e.what() << std::endl;
                // return false;
            }
            // change.readXML(instream);
        } catch (std::exception& e) {
            log_error(_("%1% is corrupted!"), remote.url);
            std::cerr << e.what() << std::endl;
            // return false;
        }
    }

    // Apply the changes to the database
    osmstats::QueryOSMStats ostats;
    ostats.connect();
#if 0
    underpass::Underpass under;
    under.connect();
    replication::StateFile state;
    for (auto it = std::begin(osmchanges.changes); it != std::end(osmchanges.changes); ++it) {
	//state.created_at = it->created_at;
	//state.closed_at = it->closed_at;
	state.frequency = replication::changeset;
	state.path = file;
	under.writeState(state);
    }
#endif
    // These stats are for the entire file
    auto stats = osmchanges.collectStats(poly);
    for (auto it = std::begin(*stats); it != std::end(*stats); ++it) {
        it->second->dump();
        ostats.applyChange(*it->second);
    }

    validate::Validate validator(osmchanges.changes);

    return true;
}

// This updates several fields in the changesets table, which are part of
// the changeset file, and don't need to be calculated.
//void threadChangeSet(const std::string &file, std::promise<bool> &&result)
std::shared_ptr<replication::StateFile>
threadChangeSet(const replication::RemoteURL &remote, const multipolygon_t &poly)
{
    changeset::ChangeSetFile changeset;

    auto state = std::make_shared<replication::StateFile>();
    auto data = std::make_shared<std::vector<unsigned char>>();
    // FIXME: this this be the datadir from the command line

    if (boost::filesystem::exists(remote.filespec)) {
        log_debug(_("Reading ChangeSet: %1%"), remote.filespec);
        // Since we want to read in the entire file so it can be
        // decompressed, blow off C++ streaming and just load the
        // entire thing.
        int size = boost::filesystem::file_size(remote.filespec);
        data->reserve(size);
        data->resize(size);
        int fd = open(remote.filespec.c_str(), O_RDONLY);
        char *buf = new char[size];
        //memset(buf, 0, size);
        read(fd, buf, size);
        // FIXME: it would be nice to avoid this copy
        std::copy(buf, buf+size, data->begin());
        close(fd);
    } else {
	log_debug(_("Downloading ChangeSet: %1%"), remote.url);
	replication::Planet planet(remote);
        data = planet.downloadFile(remote.url);
    }
    if (data->size() == 0) {
        log_error(_("ChangeSet file not found: %1%"), remote.url);
        //result.set_value(false);
        return state;
    } else {
        //result.set_value(true);
        // XML parsers expect every line to have a newline, including the end of file
#ifdef USE_CACHE
        if (!boost::filesystem::exists(remote.destdir)) {
	    boost::filesystem::create_directories(remote.destdir);
	}
	std::ofstream myfile;
	myfile.open(remote.filespec, std::ios::binary);
	myfile.write(reinterpret_cast<char *>(data->data()), data->size()-1);
	myfile.close();
#endif
        //data->push_back('\n');
        try {
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            // data->push_back('\n');
            boost::iostreams::array_source arrs{reinterpret_cast<char const*>(data->data()), data->size()};
            inbuf.push(arrs);
            std::istream instream(&inbuf);
            try {
                changeset.readXML(instream);
            } catch (std::exception& e) {
                log_error(_("Couldn't parse: %1% %2%"), remote.url, e.what());
                // return false;
            }
            // change.readXML(instream);
        } catch (std::exception& e) {
	    log_error(_("%1% is corrupted!"), remote.url);
            // return false;
        }
    }

    // Create a stubbed state file to update the underpass database with more
    // accurate timestamps, also used if there is no state.txt file.
    if (changeset.changes.size() > 0) {
        state->timestamp = changeset.changes.begin()->created_at;
        state->created_at = changeset.changes.begin()->created_at;
        state->closed_at = changeset.changes.end()->created_at;
    }
    
    return state;
}

// This updates the calculated fields in the raw_changesets table, based on
// the data in the OSM stats database.
void
threadStatistics(const std::string &database, ptime &timestamp)
{
    //osmstats::QueryOSMStats ostats;
    replication::Replication repl;
}

// Updates the states table in the Underpass database
std::shared_ptr<replication::StateFile>
threadStateFile(ssl::stream<tcp::socket> &stream, const std::string &file)
{
    std::string server;

    std::vector<std::string> result;
    boost::split(result, file, boost::is_any_of("/"));
    server = result[2];

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;
    boost::beast::error_code ec;

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, file, 11};

    req.keep_alive();
    req.set(http::field::host, server);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    
    log_debug(_("(%1%)Downloading %2%"), std::this_thread::get_id(), file);

    // Stays locked till the function exits
    const std::lock_guard<std::mutex> lock(stream_mutex);

    // Send the HTTP request to the remote host
    // std::lock_guard<std::mutex> guard(stream_mutex);
    boost::beast::http::response_parser<http::string_body> parser;

    http::write(stream, req);
    boost::beast::http::read(stream, buffer, parser, ec);
    if (ec == http::error::partial_message) {
        log_network(_("ERROR: partial read: %1%"), ec.message());
        std::this_thread::yield();
        http::write(stream, req);
        boost::beast::http::read(stream, buffer, parser, ec);
        // Give the network a chance to recover
        // std::this_thread::sleep_for(std::chrono::seconds{1});
        //return std::make_shared<replication::StateFile>();
    }
    if (ec == http::error::end_of_stream) {
        log_error(_("end of stream read failed: %1%"), ec.message());
        // Give the network a chance to recover
        // stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return std::make_shared<replication::StateFile>();
    } else if (ec) {
        log_network(_("ERROR: stream read failed: %1%"), ec.message());
        return std::make_shared<replication::StateFile>();
    }
    if (parser.get().result() == boost::beast::http::status::not_found) {
        // continue;
    }

    // File never downloaded, return empty
    if (parser.get().body().size() < 10) {
        log_error(_("failed to download: %1%"), file);
        return std::make_shared<replication::StateFile>();
    }

    //const std::lock_guard<std::mutex> unlock(stream_mutex);
    auto data = std::make_shared<std::vector<unsigned char>>();
    for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
        data->push_back((unsigned char)*body);
    }
    if (data->size() == 0) {
        log_error(_("StateFile not found: %1%"), file);
        return std::make_shared<replication::StateFile>();
    } else {
        std::string tmp(reinterpret_cast<const char *>(data->data()));
        auto state = std::make_shared<replication::StateFile>(tmp, true);
        if (!file.empty()) {
            state->path = file.substr(0, file.size() - 10);
        }
        return state;
    }
}

}       // EOF namespace threads

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
