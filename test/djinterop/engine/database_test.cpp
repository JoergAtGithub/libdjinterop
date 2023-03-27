/*
    This file is part of libdjinterop.

    libdjinterop is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libdjinterop is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libdjinterop.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <djinterop/database.hpp>

#define BOOST_TEST_MODULE database_test
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp>

#include <ostream>
#include <string>

#include <djinterop/crate.hpp>
#include <djinterop/engine/engine.hpp>
#include <djinterop/optional.hpp>
#include <djinterop/semantic_version.hpp>
#include <djinterop/track.hpp>
#include <djinterop/track_snapshot.hpp>

#include "../temporary_directory.hpp"

#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x

namespace utf = boost::unit_test;
namespace e = djinterop::engine;

namespace
{
struct reference_script
{
    std::string path;
    e::engine_version expected_version;
};

std::ostream& operator<<(std::ostream& os, const reference_script& rs)
{
    os << rs.path;
    return os;
}

const std::vector<reference_script> reference_db_scripts{
    reference_script{"/ref/engine/sc5000/firmware-1.0.3", e::os_1_0_3},
    reference_script{"/ref/engine/ep/ep-1.1.1", e::desktop_1_1_1},
    reference_script{"/ref/engine/sc5000/firmware-1.2.0", e::os_1_2_0},
    reference_script{"/ref/engine/sc5000/firmware-1.2.2", e::os_1_2_2},
    reference_script{"/ref/engine/ep/ep-1.2.2", e::desktop_1_2_2},
    reference_script{"/ref/engine/sc5000/firmware-1.3.1", e::os_1_3_1},
    reference_script{"/ref/engine/sc5000/firmware-1.4.0", e::os_1_4_0},
    reference_script{"/ref/engine/sc5000/firmware-1.5.1", e::os_1_5_1},
    reference_script{"/ref/engine/ep/ep-1.5.1", e::desktop_1_5_1},
    reference_script{"/ref/engine/sc5000/firmware-1.5.2", e::os_1_5_1},
    reference_script{"/ref/engine/ep/ep-1.6.0", e::desktop_1_5_1},
    reference_script{"/ref/engine/sc5000/firmware-1.6.0", e::os_1_6_0},
    reference_script{"/ref/engine/ep/ep-1.6.1", e::desktop_1_5_1},
    reference_script{"/ref/engine/sc5000/firmware-1.6.1", e::os_1_6_0},
    reference_script{"/ref/engine/sc5000/firmware-1.6.2", e::os_1_6_0},
    reference_script{"/ref/engine/desktop/desktop-2.0.0", e::desktop_2_0_0},
    reference_script{"/ref/engine/sc5000/firmware-2.0.0", e::os_2_0_0},
};

struct example_file
{
    std::string relative_path;
    std::string filename;
    std::string file_extension;
};

std::ostream& operator<<(std::ostream& os, const example_file& ef)
{
    os << ef.relative_path;
    return os;
}

const std::vector<example_file> valid_files{
    example_file{
        "../path/to/file_in_other_dir.mp3", "file_in_other_dir.mp3", "mp3"},
    example_file{"local_file.flac", "local_file.flac", "flac"},
};

const std::string sample_path{STRINGIFY(TESTDATA_DIR) "/el2"};
}  // anonymous namespace


BOOST_TEST_DECORATOR(* utf::description(
    "database::create_root_crate() for all supported schema versions"))
BOOST_DATA_TEST_CASE(
    create_root_crate__supported_version__creates, e::all_v1_versions, version)
{
    // Note separate scope to ensure no locks are held on the temporary dir.
    temporary_directory tmp_loc;

    {
        // Arrange
        auto db = e::create_database(tmp_loc.temp_dir, version);
        std::string crate_name = "Example Root Crate";

        // Act
        auto crate = db.create_root_crate(crate_name);

        // Assert
        BOOST_CHECK_NE(crate.id(), 0);
        BOOST_CHECK_EQUAL(crate.db().uuid(), db.uuid());
        BOOST_CHECK_EQUAL(crate.name(), crate_name);
        BOOST_CHECK(crate.parent() == djinterop::stdx::nullopt);
    }
}

BOOST_TEST_DECORATOR(* utf::description(
    "database::create_track() for all supported schema versions"))
BOOST_DATA_TEST_CASE(
    create_track__supported_version__creates, e::all_v1_versions * valid_files,
    version, file)
{
    // Note separate scope to ensure no locks are held on the temporary dir.
    temporary_directory tmp_loc;

    {
        // Arrange
        djinterop::track_snapshot track_data;
        track_data.relative_path = file.relative_path;
        auto db = e::create_database(tmp_loc.temp_dir, version);

        // Act
        auto track = db.create_track(track_data);

        // Assert
        BOOST_CHECK_NE(track.id(), 0);
        BOOST_CHECK_EQUAL(track.db().uuid(), db.uuid());
        BOOST_CHECK_EQUAL(track.filename(), file.filename);
        BOOST_CHECK_EQUAL(track.file_extension(), file.file_extension);
        BOOST_CHECK_EQUAL(track.relative_path(), file.relative_path);
    }
}

BOOST_TEST_DECORATOR(* utf::description(
    "database::verify() with 'reference scripts' for all supported versions"))
BOOST_DATA_TEST_CASE(
    verify__reference_scripts__no_throw, reference_db_scripts, reference_script)
{
    // Note separate scope to ensure no locks are held on the temporary dir.
    temporary_directory tmp_loc;

    {
        // Arrange
        auto script_path = std::string{STRINGIFY(TESTDATA_DIR) "/"} +
                           reference_script.path;

        BOOST_TEST_CHECKPOINT("(" << reference_script << ") Creating DB...");
        auto db =
            e::create_database_from_scripts(
            tmp_loc.temp_dir, script_path);

        // Act
        BOOST_TEST_CHECKPOINT("(" << reference_script << ") Verifying DB...");
        db.verify();

        // Assert
        BOOST_CHECK_EQUAL(db.directory(), tmp_loc.temp_dir);
    }
}

BOOST_AUTO_TEST_CASE(tracks__sample_db__expected_ids)
{
    // Arrange
    // TODO (mr-smidge) Replace with DB constructed in the test.
    auto db = e::load_database(sample_path);

    // Act
    auto results = db.tracks();

    // Assert
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0].id(), 1);
}

BOOST_AUTO_TEST_CASE(tracks_by_relative_path__valid_path__expected_id)
{
    // Arrange
    // TODO (mr-smidge) Replace with DB constructed in the test.
    auto db = e::load_database(sample_path);

    // Act
    auto results = db.tracks_by_relative_path(
        "../01 - Dennis Cruz - Mad (Original Mix).mp3");

    // Assert
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0].id(), 1);
}

BOOST_AUTO_TEST_CASE(tracks_by_relative_path__invalid_path__no_ids)
{
    // Arrange
    // TODO (mr-smidge) Replace with DB constructed in the test.
    auto db = e::load_database(sample_path);

    // Act
    auto results = db.tracks_by_relative_path("Does Not Exist.mp3");

    // Assert
    BOOST_CHECK_EQUAL(results.size(), 0);
}

BOOST_AUTO_TEST_CASE(track_by_id__invalid_id__nullopt)
{
    // Arrange
    // TODO (mr-smidge) Replace with DB constructed in the test.
    auto db = e::load_database(sample_path);

    // Act / Assert
    BOOST_CHECK(!db.track_by_id(123));
}
