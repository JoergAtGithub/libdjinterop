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

#include <djinterop/engine/v2/track_data_blob.hpp>

#include <cassert>
#include <stdexcept>
#include <tuple>

#include "../encode_decode_utils.hpp"

namespace djinterop::engine::v2
{
std::vector<char> track_data_blob::to_blob() const
{
    std::vector<char> uncompressed(28);  // Track data has fixed size
    auto ptr = uncompressed.data();
    const auto end = ptr + uncompressed.size();

    ptr = encode_double_be(sample_rate, ptr);
    ptr = encode_int64_be(samples, ptr);
    ptr = encode_double_be(average_loudness, ptr);
    ptr = encode_int32_be(key, ptr);
    assert(ptr == end);

    return zlib_compress(uncompressed);
}

track_data_blob track_data_blob::from_blob(const std::vector<char>& blob)
{
    const auto uncompressed = zlib_uncompress(blob);
    auto ptr = uncompressed.data();
    const auto end = ptr + uncompressed.size();

    if (uncompressed.size() != 28)
    {
        throw std::invalid_argument{
            "Track data blob doesn't have expected decompressed length of 28 "
            "bytes"};
    }

    track_data_blob result{};
    std::tie(result.sample_rate, ptr) = decode_double_be(ptr);
    std::tie(result.samples, ptr) = decode_int64_be(ptr);
    std::tie(result.average_loudness, ptr) = decode_double_be(ptr);
    std::tie(result.key, ptr) = decode_int32_be(ptr);
    assert(ptr == end);

    return result;
}

}  // namespace djinterop::engine::v2