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

#include <iostream>
#include <string>
#include <vector>

#include <djinterop/djinterop.hpp>

namespace el = djinterop::enginelibrary;

int main(int argc, char** argv)
{
    using namespace std::string_literals;

    auto dir = "Engine Library"s;
    bool created;
    auto db = el::create_or_load_database(dir, el::version_latest, created);
    std::cout << (created ? "Created " : "Loaded ") << "database in directory "
              << dir << std::endl;
    std::cout << "DB version is " << db.version_name() << std::endl;

    for (auto& cr : db.crates())
    {
        std::cout << "Removing prior crate " << cr.name() << std::endl;
        db.remove_crate(cr);
    }

    for (auto& tr : db.tracks())
    {
        std::cout << "Removing prior tr " << tr.filename() << std::endl;
        db.remove_track(tr);
    }

    djinterop::track_snapshot td;
    td.relative_path = "../01 - Some Artist - Some Song.mp3";
    td.track_number = 1;
    td.duration = std::chrono::milliseconds{366000};
    td.bpm = 120;
    td.year = 1970;
    td.title = "Some Song"s;
    td.artist = "Some Artist"s;
    td.publisher = djinterop::stdx::nullopt;  // indicates missing metadata
    td.key = djinterop::musical_key::a_minor;
    td.bitrate = 320;
    td.rating = 60; // note: rating is in the range 0-100
    td.average_loudness = 0.5;  // loudness range (0, 1]
    int64_t sample_count = 16140600;
    td.sampling = djinterop::sampling_info{
        44100,          // sample rate
        sample_count};  // sample count
    std::vector<djinterop::beatgrid_marker> beatgrid{
        {-4, -83316.78},                 // 1st marker
        {812, 17470734.439}};            // 2nd marker
    td.default_beatgrid = beatgrid;   // as analyzed
    td.adjusted_beatgrid = beatgrid;  // manually adjusted

    // The main cue concerns the cue button
    td.default_main_cue = 2732;   // as analyzed
    td.adjusted_main_cue = 2732;  // manually adjusted

    // There are always 8 hot cues, whereby each can optionally be set
    td.hot_cues[0] = djinterop::hot_cue{
        "Cue 1", 1377924.5,  // position in number of samples
        el::standard_pad_colors::pad_1};
    td.hot_cues[3] = djinterop::hot_cue{
        "Cue 4", 5508265.96, el::standard_pad_colors::pad_4};

    // The loop API works like the hot cue API
    td.loops[0] = djinterop::loop{
        "Loop 1", 1144.012, 345339.134, el::standard_pad_colors::pad_1};

    // Set high-resolution waveform
    int64_t spe = el::required_waveform_samples_per_entry(
        td.sampling->sample_rate);
    int64_t waveform_size = (sample_count + spe - 1) / spe;  // Ceiling division
    td.waveform.reserve(waveform_size);
    for (int64_t i = 0; i < waveform_size; ++i)
    {
        td.waveform.push_back(  // VALUE / OPACITY for each band (low/mid/high)
            {{0, 255},          // low
             {42, 255},         // mid
             {255, 255}});      // high
    }

    auto tr = db.create_track(td);
    std::cout << "Added tr " << tr.filename() << std::endl;

    auto cr = db.create_root_crate("My Example Crate");
    cr.add_track(tr);
    std::cout << "Added tr to crate " << cr.name() << std::endl;
}
