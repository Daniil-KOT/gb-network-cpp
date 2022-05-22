#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <fstream>
#include <csignal>

#include <libtorrent/session.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>

std::string make_magnet_uri(const std::string filename)
{
    lt::load_torrent_limits cfg;
    lt::torrent_info info(filename, cfg);

    info.set_web_seeds({});

    return lt::make_magnet_uri(info);
}

using clk = std::chrono::steady_clock;

// Return the name of a torrent status enum.
char const* state(lt::torrent_status::state_t s)
{
    switch (s)
    {
        case lt::torrent_status::checking_files: return "Checking";
        case lt::torrent_status::downloading_metadata: return "Downloading metadata";
        case lt::torrent_status::downloading: return "Downloading";
        case lt::torrent_status::finished: return "finished";
        case lt::torrent_status::seeding: return "Seeding";
        case lt::torrent_status::checking_resume_data: return "Checking resume";
        default: return "<>";
    }
}

std::vector<char> load_file(char const* filename)
{
    std::ifstream ifs(filename, std::ios_base::binary);
    ifs.unsetf(std::ios_base::skipws);
    return {std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
}


// set when we're exiting
std::atomic<bool> shut_down{false};

void sighandler(int) 
{ 
    shut_down = true; 
}

void usage(std::string name)
{
    std::cerr << "Usage: " 
              << name 
              << " {-file <filename>} or {-url <magnet-url>}"
              << std::endl;
}

int main(int argc, const char* argv[])
{
    if (argc != 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string magnet_uri;

    if (strcmp(argv[1], "-file") == 0)
    {
        magnet_uri = make_magnet_uri(argv[2]);
    }
    else if (strcmp(argv[1], "-url") == 0)
    {
        magnet_uri = argv[2];
    }
    else
    {
        std::cerr << "Unknown parameter!" << std::endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    lt::session_params params = lt::session_params();

    params.settings.set_int(lt::settings_pack::alert_mask
        , lt::alert_category::error
        | lt::alert_category::storage
        | lt::alert_category::status);

    lt::session ses(params);
    clk::time_point last_save_resume = clk::now();

    // load resume data from disk and pass it in as we add the magnet link
    auto buf = load_file(".resume_file");

    lt::add_torrent_params magnet = lt::parse_magnet_uri(magnet_uri);
    if (buf.size())
    {
        lt::add_torrent_params atp = lt::read_resume_data(buf);
        if (atp.info_hashes.v1 == magnet.info_hashes.v1) magnet = std::move(atp);
    }

    // save in current dir
    magnet.save_path = ".";
    ses.async_add_torrent(std::move(magnet));

    // this is the handle we'll set once we get the notification of it being
    // added
    lt::torrent_handle h;
    std::signal(SIGINT, &sighandler);

    // set when we're exiting
    bool done = false;

    while (!done)
    {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        if (shut_down)
        {
            shut_down = false;
            auto const handles = ses.get_torrents();
            if (1 == handles.size())
            {
                handles[0].save_resume_data(lt::torrent_handle::save_info_dict);
                done = true;
                break;
            }
        }

        for (lt::alert const* a : alerts)
        {
            if (auto at = lt::alert_cast<lt::add_torrent_alert>(a))
            {
                h = at->handle;
            }
            // if we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a))
            {
                h.save_resume_data(lt::torrent_handle::save_info_dict);
                done = true;
            }
            if (lt::alert_cast<lt::torrent_error_alert>(a))
            {
                std::cout << a->message() << std::endl;
                done = true;
                h.save_resume_data(lt::torrent_handle::save_info_dict);
            }

            // when resume data is ready, save it
            if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a))
            {
                std::ofstream of(".resume_file", std::ios_base::binary);
                of.unsetf(std::ios_base::skipws);
                auto const b = write_resume_data_buf(rd->params);
                of.write(b.data(), int(b.size()));
                if (done) 
                    break;
            }

            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a))
            {
                if (done) 
                    break;
            }

            if (auto st = lt::alert_cast<lt::state_update_alert>(a))
            {
                if (st->status.empty()) 
                    continue;

                lt::torrent_status const& s = st->status[0];
                std::cout << '\r' << state(s.state) << ' '
                    << (s.download_payload_rate / 1000) << " kB/s "
                    << (s.total_done / 1000) << " kB ("
                    << (s.progress_ppm / 10000) << "%) downloaded ("
                    << s.num_peers << " peers)\x1b[K";
                std::cout.flush();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        ses.post_torrent_updates();

        if (clk::now() - last_save_resume > std::chrono::seconds(30))
        {
            h.save_resume_data(lt::torrent_handle::save_info_dict);
            last_save_resume = clk::now();
        }
    }

    std::cout << "\nDone, shutting down!" << std::endl;
}