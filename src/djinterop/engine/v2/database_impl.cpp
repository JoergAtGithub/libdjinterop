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
#include "database_impl.hpp"

#include <stdexcept>

#include <djinterop/djinterop.hpp>
#include <djinterop/transaction_guard.hpp>

#include "../../util.hpp"
#include "../schema/schema.hpp"
#include "engine_library_context.hpp"
#include "track_impl.hpp"

namespace djinterop::engine::v2
{
database_impl::database_impl(std::shared_ptr<engine_library> library) :
    library_{std::move(library)}
{
}

transaction_guard database_impl::begin_transaction()
{
    // TODO
    throw std::runtime_error{"database_impl::begin_transaction() - Not implemented yet"};
}

stdx::optional<crate> database_impl::crate_by_id(int64_t id)
{
    // TODO
    throw std::runtime_error{"database_impl::crate_by_id() - Not implemented yet"};
}

std::vector<crate> database_impl::crates()
{
    // TODO
    throw std::runtime_error{"database_impl::crates() - Not implemented yet"};
}

std::vector<crate> database_impl::crates_by_name(const std::string& name)
{
    // TODO
    throw std::runtime_error{"database_impl::crates_by_name() - Not implemented yet"};
}

crate database_impl::create_root_crate(std::string name)
{
    // TODO
    throw std::runtime_error{"database_impl::create_root_crate() - Not implemented yet"};
}

track database_impl::create_track(const track_snapshot& snapshot)
{
    return djinterop::engine::v2::create_track(library_, snapshot);
}

std::string database_impl::directory()
{
    return library_->directory();
}

void database_impl::verify()
{
    library_->verify();
}

void database_impl::remove_crate(crate cr)
{
    // TODO
    throw std::runtime_error{"database_impl::remove_crate() - Not implemented yet"};
}

void database_impl::remove_track(track tr)
{
    // TODO
    throw std::runtime_error{"database_impl::remove_track() - Not implemented yet"};
}

std::vector<crate> database_impl::root_crates()
{
    // TODO
    throw std::runtime_error{"database_impl::root_crates() - Not implemented yet"};
}

stdx::optional<crate> database_impl::root_crate_by_name(const std::string& name)
{
    // TODO
    throw std::runtime_error{"database_impl::root_crate_by_name() - Not implemented yet"};
}

stdx::optional<track> database_impl::track_by_id(int64_t id)
{
    auto track_table = library_->track();
    if (track_table.exists(id))
    {
        return stdx::make_optional<track>(
                std::make_shared<track_impl>(library_, id));
    }

    return stdx::nullopt;
}

std::vector<track> database_impl::tracks()
{
    std::vector<track> results;
    auto track_table = library_->track();
    for (auto&& id : track_table.all_ids())
    {
        results.emplace_back(std::make_shared<track_impl>(library_, id));
    }

    return results;
}

std::vector<track> database_impl::tracks_by_relative_path(
    const std::string& relative_path)
{
    // TODO
    throw std::runtime_error{"database_impl::tracks_by_relative_path() - Not implemented yet"};
}

std::string database_impl::uuid()
{
    return library_->information().get().uuid;
}

std::string database_impl::version_name()
{
    return library_->version().name;
}

}  // namespace djinterop::engine::v2
