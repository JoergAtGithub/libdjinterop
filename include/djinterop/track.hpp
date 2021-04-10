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

#pragma once
#ifndef DJINTEROP_TRACK_HPP
#define DJINTEROP_TRACK_HPP

#if __cplusplus < 201703L
#error This library needs at least a C++17 compliant compiler
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <djinterop/config.hpp>
#include <djinterop/musical_key.hpp>
#include <djinterop/optional.hpp>
#include <djinterop/performance_data.hpp>
#include <djinterop/semantic_version.hpp>

namespace djinterop
{
class database;
class crate;
class track_impl;
struct track_snapshot;

/// The `track_import_info` struct holds information about a track in a
/// different, external Engine Library database.  This can be associated with a
/// track if it was imported into the current database from another one.
struct track_import_info
{
    // TODO (mr-smidge): Refactor to remove Engine-specific details.

    /// The UUID of the external Engine Library database.
    std::string external_db_uuid;

    /// The id of the track in the external Engine Library database.
    int64_t external_track_id;
};

/// A `track` object is a handle to a track stored in a database. As long as it
/// lives, the corresponding database connection is kept open.
///
/// `track` objects can be copied and assigned cheaply, resulting in multiple
/// handles to the same actual track.
///
/// The read/write operations provided by this class directly access the
/// database.
///
/// A `track` object becomes invalid if the track gets deleted by
/// `database::remove_track()`. After that, you must not call any methods on the
/// `track` object, except for destructing it, or assigning to it.
class DJINTEROP_PUBLIC track
{
public:
    /// Copy constructor
    track(const track& other) noexcept;

    /// Destructor
    ~track();

    /// Copy assignment operator
    track& operator=(const track& other) noexcept;

    /// Obtain a snapshot of the track's current state.
    track_snapshot snapshot() const;

    /// Update the track with the contents of the provided snapshot.
    void update(const track_snapshot& snapshot);

    std::vector<beatgrid_marker> adjusted_beatgrid() const;

    void set_adjusted_beatgrid(std::vector<beatgrid_marker> beatgrid) const;

    double adjusted_main_cue() const;

    void set_adjusted_main_cue(double sample_offset) const;

    /// Returns the album name (metadata) of the track
    stdx::optional<std::string> album() const;

    /// Sets the album name (metadata) of the track
    void set_album(stdx::optional<std::string> album) const;
    void set_album(std::string album) const;

    /// Returns the ID of the `album_art` associated to the track
    ///
    /// If the track doesn't have an associated `album_art`, then `nullopt`
    /// is returned.
    /// TODO (haslersn): Return an `album_art` object instead.
    stdx::optional<int64_t> album_art_id() const;

    /// Sets the ID of the `album_art` associated to the track
    /// TODO (haslersn): Pass an `album_art` object instead.
    void set_album_art_id(stdx::optional<int64_t> album_art_id) const;
    void set_album_art_id(int64_t album_art_id) const;

    /// Returns the artist (metadata) of the track
    stdx::optional<std::string> artist() const;

    /// Sets the artist (metadata) of the track
    void set_artist(stdx::optional<std::string> artist) const;
    void set_artist(std::string artist) const;

    stdx::optional<double> average_loudness() const;

    void set_average_loudness(stdx::optional<double> average_loudness) const;
    void set_average_loudness(double average_loudness) const;

    /// Returns the bitrate (metadata) of the track
    stdx::optional<int64_t> bitrate() const;

    /// Sets the bitrate (metadata) of the track
    void set_bitrate(stdx::optional<int64_t> bitrate) const;
    void set_bitrate(int64_t bitrate) const;

    /// Returns the BPM (metadata) of the track, rounded to the nearest integer
    stdx::optional<double> bpm() const;

    /// Sets the BPM (metadata) of the track, rounded to the nearest integer
    void set_bpm(stdx::optional<double> bpm) const;
    void set_bpm(double bpm) const;

    /// Returns the comment associated to the track (metadata)
    stdx::optional<std::string> comment() const;

    /// Sets the comment associated to the track (metadata)
    void set_comment(stdx::optional<std::string> comment) const;
    void set_comment(std::string comment) const;

    /// Returns the composer (metadata) of the track
    stdx::optional<std::string> composer() const;

    /// Sets the composer (metadata) of the track
    void set_composer(stdx::optional<std::string> composer) const;
    void set_composer(std::string composer) const;

    /// Returns the crates containing the track
    std::vector<crate> containing_crates() const;

    /// Returns the database containing the track
    database db() const;

    std::vector<beatgrid_marker> default_beatgrid() const;

    void set_default_beatgrid(std::vector<beatgrid_marker> beatgrid) const;

    double default_main_cue() const;

    void set_default_main_cue(double sample_offset) const;

    /// Returns the duration (metadata) of the track
    stdx::optional<std::chrono::milliseconds> duration() const;

    // TODO (mr-smidge): Add `file_bytes()` and `set_file_bytes()` methods.

    /// Returns the file extension part of `track::relative_path()`
    ///
    /// An empty string is returned if the file doesn't have an extension.
    std::string file_extension() const;

    /// Returns the filename part of `track::relative_path()` (including the
    /// file extension)
    std::string filename() const;

