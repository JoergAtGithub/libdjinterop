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

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <djinterop/exceptions.hpp>
#include <djinterop/track.hpp>

#include "../../util.hpp"
#include "convert_beatgrid.hpp"
#include "convert_hot_cues.hpp"
#include "convert_loops.hpp"
#include "convert_track.hpp"
#include "convert_waveform.hpp"
#include "database_impl.hpp"
#include "track_impl.hpp"

namespace djinterop::engine::v2
{
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

namespace
{
track_row snapshot_to_row(
    const track_snapshot& snapshot, const information_row& information)
{
    if (!snapshot.relative_path)
    {
        throw invalid_track_snapshot{
            "Snapshot does not contain a populated `relative_path` field, "
            "which is required to write a track"};
    }

    auto track_id = snapshot.id.value_or(TRACK_ROW_ID_NONE);

    auto filename = get_filename(*snapshot.relative_path);
    stdx::optional<std::string> remixer;
    auto rating = convert::write::rating(snapshot.rating);
    stdx::optional<std::string> album_art;
    bool is_played = false;
    bool is_analyzed = true;

    auto file_type = get_file_extension(filename);
    if (!file_type)
    {
        throw invalid_track_snapshot{
            "Snapshot refers to a file with no file extension, and so cannot "
            "auto-determine file type based on extension"};
    }

    int64_t album_art_id = ALBUM_ART_ID_NONE;
    stdx::optional<std::chrono::system_clock::time_point> date_created;
    stdx::optional<std::chrono::system_clock::time_point> date_added;
    bool is_available = true;
    bool is_metadata_of_packed_track_changed = false;
    bool is_performance_data_of_packed_track_changed = false;
    stdx::optional<int64_t> played_indicator;
    bool is_metadata_imported = false;
    int64_t pdb_import_key = 0;
    stdx::optional<std::string> streaming_source;
    stdx::optional<std::string> uri;
    bool is_beat_grid_locked = false;
    std::string origin_database_uuid = information.uuid;
    int64_t origin_track_id = 0;

    auto converted_average_loudness =
        convert::write::average_loudness(snapshot.average_loudness);
    auto converted_bpm = convert::write::bpm(snapshot.bpm);
    auto converted_duration = convert::write::duration(snapshot.duration);
    auto converted_key = convert::write::key(snapshot.key);
    auto converted_sampling = convert::write::sampling(snapshot.sampling);

    track_data_blob track_data{
        converted_sampling.track_data_sample_rate,
        converted_sampling.track_data_samples, converted_average_loudness,
        converted_key.track_data_key};

    auto overview_waveform_data =
        convert::write::waveform(snapshot.waveform, snapshot.sampling);

    auto converted_beatgrid = convert::write::beatgrid(
        snapshot.default_beatgrid, snapshot.adjusted_beatgrid);
    beat_data_blob beat_data{
        converted_sampling.beat_data_sample_rate,
        converted_sampling.beat_data_samples,
        converted_beatgrid.is_beatgrid_set,
        converted_beatgrid.default_beat_grid,
        converted_beatgrid.adjusted_beat_grid};

    quick_cues_blob quick_cues;
    quick_cues.default_main_cue =
        convert::write::main_cue(snapshot.default_main_cue);
    quick_cues.adjusted_main_cue =
        convert::write::main_cue(snapshot.adjusted_main_cue);
    quick_cues.is_main_cue_adjusted =
        quick_cues.default_main_cue != quick_cues.adjusted_main_cue;
    quick_cues.quick_cues = convert::write::hot_cues(snapshot.hot_cues);

    loops_blob loops = convert::write::loops(snapshot.loops);

    stdx::optional<int64_t> third_party_source_id;
    int64_t streaming_flags = 0;
    bool explicit_lyrics = false;

    return track_row{
        track_id,
        snapshot.track_number,
        converted_duration,
        converted_bpm.bpm,
        snapshot.year,
        *snapshot.relative_path,
        filename,
        snapshot.bitrate,
        converted_bpm.bpm_analyzed,
        album_art_id,
        snapshot.file_bytes,
        snapshot.title,
        snapshot.artist,
        snapshot.album,
        snapshot.genre,
        snapshot.comment,
        snapshot.publisher,
        snapshot.composer,
        remixer,
        converted_key.key,
        rating,
        album_art,
        snapshot.last_played_at,
        is_played,
        *file_type,
        is_analyzed,
        date_created,
        date_added,
        is_available,
        is_metadata_of_packed_track_changed,
        is_performance_data_of_packed_track_changed,
        played_indicator,
        is_metadata_imported,
        pdb_import_key,
        streaming_source,
        uri,
        is_beat_grid_locked,
        origin_database_uuid,
        origin_track_id,
        track_data,
        overview_waveform_data,
        beat_data,
        quick_cues,
        loops,
        third_party_source_id,
        streaming_flags,
        explicit_lyrics};
}
}  // namespace

track_impl::track_impl(std::shared_ptr<engine_library> library, int64_t id) :
    djinterop::track_impl{id}, library_{std::move(library)},
    track_{library_->track()}
{
}

track_snapshot track_impl::snapshot() const
{
    auto information = library_->information().get();
    auto row_maybe = track_.get(id());
    if (!row_maybe)
        throw djinterop::track_deleted{id()};

    track_snapshot snapshot{id()};
    auto& row = *row_maybe;

    snapshot.adjusted_beatgrid =
        convert::read::beatgrid_markers(row.beat_data.adjusted_beat_grid);
    snapshot.adjusted_main_cue =
        convert::read::main_cue(row.quick_cues.adjusted_main_cue);
    snapshot.album = row.album;
    snapshot.artist = row.artist;
    snapshot.average_loudness = convert::read::average_loudness(row.track_data);
    snapshot.bitrate = row.bitrate;
    snapshot.bpm = convert::read::bpm(row.bpm_analyzed, row.bpm);
    snapshot.comment = row.comment;
    snapshot.composer = row.composer;
    snapshot.default_beatgrid =
        convert::read::beatgrid_markers(row.beat_data.default_beat_grid);
    snapshot.default_main_cue =
        convert::read::main_cue(row.quick_cues.default_main_cue);
    snapshot.duration = convert::read::duration(row.length);
    snapshot.file_bytes = row.file_bytes;
    snapshot.genre = row.genre;
    snapshot.hot_cues = convert::read::hot_cues(row.quick_cues);
    snapshot.key = convert::read::key(row.key);
    snapshot.last_accessed_at = stdx::nullopt;
    snapshot.last_modified_at = stdx::nullopt;
    snapshot.last_played_at = row.time_last_played;
    snapshot.loops = convert::read::loops(row.loops);
    snapshot.publisher = row.label;
    snapshot.rating = convert::read::rating(row.rating);
    snapshot.relative_path = row.path;
    snapshot.sampling = convert::read::sampling(row.track_data);
    snapshot.title = row.title;
    snapshot.track_number = row.play_order;
    snapshot.waveform = convert::read::waveform(row.overview_waveform_data);
    snapshot.year = row.year;

    return snapshot;
}

void track_impl::update(const track_snapshot& snapshot)
{
    if (snapshot.id && *snapshot.id != id())
    {
        throw invalid_track_snapshot{
            "Snapshot pertains to a different track, and so it cannot be used "
            "to update this track"};
    }

    auto information = library_->information().get();
    auto row = snapshot_to_row(snapshot, information);
    row.id = id();

    track_.update(row);
}

std::vector<beatgrid_marker> track_impl::adjusted_beatgrid()
{
    auto beat_data = track_.get_beat_data(id());
    return convert::read::beatgrid_markers(beat_data.adjusted_beat_grid);
}

void track_impl::set_adjusted_beatgrid(std::vector<beatgrid_marker> beatgrid)
{
    auto beat_data = track_.get_beat_data(id());

    auto converted_beatgrid = convert::write::beatgrid(beatgrid);
    beat_data.adjusted_beat_grid = converted_beatgrid.adjusted_beat_grid;
    beat_data.is_beatgrid_set = converted_beatgrid.is_beatgrid_set;

    track_.set_beat_data(id(), beat_data);
}

double track_impl::adjusted_main_cue()
{
    auto quick_cues = track_.get_quick_cues(id());
    return quick_cues.adjusted_main_cue;
}

void track_impl::set_adjusted_main_cue(double sample_offset)
{
    auto quick_cues = track_.get_quick_cues(id());
    quick_cues.adjusted_main_cue = sample_offset;
    track_.set_quick_cues(id(), quick_cues);
}

stdx::optional<std::string> track_impl::album()
{
    return track_.get_album(id());
}

void track_impl::set_album(stdx::optional<std::string> album)
{
    track_.set_album(id(), album);
}

stdx::optional<int64_t> track_impl::album_art_id()
{
    return convert::read::album_art_id(track_.get_album_art_id(id()));
}

void track_impl::set_album_art_id(stdx::optional<int64_t> album_art_id)
{
    track_.set_album_art_id(id(), convert::write::album_art_id(album_art_id));
}

stdx::optional<std::string> track_impl::artist()
{
    return track_.get_artist(id());
}

void track_impl::set_artist(stdx::optional<std::string> artist)
{
    track_.set_artist(id(), artist);
}

stdx::optional<double> track_impl::average_loudness()
{
    auto track_data = track_.get_track_data(id());
    return convert::read::average_loudness(track_data);
}

void track_impl::set_average_loudness(stdx::optional<double> average_loudness)
{
    auto track_data = track_.get_track_data(id());
    track_data.average_loudness =
        convert::write::average_loudness(average_loudness);
    track_.set_track_data(id(), track_data);
}

stdx::optional<int64_t> track_impl::bitrate()
{
    return track_.get_bitrate(id());
}

void track_impl::set_bitrate(stdx::optional<int64_t> bitrate)
{
    track_.set_bitrate(id(), bitrate);
}

stdx::optional<double> track_impl::bpm()
{
    return convert::read::bpm(
        track_.get_bpm_analyzed(id()), track_.get_bpm(id()));
}

void track_impl::set_bpm(stdx::optional<double> bpm)
{
    auto fields = convert::write::bpm(bpm);
    track_.set_bpm_analyzed(id(), fields.bpm_analyzed);
    track_.set_bpm(id(), fields.bpm);
}

stdx::optional<std::string> track_impl::comment()
{
    return track_.get_comment(id());
}

void track_impl::set_comment(stdx::optional<std::string> comment)
{
    track_.set_comment(id(), comment);
}

stdx::optional<std::string> track_impl::composer()
{
    return track_.get_composer(id());
}

void track_impl::set_composer(stdx::optional<std::string> composer)
{
    track_.set_composer(id(), composer);
}

database track_impl::db()
{
    return database{std::make_shared<database_impl>(library_)};
}

std::vector<crate> track_impl::containing_crates()
{
    // TODO
    throw std::runtime_error{"containing_crates() - Not yet implemented"};
}

std::vector<beatgrid_marker> track_impl::default_beatgrid()
{
    auto beat_data = track_.get_beat_data(id());
    return convert::read::beatgrid_markers(beat_data.default_beat_grid);
}

void track_impl::set_default_beatgrid(std::vector<beatgrid_marker> beatgrid)
{
    auto beat_data = track_.get_beat_data(id());
    beat_data.default_beat_grid = convert::write::beatgrid_markers(beatgrid);
    track_.set_beat_data(id(), beat_data);
}

double track_impl::default_main_cue()
{
    auto quick_cues = track_.get_quick_cues(id());
    return quick_cues.default_main_cue;
}

void track_impl::set_default_main_cue(double sample_offset)
{
    auto quick_cues = track_.get_quick_cues(id());
    quick_cues.default_main_cue = sample_offset;
    track_.set_quick_cues(id(), quick_cues);
}

stdx::optional<milliseconds> track_impl::duration()
{
    auto length = track_.get_length(id());
    return convert::read::duration(length);
}

std::string track_impl::file_extension()
{
    auto rel_path = relative_path();
    return get_file_extension(rel_path).value_or(std::string{});
}

std::string track_impl::filename()
{
    auto rel_path = relative_path();
    return get_filename(rel_path);
}

stdx::optional<std::string> track_impl::genre()
{
    return track_.get_genre(id());
}

void track_impl::set_genre(stdx::optional<std::string> genre)
{
    track_.set_genre(id(), genre);
}

stdx::optional<hot_cue> track_impl::hot_cue_at(int32_t index)
{
    auto quick_cues = track_.get_quick_cues(id());
    if (index < 0 || index > quick_cues.quick_cues.size())
    {
        throw std::out_of_range{
            "Request for hot cue at given index exceeds maximum number of cues "
            "on track"};
    }

    return convert::read::hot_cue(quick_cues.quick_cues[index]);
}

void track_impl::set_hot_cue_at(int32_t index, stdx::optional<hot_cue> cue)
{
    auto quick_cues = track_.get_quick_cues(id());
    if (index < 0 || index > quick_cues.quick_cues.size())
    {
        throw std::out_of_range{
            "Request to set hot cue at given index exceeds maximum number of "
            "cues on track"};
    }

    quick_cues.quick_cues[index] = convert::write::hot_cue(cue);
    track_.set_quick_cues(id(), quick_cues);
}

std::array<stdx::optional<hot_cue>, 8> track_impl::hot_cues()
{
    auto quick_cues = track_.get_quick_cues(id());
    return convert::read::hot_cues(quick_cues);
}

void track_impl::set_hot_cues(std::array<stdx::optional<hot_cue>, 8> cues)
{
    auto quick_cues = track_.get_quick_cues(id());
    quick_cues.quick_cues = convert::write::hot_cues(cues);
    track_.set_quick_cues(id(), quick_cues);
}

stdx::optional<track_import_info> track_impl::import_info()
{
    auto this_database_uuid = library_->information().get().uuid;
    auto origin_database_uuid = track_.get_origin_database_uuid(id());
    auto origin_track_id = track_.get_origin_track_id(id());

    if (origin_database_uuid == this_database_uuid && origin_track_id == id())
        return stdx::nullopt;

    return track_import_info{origin_database_uuid, origin_track_id};
}

void track_impl::set_import_info(
    const stdx::optional<track_import_info>& import_info)
{
    if (import_info)
    {
        track_.set_origin_database_uuid(id(), import_info->external_db_uuid);
        track_.set_origin_track_id(id(), import_info->external_track_id);
    }
    else
    {
        auto this_database_uuid = library_->information().get().uuid;
        track_.set_origin_database_uuid(id(), this_database_uuid);
        track_.set_origin_track_id(id(), id());
    }
}

bool track_impl::is_valid()
{
    return track_.exists(id());
}

stdx::optional<musical_key> track_impl::key()
{
    return convert::read::key(track_.get_key(id()));
}

void track_impl::set_key(stdx::optional<musical_key> key)
{
    auto converted = convert::write::key(key);
    track_.set_key(id(), converted.key);

    auto track_data = track_.get_track_data(id());
    track_data.key = converted.track_data_key;
    track_.set_track_data(id(), track_data);
}

stdx::optional<system_clock::time_point> track_impl::last_accessed_at()
{
    throw std::runtime_error{
        "last_accessed_at() - Not implemented in Engine V2 track table"};
}

void track_impl::set_last_accessed_at(
    stdx::optional<system_clock::time_point> accessed_at)
{
    throw std::runtime_error{
        "set_last_accessed_at() - Not implemented in Engine V2 track table"};
}

stdx::optional<system_clock::time_point> track_impl::last_modified_at()
{
    throw std::runtime_error{
        "last_modified_at() - Not implemented in Engine V2 track table"};
}

void track_impl::set_last_modified_at(
    stdx::optional<system_clock::time_point> modified_at)
{
    throw std::runtime_error{
        "set_last_modified_at() - Not implemented in Engine V2 track table"};
}

stdx::optional<system_clock::time_point> track_impl::last_played_at()
{
    return track_.get_time_last_played(id());
}

void track_impl::set_last_played_at(
    stdx::optional<system_clock::time_point> played_at)
{
    track_.set_time_last_played(id(), played_at);
}

stdx::optional<loop> track_impl::loop_at(int32_t index)
{
    auto loops = track_.get_loops(id());
    if (index < 0 || index > loops.loops.size())
    {
        throw std::out_of_range{
            "Request for loop at given index exceeds maximum number of loops "
            "on track"};
    }

    return convert::read::loop(loops.loops[index]);
}

void track_impl::set_loop_at(int32_t index, stdx::optional<loop> l)
{
    auto loops = track_.get_loops(id());
    if (index < 0 || index > loops.loops.size())
    {
        throw std::out_of_range{
            "Request to set loop at given index exceeds maximum number of "
            "loops on track"};
    }

    loops.loops[index] = convert::write::loop(l);
    track_.set_loops(id(), loops);
}

std::array<stdx::optional<loop>, 8> track_impl::loops()
{
    return convert::read::loops(track_.get_loops(id()));
}

void track_impl::set_loops(std::array<stdx::optional<loop>, 8> loops)
{
    auto converted = convert::write::loops(loops);
    track_.set_loops(id(), converted);
}

std::vector<waveform_entry> track_impl::overview_waveform()
{
    auto overview_waveform_data = track_.get_overview_waveform_data(id());
    return convert::read::waveform(overview_waveform_data);
}

stdx::optional<std::string> track_impl::publisher()
{
    return track_.get_label(id());
}

void track_impl::set_publisher(stdx::optional<std::string> publisher)
{
    track_.set_label(id(), publisher);
}

stdx::optional<int32_t> track_impl::rating()
{
    auto rating = track_.get_rating(id());
    return convert::read::rating(rating);
}

void track_impl::set_rating(stdx::optional<int32_t> rating)
{
    track_.set_rating(id(), convert::write::rating(rating));
}

std::string track_impl::relative_path()
{
    return track_.get_path(id());
}

void track_impl::set_relative_path(std::string relative_path)
{
    track_.set_path(id(), relative_path);
}

stdx::optional<sampling_info> track_impl::sampling()
{
    auto track_data = track_.get_track_data(id());
    return convert::read::sampling(track_data);
}

void track_impl::set_sampling(stdx::optional<sampling_info> sampling)
{
    auto converted = convert::write::sampling(sampling);

    auto track_data = track_.get_track_data(id());
    track_data.samples = converted.track_data_samples;
    track_data.sample_rate = converted.track_data_sample_rate;

    auto beat_data = track_.get_beat_data(id());
    beat_data.samples = converted.beat_data_samples;
    beat_data.sample_rate = converted.beat_data_sample_rate;

    track_.set_track_data(id(), track_data);
    track_.set_beat_data(id(), beat_data);
}

stdx::optional<std::string> track_impl::title()
{
    return track_.get_title(id());
}

void track_impl::set_title(stdx::optional<std::string> title)
{
    track_.set_title(id(), title);
}

stdx::optional<int32_t> track_impl::track_number()
{
    return track_.get_play_order(id());
}

void track_impl::set_track_number(stdx::optional<int32_t> track_number)
{
    track_.set_play_order(id(), track_number);
}

std::vector<waveform_entry> track_impl::waveform()
{
    // Engine 2.x only has an overview waveform.
    return overview_waveform();
}

void track_impl::set_waveform(std::vector<waveform_entry> waveform)
{
    auto overview_waveform_data =
        convert::write::waveform(waveform, sampling());
    track_.set_overview_waveform_data(id(), overview_waveform_data);
}

stdx::optional<int32_t> track_impl::year()
{
    return track_.get_year(id());
}

void track_impl::set_year(stdx::optional<int32_t> year)
{
    track_.set_year(id(), year);
}

track create_track(
    const std::shared_ptr<engine_library>& library,
    const track_snapshot& snapshot)
{
    if (snapshot.id)
    {
        throw invalid_track_snapshot{
            "Snapshot already pertains to a persisted track, and so it cannot "
            "be created again"};
    }

    auto information = library->information().get();
    auto row = snapshot_to_row(snapshot, information);
    auto id = library->track().add(row);

    return track{std::make_shared<track_impl>(library, id)};
}

}  // namespace djinterop::engine::v2