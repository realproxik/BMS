#include "TabManager.h"
#include <iostream>

Tab::Tab(size_t bytes) : data_(bytes)
{
    // Touch one byte per page to force allocation
    for (size_t i = 0; i < data_.size(); i += 4096) {
        data_[i] = static_cast<char>(i & 0xFF);
    }
}

void Tab::refresh()
{
    // Simple refresh: touch one byte per page to simulate activity
    for (size_t i = 0; i < data_.size(); i += 4096) {
        data_[i] = static_cast<char>((data_[i] + 1) & 0xFF);
    }
}

TabManager::TabManager(double per_tab_mib, int refresh_ms)
    : running_(false), activeIndex_(0), perTabMiB_(per_tab_mib), refreshMs_(refresh_ms)
{
}

TabManager::~TabManager()
{
    running_ = false;
    if (refresher_.joinable())
        refresher_.join();
}

void TabManager::openTabs(size_t n)
{
    size_t per_tab_bytes = static_cast<size_t>(perTabMiB_ * 1024.0 * 1024.0);
    tabs_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        tabs_.emplace_back(std::make_shared<Tab>(per_tab_bytes));
    }

    running_ = true;
    refresher_ = std::thread(&TabManager::refresherLoop, this);
}

void TabManager::setActive(size_t index)
{
    if (index < tabs_.size())
        activeIndex_ = index;
}

size_t TabManager::count() const
{
    return tabs_.size();
}

void TabManager::refresherLoop()
{
    while (running_) {
        // Refresh background tabs only
        for (size_t i = 0; i < tabs_.size(); ++i) {
            if (i == activeIndex_)
                continue; // skip active tab (simulate foreground being handled elsewhere)
            if (tabs_[i])
                tabs_[i]->refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(refreshMs_));
    }
}
