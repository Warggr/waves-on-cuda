#include "grid.hpp"
#include "viewer.hpp"
#include <ostream>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
    for (const auto& row : grid) {
        for (const auto& col : row) {
            os << col << " ";
        }
        os << "\n";
    }
    return os;
}

namespace po = boost::program_options;

struct RunConfig {
    bool perf = false;
    int niters = 0;
};

RunConfig parse_options(int argc, char* argv[]) {
    RunConfig config;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",      "Show help")
        ("perf,p", po::value<int>(&config.niters),"Run without GUI for [N] iterations to test performance")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(1);
    }
    if (vm.count("perf")) {
        config.perf = true;
        config.niters = vm["perf"].as<int>();
    }
    return config;
}

int main(int argc, char* argv[]) {
    auto options = parse_options(argc, argv);

    using namespace std::chrono;

    const double dt = 0.01;
    const std::size_t GRID_WIDTH = 100, GRID_HEIGHT = 100;

    World world(GRID_HEIGHT, GRID_WIDTH, dt);

    if (options.perf) {
        std::cout << "Starting...";
        auto t1 = high_resolution_clock::now();
        for (int i = 0; i < options.niters; i++) {
            world.step();
        }
        auto t2 = high_resolution_clock::now();
        duration<double, std::milli> runtime = t2 - t1;
        std::cout << "Finished in " << runtime.count() << " ms" << std::endl;
    } else {
        Viewer myGlfw;

        const steady_clock::duration dt_as_duration = duration_cast<steady_clock::duration>(duration<float, std::milli>(1000 * dt));
        auto tick_time = steady_clock::now();

        try {
            while (true) {
                world.step(true);
                myGlfw.render(world.grid());
                tick_time += dt_as_duration;
                std::this_thread::sleep_until(tick_time);
            }
        } catch (const Viewer::WindowClosed&) {

        }
    }
}
