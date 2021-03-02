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
#include <cstdlib>
#include <sstream>
#include <vector>
#include <cassert>
#include <deque>
#include <list>
#include <thread>
#include <tuple>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace boost;
namespace opts = boost::program_options;

// #include "hotosm.hh"
#include "osmstats/changeset.hh"
// #include "osmstats/osmstats.hh"
#include "data/geoutil.hh"
// #include "osmstats/replication.hh"
#include "data/import.hh"
#include "data/threads.hh"
#include "data/underpass.hh"
#include "timer.hh"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

enum pathMatches{ROOT, DIRECTORY, SUBDIRECTORY, FILEPATH};

// Forward declarations
namespace changeset {
  class ChangeSet;
};

/// A helper function to simplify the main part.
template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(os, " "));
    return os;
}

/// \class Replicator
/// \brief This class does all the actual work
///
/// This class identifies, downloads, and processes a replication file.
/// Replication files are available from the OSM planet server.
class Replicator : public replication::Replication
{
public:
    /// Create a new instance, and read in the geoboundaries file.
    bool initializeData(void) {
        auto hashes = std::make_shared<std::map<std::string, int>>();
        auto geou = std::make_shared<geoutil::GeoUtil>();
        // FIXME: this shouldn't be hardcoded
        geou->readFile("../underpass.git/data/geoboundaries.osm", true);
        changes = std::make_shared<changeset::ChangeSetFile>();
        changes->setupBoundaries(geou);

        under.connect();

        // Connect to the OSM Stats database
        ostats.connect("mystats");

        // FIXME: should return a real value
        return false;
    };
    
    /// Initialize the raw_user, raw_hashtags, and raw_changeset tables
    /// in the OSM stats database from a changeset file
    bool initializeRaw(std::vector<std::string> &rawfile, const std::string &database) {
        for (auto it = std::begin(rawfile); it != std::end(rawfile); ++it) {
            changes->importChanges(*it);
        }
        // FIXME: return a real value
        return false;
    };

    /// Get the value for a hashtag
    int lookupHashID(const std::string &hash) {
        auto found = hashes->find(hash);
        if (found != hashes->end()) {
            return (*hashes)[hash];
        } else {
            int id = ostats.lookupHashtag(hash);
            return id;
        }

        return 0;
    };
    /// Get the numeric ID of the country by name
    long lookupCountryID(const std::string &country) {
        return geou->getCountry(country).getID();
    };

    std::string getLastPath(replication::frequency_t interval) {
        ptime last = ostats.getLastUpdate();
        under.connect();
        auto state = under.getState(interval, last);
        return state->path;
    };
    // osmstats::RawCountry & findCountry() {
    //     geou.inCountry();


    enum pathMatches matchUrl(const std::string &url) {
        boost::regex test{"([0-9]{3})"};

        boost::sregex_token_iterator iter(url.begin(), url.end(), test, 0);
        boost::sregex_token_iterator end;

        std::vector<std::string> dirs;

        for( ; iter != end; ++iter ) {
            dirs.push_back(*iter);
        }

        pathMatches match;
        switch(dirs.size()) {
            default:
                match = pathMatches::ROOT;
                break;
            case 1:
                match = pathMatches::DIRECTORY;
                break;
            case 2:
                match = pathMatches::SUBDIRECTORY;
                break;
            case 3:
                match = pathMatches::FILEPATH;
                break;
        }

        return match;
    }

private:
    underpass::Underpass under;
    osmstats::QueryOSMStats ostats;                    ///< OSM Stats database access
    std::shared_ptr<changeset::ChangeSetFile> changes; ///< All the changes in the file
    std::shared_ptr<geoutil::GeoUtil> geou;             ///< Country boundaries
    std::shared_ptr<std::map<std::string, int>> hashes; ///< Existing hashtags
};

