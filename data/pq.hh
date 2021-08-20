//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Pq is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Pq is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Pq.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __PQ_HH__
#define __PQ_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>

/// \file pq.hh
/// \brief Pq for monitoring the OSM planet server for replication files.
///
/// These are the pq used to download and apply the replication files
/// to a database. The monitor the OSM planet server for updated replication
/// files.

/// \namespace pq
namespace pq {

class Pq
{
  public:
    Pq(void);
    ~Pq(void);
    /// Connect to the Pq database
    Pq(const std::string &dbname);
    bool connect(const std::string &args);
    /// Query the database
    pqxx::result query(const std::string &query);
    bool parseURL(const std::string &query);
    void dump(void);
    bool isOpen();

    //protected:
    std::unique_ptr<pqxx::connection> sdb;
    pqxx::result result;
    std::string host;
    std::string user;
    std::string passwd;
    std::string dbname;
};

} // namespace pq

#endif // EOF __PQ_HH__
