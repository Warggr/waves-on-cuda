#include "grid.hpp"
#include "viewer.hpp"
#include <boost/program_options.hpp>
#include <ostream>
#include <chrono>
#include <iostream>
#include <variant>

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

struct UIRunConfig {
};

struct PerfRunConfig {
    std::vector<unsigned int> niters;
};

struct RunConfig {
    std::size_t grid_size = 100;
    double time_step = 0.01;
    std::variant<UIRunConfig, PerfRunConfig> specific_config;
};

RunConfig parse_options(int argc, char* argv[]) {
    RunConfig config;
    po::options_description regular_options;
    regular_options.add_options()
        ("help,h",      "Show help")
        ("perf,p","Run without GUI for [N] iterations to test performance")
	("size,s", po::value<unsigned int>(), "Set grid size to s")
	("timestep,t", po::value<double>())
    ;

    po::options_description hidden;
    hidden.add_options()
        ("niters", po::value<std::vector<unsigned int>>())
    ;

    po::positional_options_description p;
    p.add("niters",-1);

    po::options_description desc("Allowed options");
    desc.add(regular_options);

    po::options_description cmdline_options;
    cmdline_options.add(regular_options).add(hidden);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .positional(p)
            .options(cmdline_options)
            .run(),
        vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(1);
    }
    if (vm.count("perf")) {
        PerfRunConfig config_;
        config_.niters = vm["niters"].as<std::vector<unsigned int>>();
        config.specific_config = config_;
    } else {
        config.specific_config = UIRunConfig();
    }
    if (vm.count("size")) {
        config.grid_size = vm["size"].as<unsigned int>();
    }
	if (vm.count("timestep")) {
		config.time_step = vm["timestep"].as<double>();
	}
    return config;
}

int main(int argc, char* argv[]) {
    auto options = parse_options(argc, argv);

    using namespace std::chrono;

    World world(options.grid_size, options.grid_size, options.time_step);

    auto config = std::get_if<PerfRunConfig>(&options.specific_config);
    if (config) {
        std::cout << "#N,time[ms]" << std::endl;
        for (const unsigned int niters : config->niters) {
            std::cout << niters << ",";
            world.reset();
            auto t1 = high_resolution_clock::now();
            world.multi_step(niters);
            world.synchronize();
            auto t2 = high_resolution_clock::now();
            duration<double, std::milli> runtime = t2 - t1;
            std::cout << runtime.count() << std::endl;
        }
    } else {
        Viewer myGlfw;

        const steady_clock::duration dt_as_duration = duration_cast<steady_clock::duration>(duration<float, std::milli>(1000 * options.time_step));
        auto tick_time = steady_clock::now();

        try {
            while (true) {
                world.step();
                world.synchronize();
                myGlfw.render(world.grid());
                tick_time += dt_as_duration;
                std::this_thread::sleep_until(tick_time);
            }
        } catch (const Viewer::WindowClosed&) {

        }
    }
}
