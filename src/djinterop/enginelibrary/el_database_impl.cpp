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
#include <djinterop/enginelibrary/el_database_impl.hpp>

#include <djinterop/enginelibrary/el_crate_impl.hpp>
#include <djinterop/enginelibrary/el_storage.hpp>
#include <djinterop/enginelibrary/el_track_impl.hpp>
#include <djinterop/enginelibrary/el_transaction_guard_impl.hpp>
#include <djinterop/enginelibrary/schema/schema.hpp>
#include <djinterop/transaction_guard.hpp>
#include <djinterop/util.hpp>

namespace djinterop
{
namespace enginelibrary
{
using djinterop::crate;
using djinterop::track;

namespace
{
void ensure_valid_crate_name(const std::string& name)
{
    if (name == "")
    {
        throw djinterop::crate_invalid_name{
            "Crate names must be non-empty", name};
    }
    else if (name.find_first_of(';') != std::string::npos)
    {
        throw djinterop::crate_invalid_name{
            "Crate names must not contain semicolons", name};
    }
}

}  // namespace

el_database_impl::el_database_impl(std::shared_ptr<el_storage> storage) :
    storage_{std::move(storage)}
{
}

transaction_guard el_database_impl::begin_transaction()
{
    return transaction_guard{
        std::make_unique<el_transaction_guard_impl>(storage_)};
}

stdx::optional<crate> el_database_impl::crate_by_id(int64_t id)
{
    stdx::optional<crate> cr;
    storage_->db << "SELECT COUNT(*) FROM Crate WHERE id = ?" << id >>
        [&](int64_t count) {
            if (count == 1)
            {
                cr = crate{std::make_shared<el_crate_impl>(storage_, id)};
            }
            else if (count > 1)
            {
                throw crate_database_inconsistency{
                    "More than one crate with the same ID", id};
            }
        };
    return cr;
}

std::vector<crate> el_database_impl::crates()
{
    std::vector<crate> results;
    storage_->db << "SELECT id FROM Crate ORDER BY id" >> [&](int64_t id) {
        results.push_back(crate{std::make_shared<el_crate_impl>(storage_, id)});
    };
    return results;
}

std::vector<crate> el_database_impl::crates_by_name(const std::string& name)
{
    std::vector<crate> results;
    storage_->db << "SELECT id FROM Crate WHERE title = ? ORDER BY id"
                 << name.data() >>
        [&](int64_t id) {
            results.push_back(
                crate{std::make_shared<el_crate_impl>(storage_, id)});
        };
    return results;
}

crate el_database_impl::create_root_crate(std::string name)
{
    ensure_valid_crate_name(name);
    el_transaction_guard_impl trans{storage_};

    int64_t id;
    if (storage_->version >= version_1_9_1)
    {
        // Newer schemas consider crates to be a kind of 'list', and so the
        // `Crate` table has been replaced with a VIEW onto `List`.  The main
        // difference is that `List` does not have an integer primary key, so
        // the new id will need to be determined in advance.
        storage_->db << "SELECT IFNULL(MAX(id), 0) + 1 FROM Crate" >> id;
        storage_->db << "INSERT INTO Crate (id, title, path) VALUES (?, ?, ?)"
                     << id << name.data() << std::string{name} + ';';
    }
    else
    {
        // Older schema versions have a dedicated table for crates that has
        // an integer primary key, which will be filled automatically.
        storage_->db << "INSERT INTO Crate (title, path) VALUES (?, ?)"
                     << name.data() << std::string{name} + ';';
        id = storage_->db.last_insert_rowid();
    }

    storage_->db << "INSERT INTO CrateParentList (crateOriginId, "
                    "crateParentId) VALUES (?, ?)"
                 << id << id;

    crate cr{std::make_shared<el_crate_impl>(storage_, id)};

    trans.commit();

    return cr;
}

track el_database_impl::create_track(const track_snapshot& snapshot)
{
    return djinterop::enginelibrary::create_track(storage_, snapshot);
}

std::string el_database_impl::directory()
{
    return storage_->directory;
}

bool el_database_impl::is_supported()
{
    return schema::is_supported(version());
}

void el_database_impl::verify()
{
    auto schema_creator_validator =
        schema::make_schema_creator_validator(version());
    schema_creator_validator->verify(storage_->db);
}

void el_database_impl::remove_crate(crate cr)
{
    storage_->db << "DELETE FROM Crate WHERE id = ?" << cr.id();
}

void el_database_impl::remove_track(track tr)
{
    storage_->db << "DELETE FROM Track WHERE id = ?" << tr.id();
    // All other references to the track should automatically be cleared by
    // "ON DELETE CASCADE"
}

std::vector<crate> el_database_impl::root_crates()
{
    std::vector<crate> results;
    storage_->db
            << "SELECT crateOriginId FROM CrateParentList WHERE crateParentId "
               "= crateOriginId ORDER BY crateOriginId" >>
        [&](int64_t id) {
            results.push_back(
                crate{std::make_shared<el_crate_impl>(storage_, id)});
        };
    return results;
}

stdx::optional<crate> el_database_impl::root_crate_by_name(
    const std::string& name)
{
    stdx::optional<crate> cr;
    storage_->db << "SELECT cr.id FROM Crate cr "
                    "JOIN CrateParentList cpl ON (cpl.crateOriginId = cr.id) "
                    "WHERE cr.title = ? "
                    "AND cpl.crateOriginId = cpl.crateParentId "
                    "ORDER BY cr.id"
                 << name.data() >>
        [&](int64_t id) {
            cr = crate{std::make_shared<el_crate_impl>(storage_, id)};
        };
    return cr;
}

stdx::optional<track> el_database_impl::track_by_id(int64_t id)
{
    stdx::optional<track> tr;
    storage_->db << "SELECT COUNT(*) FROM Track WHERE id = ?" << id >>
        [&](int64_t count) {
            if (count == 1)
            {
                tr = track{std::make_shared<el_track_impl>(storage_, id)};
            }
            else if (count > 1)
            {
                throw track_database_inconsistency{
                    "More than one track with the same ID", id};
            }
        };
    return tr;
}

std::vector<track> el_database_impl::tracks()
{
    std::vector<track> results;
    storage_->db << "SELECT id FROM Track ORDER BY id" >> [&](int64_t id) {
        results.push_back(track{std::make_shared<el_track_impl>(storage_, id)});
    };
    return results;
}

std::vector<track> el_database_impl::tracks_by_relative_path(
    const std::string& relative_path)
{
    std::vector<track> results;
    storage_->db << "SELECT id FROM Track WHERE path = ? ORDER BY id"
                 << relative_path.data() >>
        [&](int64_t id) {
            results.push_back(
                track{std::make_shared<el_track_impl>(storage_, id)});
        };
    return results;
}

std::string el_database_impl::uuid()
{
    std::string uuid;
    storage_->db << "SELECT uuid FROM Information" >> uuid;
    return uuid;
}

semantic_version el_database_impl::version()
{
    return storage_->version;
}

std::string el_database_impl::version_name()
{
    return storage_->schema_creator_validator->name();
}

}  // namespace enginelibrary
}  // namespace djinterop