int
main(int argc, char *argv[])
{
    // Store the file names for replication files
    std::string changeset;
    std::string osmchange;
    // The update frequency between downloads
    std::string interval = "minutely";
    //bool encrypted = false;
    long sequence = 0;
    ptime timestamp(not_a_date_time);

    opts::positional_options_description p;
    opts::variables_map vm;
     try {
        opts::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "display help")
            ("server,s", "database server (defaults to localhost)")
            ("statistics,s", "OSM Stats database name (defaults to osmstats)")
            ("url,u", opts::value<std::string>(), "Starting URL")
            ("monitor,m", opts::value<std::string>(), "Starting URL to monitor")
            ("frequency,f", opts::value<std::string>(), "Update frequency (hour, daily), default minute)")
            ("timestamp,t", opts::value<std::string>(), "Starting timestamp")
            ("changeset,c", opts::value<std::vector<std::string>>(), "Initialize OSM Stats with changeset")
            ("osmchange,o", opts::value<std::vector<std::string>>(), "Apply osmchange to OSM Stats")
            ("import,i", opts::value<std::string>(), "Initialize pgsnapshot database with datafile")
//            ("osm,o", "OSM database name (defaults to pgsnapshot)")
//            ("url,u", opts::value<std::string>(), "Replication File URL")
//            ("encrypted,e", "enable HTTPS (the default)")
//            ("format,f", "database format (defaults to pgsnapshot)")
//            ("verbose,v", "enable verbosity")
            ;
        
        opts::store(opts::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        opts::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: options_description [options]" << std::endl;
            std::cout << desc;
            return 0;
        }
     } catch(std::exception& e) {
         std::cout << e.what() << std::endl;
         return 1;
     }

     Replicator replicator;
     replicator.initializeData();
     std::vector<std::string> rawfile;
     std::shared_ptr<std::vector<unsigned char>> data;

     // A changeset has the hashtags and comments we need. Multiple URLS
     // or filename may be specified on the command line, common when
     // catching up on changes.
     if (vm.count("changeset")) {
         std::vector<std::string> files = vm["changeset"].as<std::vector<std::string>>();
         if (files[0].substr(0, 4) == "http") {
             data = replicator.downloadFiles(files, true);
         } else {
             for (auto it = std::begin(files); it != std::end(files); ++it) {
                 replicator.readChanges(*it);
             }
         }
     }
     if (vm.count("osmchange")) {
         std::vector<std::string> files = vm["osmchange"].as<std::vector<std::string>>();
         if (files[0].substr(0, 4) == "http") {
             data = replicator.downloadFiles(files, false);
         } else {
             for (auto it = std::begin(files); it != std::end(files); ++it) {
                 replicator.readChanges(*it);
             }
         }
     }
     if (vm.count("sequence")) {
         std::cout << "Sequence is " << vm["sequence"].as<int>() << std::endl;
     }

     replication::Planet planet;
     underpass::Underpass under;
     if (vm.count("monitor")) {
        ptime tstamp(not_a_date_time);

        if (vm.count("timestamp")) {
            std::string timestamp = vm["timestamp"].as<std::string>();
            try {
                tstamp = time_from_string(timestamp);
            } catch (const std::exception& e) {
                std::cerr << "Invalid timestamp" << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            osmstats::QueryOSMStats ostats("osmstats");
            tstamp = ostats.getLastUpdate();
        }

         auto state = under.getState(replication::minutely, tstamp);
         std::string last;
         if (state->sequence == 0) {
             last = planet.findData(replication::minutely, tstamp);
         } else {
             last = state->path;
         }
         std::cout << "Last minutely is " << last  << std::endl;
         std::thread mstate (threads::startMonitor, std::ref(last));
         //threads::startMonitor(std::ref(last));
         //url = "https://planet.openstreetmap.org/replication/hour/004/308/210";
         // std::string last = replicator::getLastPath(replication::hourly);
         // std::thread hstate (threads::startMonitor, std::ref(url));
         auto state2 = under.getState(replication::changeset, tstamp);
         std::string clast;
         if (state2->sequence == 0 || state2->timestamp.is_not_a_date_time()) {
             clast = planet.findData(replication::changeset, tstamp);
         } else {
             clast = state2->path;
         }
         std::cout << "Last changeset is " << clast  << std::endl;
         std::thread cstate (threads::startMonitor, std::ref(clast));
         // threads::startMonitor(url);
         std::cout << "Waiting..." << std::endl;
         cstate.join();
         mstate.join();
         // hstate.join();
     }

    if (vm.count("url")) {
        std::string url = vm["url"].as<std::string>();
        int match = replicator.matchUrl(url);

        // Trailing slash check.
        if (url.back() != '/' && match != pathMatches::FILEPATH) {
          url += "/";
        }
        auto links = planet.scanDirectory(url);

        Timer timer;
        switch (match) {
            default: // Root
            {
                for (auto it = std::begin(*links); it != std::end(*links); ++it) {
                    if (it->empty()) {
                        continue;
                    }

                    std::cout << *it << std::endl;
                    std::string subUrl = url + *it;
                    auto sublinks = planet.scanDirectory(subUrl);
                    std::thread tstate (threads::startStateThreads, std::ref(subUrl),
                                        std::ref(*sublinks));
                    std::cout << "Waiting..." << std::endl;
                    tstate.join();
                    timer.endTimer();
               }
            }
            break;
            case pathMatches::DIRECTORY:
            {
                for (auto sit = std::begin(*links); sit != std::end(*links); ++sit) {
                    if (sit->empty()) {
                        continue;
                    }
                    timer.startTimer();
                    std::string subdir = url + *sit;
                    
                    std::cout << "Sub Directory: " << subdir << std::endl;
                    auto flinks = planet.scanDirectory(subdir);
                    std::thread tstate (threads::startStateThreads, std::ref(subdir),
                                         std::ref(*flinks));

                    std::cout << "Waiting..." << std::endl;
                    tstate.join();
                    timer.endTimer();
                    continue;
                }
            }
            break;

            case pathMatches::SUBDIRECTORY:
            {
                std::thread tstate (threads::startStateThreads, std::ref(url),
                    std::ref(*links));
                std::cout << "Waiting for startStateThreads()..." << std::endl;
                tstate.join();
                timer.endTimer();
            }
            break;

            case pathMatches::FILEPATH:
                threads::startStateThreads(std::ref(url), std::ref(*links));
                timer.endTimer();
            break;
        }
    }

     //replicator.startTimer();
     if (vm.count("timestamp")) {
         std::cout << "Timestamp is: " << vm["timestamp"].as<std::string> () << "\n";
         timestamp = time_from_string(vm["timestamp"].as<std::string>());
#if 0
         std::string foo = planet.findData(replication::minutely, timestamp);
         std::cout << "Full path: " << foo << std::endl;
         data = replicator.downloadFiles("https://planet.openstreetmap.org/replication/minute" + foo + "/000/000.state.txt", true);
         if (data->empty()) {
             std::cout << "File not found" << std::endl;
             exit(-1);
         }
         std::string tmp(reinterpret_cast<const char *>(data->data()));
         replication::StateFile state(tmp, true);
         state.dump();
#endif
     }

     std::string statistics;
     if (vm.count("initialize")) {
         rawfile = vm["initialize"].as<std::vector<std::string>>();
         replicator.initializeRaw(rawfile, statistics);
     }
     std::string osmdb;
     if (vm.count("osm")) {
         osmdb = vm["osm"].as<std::vector<std::string>>()[0];
     }
     if (vm.count("import")) {
         std::string file = vm["import"].as<std::string>();
         import::ImportOSM osm(file, osmdb);
     }
     // FIXME: add logging
     if (vm.count("verbose")) {
         std::cout << "Verbosity enabled.  Level is " << vm["verbose"].as<int>() << std::endl;
     }

     if (sequence > 0 and !timestamp.is_not_a_date_time()) {
        std::cout << "ERROR: Can only specify a timestamp or a sequence" << std::endl;
        exit(1);
    }

    // replication::Replication rep(server, timestamp, sequence);
    replication::Replication rep("https://planet.openstreetmap.org", timestamp, sequence);


    
    // std::vector<std::string> files;
    // files.push_back("004/139/998.state.txt");
    // std::shared_ptr<std::vector<std::string>> links = rep.downloadFiles(files, true);
    // std::cout << "Got "<< links->size() << " directories" << std::endl;
    // // links = rep.downloadFiles(*links, true);
    // // std::cout << "Got "<< links->size() << " directories" << std::endl;

    // changeset::StateFile foo("/tmp/foo1.txt", false);
    // changeset::StateFile bar("/tmp/foo2.txt", false);
    // return 0;
}

