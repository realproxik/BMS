#include "TabManager.h"
#include <iostream>
#include <cstdlib>
#include <thread>

int main(int argc, char** argv)
{
    size_t tabs = 10;
    double per_tab_mib = 10.24; // default per-tab target to approximate 102MiB for 10 tabs and ~1GiB for 100 tabs
    int runtime_seconds = 30;

    if (argc > 1) tabs = static_cast<size_t>(std::atoi(argv[1]));
    if (argc > 2) per_tab_mib = std::atof(argv[2]);
    if (argc > 3) runtime_seconds = std::atoi(argv[3]);

    std::cout << "Starting TabManager test: " << tabs << " tabs, " << per_tab_mib << " MiB per tab, runtime " << runtime_seconds << "s\n";

    TabManager manager(per_tab_mib, 1000);
    manager.openTabs(tabs);

    std::cout << "Opened " << manager.count() << " tabs. Sleeping for " << runtime_seconds << "s...\n";
    std::this_thread::sleep_for(std::chrono::seconds(runtime_seconds));

    std::cout << "Test finished. Exiting.\n";
    return 0;
}
