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
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <utility>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <deque>
#include <list>
#include <zlib.h>
#ifdef LIBXML
#  include <libxml++/libxml++.h>
#endif 

#include <osmium/io/any_input.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/any_output.hpp>
#include <glibmm/convert.h>

#include <boost/foreach.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include<boost/tokenizer.hpp>

#include "hotosm.hh"
#include "osmstats/osmstats.hh"
#include "osmstats/changeset.hh"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

/// \namespace changeset
namespace changeset {

/// Check a character in a string if it's a control character
bool IsControl(int i) { return (iscntrl(i)); }

/// Read a changeset file from disk, which may be a huge file
/// Since it is a huge file, process in pieces and don't store
/// anything except in the database. A changeset file entry
/// looks like this:
///
/// <changeset id="12345" created_at="2014-10-10T01:57:09Z" closed_at="2014-10-10T01:57:23Z" open="false" user="foo" uid="54321" min_lat="-2.8042325" min_lon="29.5842812" max_lat="-2.7699398" max_lon="29.6012844" num_changes="569" comments_count="0">
///  <tag k="source" v="Bing"/>
///  <tag k="comment" v="#hotosm-task-001 #redcross #missingmaps"/>
///  <tag k="created_by" v="JOSM/1.5 (7182 en)"/>
/// </changeset>
bool
ChangeSetFile::importChanges(const std::string &file)
{
    std::ifstream change;
    int size = 0;
//    store = false;

#ifdef LIBXML
    // FIXME: this should really use CHUNKS, since the files can
    // many gigs.
    try {
        set_substitute_entities(true);
        parse_file(file);
    }
    catch(const xmlpp::exception& ex) {
        std::cerr << "libxml++ exception: " << ex.what() << std::endl;
        int return_code = EXIT_FAILURE;
    }
#endif

    osmstats::QueryOSMStats ostats;
    ostats.connect();
    for (auto it = std::begin(changes); it != std::end(changes); ++it) {
        ostats.applyChange(*it);
    }

    change.close();
    // FIXME: return real value
    return false;
}

bool
ChangeSetFile::readChanges(const std::vector<unsigned char> &buffer)
{
    
    //parse_memory((const Glib::ustring &)buffer);
}

// Read a changeset file from disk or memory into internal storage
bool
ChangeSetFile::readChanges(const std::string &file)
{
    std::ifstream change;
    int size = 0;
//    store = false;
    
    unsigned char *buffer;
    std::cout << "Reading changeset file " << file << std::endl;
    std::string suffix = boost::filesystem::extension(file);
    // It's a gzipped file, common for files downloaded from planet
    std::ifstream ifile(file, std::ios_base::in | std::ios_base::binary);
    if (suffix == ".gz") {  // it's a compressed file
//    if (file[0] == 0x1f) {
        change.open(file,  std::ifstream::in |  std::ifstream::binary);
        try {
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            inbuf.push(ifile);
            std::istream instream(&inbuf);
            // std::cout << instream.rdbuf();
            readXML(instream);
        } catch(std::exception& e) {
            std::cout << "ERROR opening " << file << std::endl;
            std::cout << e.what() << std::endl;
            // return false;
        }
    } else {                // it's a text file
        change.open(file, std::ifstream::in);
        readXML(change);
    }

#if 0
    // magic number: 0x8b1f or 0x1f8b for gzipped
    // <?xml for text
    std::string foo = "Hello World";
    boost::iostreams::array_source(foo.c_str(), foo.size());
    // boost::iostreams::filtering_streambuf<boost::iostreams::input> fooby(foo, 10);
#endif
    change.close();
}

void
ChangeSet::dump(void)
{
    std::cout << "-------------------------" << std::endl;
    std::cout << "Change ID: " << id << std::endl;
    std::cout << "Created At:  " << to_simple_string(created_at)  << std::endl;
    std::cout << "Closed At:   " << to_simple_string(closed_at) << std::endl;
    if (open) {
        std::cout << "Open change: true" << std::endl;
    } else {
        std::cout << "Open change: false" << std::endl;
    }
    std::cout << "User:        " << user<< std::endl;
    std::cout << "User ID:     " << uid << std::endl;
    std::cout << "Min Lat:     " << min_lat << std::endl;
    std::cout << "Min Lon:     " << min_lon << std::endl;
    std::cout << "Max Lat:     " << max_lat << std::endl;
    std::cout << "Max Lon:     " << max_lon << std::endl;
    std::cout << "Changes:     " << num_changes << std::endl;
    if (!source.empty()) {
        std::cout << "Source:      " << source << std::endl;
    }
    // std::cout << "Comments:    " << comments_count << std::endl;
    for (auto it = std::begin(hashtags); it != std::end(hashtags); ++it) {
        std::cout << "Hashtags:    " << *it <<  std::endl;
    }
    if (!comment.empty()) {
        std::cout << "Comments:    " << comment << std::endl;
    }
    std::cout << "Editor:      " << editor << std::endl;
}

#ifdef LIBXML
ChangeSet::ChangeSet(const std::deque<xmlpp::SaxParser::Attribute> attributes)
{
    for(const auto& attr_pair : attributes) {
        try {
            if (attr_pair.name == "id") {
                id = std::stol(attr_pair.value);   // change id
            } else if (attr_pair.name == "created_at") {
                created_at = from_iso_extended_string(attr_pair.value.substr(0,18));
            } else if (attr_pair.name == "closed_at") {
                closed_at = from_iso_extended_string(attr_pair.value.substr(0,18));
            } else if (attr_pair.name == "open") {
                if (attr_pair.value == "true") {
                    open = true;
                } else {
                    open = false;
                }
            } else if (attr_pair.name == "user") {
                user = attr_pair.value;
            } else if (attr_pair.name == "source") {
                source = attr_pair.value;
            } else if (attr_pair.name == "uid") {
                uid = std::stol(attr_pair.value);
            } else if (attr_pair.name == "lat") {
                min_lat = std::stod(attr_pair.value);
                max_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "min_lat") {
                min_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "max_lat") {
                max_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "lon") {
                min_lon = std::stod(attr_pair.value);
                max_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "min_lon") {
                min_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "max_lon") {
                max_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "num_changes") {
                num_changes = std::stoi(attr_pair.value);
            } else if (attr_pair.name == "changes_count") {
                num_changes = std::stoi(attr_pair.value);
            } else if (attr_pair.name == "comments_count") {
            }
        } catch(const Glib::ConvertError& ex) {
            std::cerr << "ChangeSet::ChangeSet(): Exception caught while converting values for std::cout: " << ex.what() << std::endl;
            }
    }
}
#endif  // EOF LIBXML

void
ChangeSetFile::dump(void)
{
    std::cout << "There are " << changes.size() << " changes" << std::endl;
    for (auto it = std::begin(changes); it != std::end(changes); ++it) {
        it->dump();
    }
}

// Read an istream of the data and parse the XML
//
bool
ChangeSetFile::readXML(std::istream &xml)
{
    // std::cout << xml.rdbuf();
#ifdef LIBXML
    // libxml calls on_element_start for each node, using a SAX parser,
    // and works well for large files.
    try {
        set_substitute_entities(true);
        parse_stream(xml);
    }
    catch(const xmlpp::exception& ex) {
        std::cerr << "libxml++ exception: " << ex.what() << std::endl;
        int return_code = EXIT_FAILURE;
    }
#else
    // Boost::parser_tree with RapidXML is faster, but builds a DOM tree
    // so loads the entire file into memory. Most replication files for
    // hourly or minutely changes are small, so this is better for that
    // case.
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(xml, pt);

    if (pt.empty()) {
        std::cerr << "ERROR: XML data is empty!" << std::endl;
        return false;
    }
    
    for (auto value: pt.get_child("osm")) {
        if (value.first == "changeset") {
            changeset::ChangeSet change;
            // Process the tags. These don't exist for every element
            for (auto tag: value.second) {
                if (tag.first == "tag") {
                    std::string key = tag.second.get("<xmlattr>.k", "");
                    std::string val = tag.second.get("<xmlattr>.v", "");
                    change.tags[key] = val;
                }
            }
            // Process the attributes, which do exist in every element
            change.id = value.second.get("<xmlattr>.id", 0);
            change.created_at = value.second.get("<xmlattr>.created_at",
                          boost::posix_time::second_clock::local_time());
            change.closed_at = value.second.get("<xmlattr>.closed_at",
                          boost::posix_time::second_clock::local_time());
            change.open = value.second.get("<xmlattr>.open", false);
            change.user = value.second.get("<xmlattr>.user", "");
            change.uid = value.second.get("<xmlattr>.uid", 0);
            change.min_lat = value.second.get("<xmlattr>.min_lat", 0.0);
            change.min_lon = value.second.get("<xmlattr>.min_lon", 0.0);
            change.max_lat = value.second.get("<xmlattr>.max_lat", 0.0);
            change.max_lon = value.second.get("<xmlattr>.max_lon", 0.0);
            change.num_changes = value.second.get("<xmlattr>.num_changes", 0);
            change.comments_count = value.second.get("<xmlattr>.comments_count", 0);
            changes.push_back(change);
        }
    }
#endif
}

#ifdef LIBXML
void
ChangeSetFile::on_end_element(const Glib::ustring& name)
{
    // std::cout << "Element \'" << name << "\' ending" << std::endl;
    if (name == "changeset") {
        // FIXME: it'd be better to not redo the database connection
        //for every changeset
        osmstats::QueryOSMStats ostats;
        ostats.connect();
        ostats.applyChange(changes.back());
    }
}

void
ChangeSetFile::on_start_element(const Glib::ustring& name,
                                const AttributeList& attributes)
{
    // std::cout << "Element \'" << name << "\' starting" << std::endl;

    if (name == "changeset") {
        changeset::ChangeSet change(attributes);
        changes.push_back(change);
        // changes.back().dump();
    } else if (name == "tag") {
        // We ignore most of the attributes, as they're not used for OSM stats.
        // Processing a tag requires multiple passes through the loop. The
        // tho tags to look for are 'k' (keyword) and 'v' (value). So when
        // we see a key we want, we have to wait for the next iteration of
        // the loop to get the value.
        bool hashit = false;
        bool comhit = false;
        bool cbyhit = false;
        bool min_lathit = false;
        bool min_lonhit = false;
        bool max_lathit = false;
        bool max_lonhit = false;
        double min_lat = 0.0;
        double min_lon = 0.0;
        double max_lat = 0.0;
        double max_lon = 0.0;

        for (const auto& attr_pair : attributes) {
            // std::wcout << "\tPAIR: " << attr_pair.name << " = " << attr_pair.value << std::endl;
            if (attr_pair.name == "k" && attr_pair.value == "max_lat") {
                max_lat = std::stod(attr_pair.value);
                max_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "min_lat") {
                min_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "lat") {
                min_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "max_lon") {
                max_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "min_lon") {
                min_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "lon") {
                min_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "hashtags") {
                hashit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "comment") {
                comhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "created_by") {
                cbyhit = true;
            }

            if (hashit && attr_pair.name == "v") {
                hashit = false;
                std::size_t pos = attr_pair.value.find('#', 0);
                if (pos != std::string::npos) {
                    // Don't allow really short hashtags, they're usually a typo
                    if (attr_pair.value.length() < 3) {
                        continue;
                    }
                    char *token = std::strtok((char *) attr_pair.value.c_str(), "#;");
                    while (token != NULL) {
                        token = std::strtok(NULL, "#;");
                        if (token) {
                            changes.back().addHashtags(token);
                        }
                    }
                } else {
                    changes.back().addHashtags(attr_pair.value);
                }
            }
            // Hashtags start with an # of course. The hashtag tag wasn't
            // added till later, so many older hashtags are in the comment
            // field instead.
            if (comhit && attr_pair.name == "v") {
                comhit = false;
                changes.back().addComment(attr_pair.value);
                if (attr_pair.value.find('#') != std::string::npos) {
                    std::vector<std::string> result;
                    boost::split(result, attr_pair.value, boost::is_any_of(" "));
                    for (auto it=std::begin(result); it != std::end(result); ++it){
                        int i = 0;
                        while (++i < it->size()) {
                            // if (std::isalpha(it->at(i))) {
                            //     break;
                            // }
                            if (it->at(i) == '#') {
                                changes.back().addHashtags(it->substr(i));
                                break;
                            }
                        }
                    }
                }
            }
            if (cbyhit && attr_pair.name == "v") {
                cbyhit = false;
                changes.back().addEditor(attr_pair.value);
            }
        }
    }
}
#endif  // EOF LIBXML

std::string
fixString(std::string text)
{
    std::string newstr;
    int i = 0;
    while (i < text.size()) {
        if (text[i] == '\'') {
            newstr += "&apos;";
        } else if (text[i] == '\"') {
            newstr += "&quot;";
        } else if (text[i] == ')') {
            newstr += "&#41;";
        } else if (text[i] == '(') {
            newstr += "&#40;";
        } else if (text[i] == '\\') {
            // drop this character
        } else {
            newstr += text[i];
        }
        i++;
    }
    return boost::locale::conv::to_utf<char>(newstr,"Latin1");
}

}       // EOF changeset

