#pragma once

// BMSIntegration.h
// Lightweight, non-invasive stubs for BMS-specific integration into the Chromium tree.
// These files are placeholders to help track and review real changes before applying
// them to Chromium's production codebase.

namespace bms {

// Initialize BMS integration. Safe to call multiple times.
void InitializeBMSIntegration();

// Shutdown BMS integration.
void ShutdownBMSIntegration();

} // namespace bms
