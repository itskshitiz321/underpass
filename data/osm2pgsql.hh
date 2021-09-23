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

#ifndef __OSM2PGSQL_HH__
#define __OSM2PGSQL_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include "pq.hh"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

/// \file osm2pgsql.hh
/// \brief Manages a osm2pgsql DB

/// \namespace osm2pgsql
namespace osm2pgsql {

///
/// \brief The Osm2Pgsql class handles the communication with an osm2pgsql DB.
///
/// Methods to query the osm2pgsql DB status and to update the DB with osm changes
/// are provided.
///
class Osm2Pgsql : public pq::Pq
{
  public:
    ///
    /// \brief OSM2PGSQL_DEFAULT_SCHEMA_NAME the default schema name for osm2pgsql tables.
    ///
    static const std::string OSM2PGSQL_DEFAULT_SCHEMA_NAME;

    ///
    /// \brief Osm2Pgsql constructs an Osm2Pgsql from arguments.
    /// \param dburl the DB url in the form USER:PASSSWORD@HOST/DATABASENAME
    /// \param schema name of the osm2pgsql schema, defaults to "osm2pgsql_pgsql".
    ///
    Osm2Pgsql(const std::string &dburl,
              const std::string &schema = OSM2PGSQL_DEFAULT_SCHEMA_NAME);

    ///
    /// \brief Osm2Pgsql constructs a default uninitialized Osm2Pgsql.
    ///        Call connect() and setSchema() to initialize.
    ///
    Osm2Pgsql() = default;

    ///
    /// \brief getLastUpdate
    /// \return the last timestamp in the DB
    ///
    ptime getLastUpdate();

    ///
    /// \brief updateDatabase updates the DB with osm changes.
    /// \param osm_changes input data (decompressed) from an OSC file.
    /// \return TRUE on success, errors are logged.
    ///
    bool updateDatabase(const std::string &osm_changes);

    bool connect(const std::string &dburl);

    ///
    /// \brief getSchema returns the schema name for osm2pgsql tables.
    /// \return the schema name.
    ///
    const std::string &getSchema() const;

    ///
    /// \brief setSchema sets the the schema name for osm2pgsql tables.
    /// \param newSchema the schema name.
    ///
    void setSchema(const std::string &newSchema);

  private:
    /// Get last timestamp in the DB
    bool getLastUpdateFromDb();

    ptime last_update = not_a_date_time;
    std::string dburl;

    /// Default schema name for osm2pgsql ("osm2pgsql_pgsql"), for simplicity, we are using
    /// the same schema for data and "middle" tables.
    std::string schema = OSM2PGSQL_DEFAULT_SCHEMA_NAME;
};

} // namespace osm2pgsql

#endif