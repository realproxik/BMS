// ___init.cpp - Python Module Initialization
#include <Python.h>
#include <iostream>
#include <string>
#include <memory>

// Forward declarations for SWIG-generated modules
extern "C" {
    PyObject* PyInit_bms_api(void);
    PyObject* PyInit_bms_core(void);
    PyObject* PyInit_bms_network(void);
    PyObject* PyInit_bms_ui(void);
}

// Module initialization functions
static PyMethodDef BMSMethods[] = {
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef BMSModule = {
    PyModuleDef_HEAD_INIT,
    "bms",
    "BMS Browser Python Bindings",
    -1,
    BMSMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

// Main initialization function
PyMODINIT_FUNC PyInit_bms(void) {
    std::cout << "[BMS Python] Initializing BMS module..." << std::endl;
    
    // Initialize main module
    PyObject* module = PyModule_Create(&BMSModule);
    if (!module) {
        std::cerr << "[BMS Python] Failed to create main module" << std::endl;
        return nullptr;
    }
    
    // Initialize submodules
    std::cout << "[BMS Python] Initializing submodules..." << std::endl;
    
    // Import submodules
    PyObject* api_module = PyImport_ImportModule("bms_api");
    if (api_module) {
        PyModule_AddObject(module, "api", api_module);
        std::cout << "[BMS Python] API module loaded" << std::endl;
    }
    
    PyObject* core_module = PyImport_ImportModule("bms_core");
    if (core_module) {
        PyModule_AddObject(module, "core", core_module);
        std::cout << "[BMS Python] Core module loaded" << std::endl;
    }
    
    PyObject* network_module = PyImport_ImportModule("bms_network");
    if (network_module) {
        PyModule_AddObject(module, "network", network_module);
        std::cout << "[BMS Python] Network module loaded" << std::endl;
    }
    
    PyObject* ui_module = PyImport_ImportModule("bms_ui");
    if (ui_module) {
        PyModule_AddObject(module, "ui", ui_module);
        std::cout << "[BMS Python] UI module loaded" << std::endl;
    }
    
    // Add version information
    PyModule_AddStringConstant(module, "__version__", "1.1.0");
    PyModule_AddStringConstant(module, "__author__", "BMS Browser Team");
    
    std::cout << "[BMS Python] BMS module initialized successfully" << std::endl;
    
    return module;
}

// Module cleanup
static void cleanup_module() {
    std::cout << "[BMS Python] Cleaning up BMS module..." << std::endl;
}