    /// Returns the genre (metadata) of the track
    stdx::optional<std::string> genre() const;

    /// Sets the genre (metadata) of the track
    void set_genre(stdx::optional<std::string> genre) const;
    void set_genre(std::string genre) const;

    stdx::optional<hot_cue> hot_cue_at(int32_t index) const;

    void set_hot_cue_at(int32_t index, stdx::optional<hot_cue> cue) const;
    void set_hot_cue_at(int32_t index, hot_cue cue) const;

    std::array<stdx::optional<hot_cue>, 8> hot_cues() const;

    void set_hot_cues(std::array<stdx::optional<hot_cue>, 8> cues) const;

    /// Returns the ID of this track
    ///
    /// The ID is used internally in the database and is unique for tracks
    /// contained in the same database.
    int64_t id() const;

    /// TODO (haslersn): Document this method.
    stdx::optional<track_import_info> import_info() const;

    /// TODO (haslersn): Document these methods.
    void set_import_info(
        const stdx::optional<track_import_info>& import_info) const;
    void set_import_info(const track_import_info& import_info) const;

    /// Returns `true` iff `*this` is valid as described in the class comment
    bool is_valid() const;

    /// Returns the key (metadata) of the track
    stdx::optional<musical_key> key() const;

    /// Sets the key (metadata) of the track
    void set_key(stdx::optional<musical_key> key) const;
    void set_key(musical_key key) const;

    /// Get the time at which this track was last accessed
    ///
    /// Note that on VFAT filesystems, the access time is ceiled to just a date,
    /// and loses any time precision.
    stdx::optional<std::chrono::system_clock::time_point> last_accessed_at()
        const;

    /// TODO (haslersn): Document these methods.
    void set_last_accessed_at(
        stdx::optional<std::chrono::system_clock::time_point> last_accessed_at)
        const;
    void set_last_accessed_at(
        std::chrono::system_clock::time_point last_accessed_at) const;

    /// Get the time of last attribute modification of this track's file
    ///
    /// Note that this is the attribute modification time, not the data
    /// modification time, i.e. ctime not mtime.
    stdx::optional<std::chrono::system_clock::time_point> last_modified_at()
        const;

    /// TODO (haslersn): Document these methods.
    void set_last_modified_at(
        stdx::optional<std::chrono::system_clock::time_point> last_modified_at)
        const;
    void set_last_modified_at(
        std::chrono::system_clock::time_point last_modified_at) const;

    /// Returns the time at which the track was last played
    stdx::optional<std::chrono::system_clock::time_point> last_played_at()
        const;

    /// Sets the time at which the track was last played
    void set_last_played_at(
        stdx::optional<std::chrono::system_clock::time_point> time) const;
    void set_last_played_at(std::chrono::system_clock::time_point time) const;

    stdx::optional<loop> loop_at(int32_t index) const;

    void set_loop_at(int32_t index, stdx::optional<loop> l) const;
    void set_loop_at(int32_t index, loop l) const;

    std::array<stdx::optional<loop>, 8> loops() const;

    void set_loops(std::array<stdx::optional<loop>, 8> loops) const;

    std::vector<waveform_entry> overview_waveform() const;

    /// Returns the publisher (metadata) of the track
    stdx::optional<std::string> publisher() const;

    /// Sets the publisher (metadata) of the track
    void set_publisher(stdx::optional<std::string> publisher) const;
    void set_publisher(std::string publisher) const;

    /// Gets the track rating, from 0-100.
    stdx::optional<int32_t> rating() const;

    /// Sets the track rating, from 0-100.  Any rating provided outside this
    /// range will be clamped.
    void set_rating(stdx::optional<int32_t> rating);
    void set_rating(int32_t rating);

    /// Get the path to this track's file on disk, relative to the music
    /// database.
    std::string relative_path() const;

    /// TODO (haslersn): Document this method.
    void set_relative_path(std::string relative_path) const;

    stdx::optional<sampling_info> sampling() const;

    void set_sampling(stdx::optional<sampling_info> sample_rate) const;
    void set_sampling(sampling_info sample_rate) const;

    /// Returns the title (metadata) of the track
    stdx::optional<std::string> title() const;

    /// Sets the title (metadata) of the track
    void set_title(stdx::optional<std::string> title) const;
    void set_title(std::string title) const;

    /// Returns the track number (metadata) of the track
    stdx::optional<int32_t> track_number() const;

    /// Sets the track number (metadata) of the track
    void set_track_number(stdx::optional<int32_t> track_number) const;
    void set_track_number(int32_t track_number) const;

    // TODO (mr-smidge): Add `uri()` and `set_uri()` methods.

    std::vector<waveform_entry> waveform() const;

    void set_waveform(std::vector<waveform_entry> waveform) const;

    /// Returns the recording year (metadata) of the track
    stdx::optional<int32_t> year() const;

    /// Sets the recording year (metadata) of the track
    void set_year(stdx::optional<int32_t> year) const;
    void set_year(int32_t year) const;

    // TODO (haslersn): non public?
    track(std::shared_ptr<track_impl> pimpl);

private:
    std::shared_ptr<track_impl> pimpl_;
};

}  // namespace djinterop

#endif  // DJINTEROP_TRACK_HPP
