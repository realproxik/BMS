#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstddef>

class Tab {
public:
    explicit Tab(size_t bytes);
    void refresh();
private:
    std::vector<char> data_;
};

class TabManager {
public:
    TabManager(double per_tab_mib = 10.24, int refresh_ms = 1000);
    ~TabManager();

    void openTabs(size_t n);
    void setActive(size_t index);
    size_t count() const;

private:
    void refresherLoop();

    std::vector<std::shared_ptr<Tab>> tabs_;
    std::thread refresher_;
    std::atomic<bool> running_;
    std::atomic<size_t> activeIndex_;
    double perTabMiB_;
    int refreshMs_;
};
