/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

// module.cpp: YAL data types
// Author: LYL

#include "module.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace yal;

Module::Module(const std::string & name, ModuleType type, 
    const std::vector<int>& xpos, const std::vector<int>& ypos, 
    const std::vector<Signal>& iolist) : name(name), type(type),
    xpos(xpos), ypos(ypos), iolist(iolist) {}

std::ostream & Module::print() const {
    return print(std::cout);
}

std::ostream & Module::print(std::ostream & os, 
    const std::string &blank) const {
    // MODULE
    os << "MODULE " << name << ";\n";

    // TYPE
    os << blank << "TYPE ";
    switch (type) {
    case Module::ModuleType::GENERAL:
        os << "GENERAL";
        break;
    case Module::ModuleType::PAD:
        os << "PAD";
        break;
    case Module::ModuleType::PARENT:
        os << "PARENT";
        break;
    case Module::ModuleType::STANDARD:
        os << "STANDARD";
        break;
    default:
        assert(false);
    }
    os << ";\n";

    // DIMENSIONS
    os << blank << "DIMENSIONS";
    for (std::size_t i = 0; i != xpos.size(); ++i)
        os << " " << xpos[i] << " " << ypos[i];
    os << ";\n";

    // IOLIST
    os << blank << "IOLIST;\n";
    for (const Signal &s : iolist) {
        os << blank << blank;
        os << s.name << " ";
        switch (s.terminal_type) {
        case Signal::TerminalType::BIDIRECTIONAL:
            os << "B";
            break;
        case Signal::TerminalType::FEEDTHROUGH:
            os << "F";
            break;
        case Signal::TerminalType::GROUND:
            os << "GND";
            break;
        case Signal::TerminalType::PAD_BIDIRECTIONAL:
            os << "PB";
            break;
        case Signal::TerminalType::PAD_INPUT:
            os << "PI";
            break;
        case Signal::TerminalType::PAD_OUTPUT:
            os << "PO";
            break;
        case Signal::TerminalType::POWER:
            os << "PWR";
            break;
        default:
            assert(false);
        }

        os << " " << s.xpos << " " << s.ypos << " " << s.width << " ";
        switch (s.layer_type) {
        case Signal::LayerType::METAL1:
            os << "METAL1";
            break;
        case Signal::LayerType::METAL2:
            os << "METAL2";
            break;
        case Signal::LayerType::NDIFF:
            os << "NDIFF";
            break;
        case Signal::LayerType::PDIFF:
            os << "PDIFF";
            break;
        case Signal::LayerType::POLY:
            os << "POLY";
            break;
        default:
            assert(false);
        }

        if (s.is_current_defined())
            os << " CURRENT " << s.current;
        if (s.is_voltage_defined())
            os << " VOLTAGE " << s.voltage;
        os << ";\n";
    }

    os << blank << "ENDIOLIST;\n";

    // NETWORK if parent module
    print_network(os, blank);

    os << "ENDMODULE;\n";
    return os;
}

int yal::Module::xspan() const {
    return span(xpos);
}

int yal::Module::yspan() const {
    return span(ypos);
}

void Module::clear() {
    name.clear();
    xpos.clear();
    ypos.clear();
    iolist.clear();
}

void Module::print_network(std::ostream & os, 
    const std::string & blank) const {
}

int yal::Module::span(const std::vector<int>& v) {
    if (v.empty())
        return -1;
    auto iters = std::minmax_element(v.begin(), v.end());
    return *iters.second - *iters.first;
}

ParentModule::ParentModule() : Module() {
    this->type = ModuleType::PARENT;
}

ParentModule::ParentModule(const std::string & name, 
    const std::vector<int>& xpos, const std::vector<int>& ypos, 
    const std::vector<Signal>& iolist,
    const std::vector<NetworkEntry>& network) :
    Module(name, ModuleType::PARENT, xpos, ypos, iolist),
    network(network) {
}

const std::string & ParentModule::get_instance_name(
    const NetworkEntry & ne) {
    return std::get<0>(ne);
}

const std::string & ParentModule::get_module_name(
    const NetworkEntry & ne) {
    return std::get<1>(ne);
}

const std::vector<std::string>& ParentModule::get_signal_names(
    const NetworkEntry & ne) {
    return std::get<2>(ne);
}

void ParentModule::clear() {
    Module::clear();
    network.clear();
}

void ParentModule::print_network(std::ostream & os, 
    const std::string & blank) const {
    os << blank << "NETWORK;\n";
    for (const ParentModule::NetworkEntry &ne : network) {
        os << blank << blank << ParentModule::get_instance_name(ne);
        os << " " << ParentModule::get_module_name(ne);
        for (const std::string &sig : ParentModule::get_signal_names(ne))
            os << " " << sig;
        os << ";\n";
    }
    os << blank << "ENDNETWORK;\n";
}

Signal::Signal() : current(NaN()), voltage(NaN()) {}

Signal::Signal(const std::string & name,
    TerminalType terminal_type, int xpos, int ypos, int width, 
    LayerType layer_type, double current, double voltage) :
    name(name), terminal_type(terminal_type),
    xpos(xpos), ypos(ypos), width(width), layer_type(layer_type),
    current(current), voltage(voltage) {
}

bool Signal::is_current_defined() const noexcept {
    return !std::isnan(current);
}

bool Signal::is_voltage_defined() const noexcept {
    return !std::isnan(voltage);
}
