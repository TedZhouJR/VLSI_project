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
#include <cmath>

using namespace yal;

Module::Module(const std::string & name, ModuleType type, 
    const std::vector<int>& xpos, const std::vector<int>& ypos, 
    const std::vector<Signal>& iolist) : name(name), type(type),
    xpos(xpos), ypos(ypos), iolist(iolist) {}

void Module::print(std::ostream & out, std::size_t ident) const {
    // MODULE
    std::string blank(ident, ' ');
    out << "MODULE " << name << ";\n";

    // TYPE
    out << blank << "TYPE ";
    switch (type) {
    case Module::ModuleType::GENERAL:
        out << "GENERAL";
        break;
    case Module::ModuleType::PAD:
        out << "PAD";
        break;
    case Module::ModuleType::PARENT:
        out << "PARENT";
        break;
    case Module::ModuleType::STANDARD:
        out << "STANDARD";
        break;
    default:
        ;
    }
    out << ";\n";

    // DIMENSIONS
    out << blank << "DIMENSIONS";
    for (std::size_t i = 0; i != xpos.size(); ++i)
        out << " " << xpos[i] << " " << ypos[i];
    out << ";\n";

    // IOLIST
    out << blank << "IOLIST;\n";
    for (const Signal &s : iolist) {
        out << blank << blank;
        out << s.name << " ";
        switch (s.terminal_type) {
        case Signal::TerminalType::BIDIRECTIONAL:
            out << "B";
            break;
        case Signal::TerminalType::FEEDTHROUGH:
            out << "F";
            break;
        case Signal::TerminalType::GROUND:
            out << "GND";
            break;
        case Signal::TerminalType::PAD_BIDIRECTIONAL:
            out << "PB";
            break;
        case Signal::TerminalType::PAD_INPUT:
            out << "PI";
            break;
        case Signal::TerminalType::PAD_OUTPUT:
            out << "PO";
            break;
        case Signal::TerminalType::POWER:
            out << "PWR";
            break;
        default:
            ;
        }

        out << " " << s.xpos << " " << s.ypos << " " << s.width << " ";
        switch (s.layer_type) {
        case Signal::LayerType::METAL1:
            out << "METAL1";
            break;
        case Signal::LayerType::METAL2:
            out << "METAL2";
            break;
        case Signal::LayerType::NDIFF:
            out << "NDIFF";
            break;
        case Signal::LayerType::PDIFF:
            out << "PDIFF";
            break;
        case Signal::LayerType::POLY:
            out << "POLY";
            break;
        default:
            ;
        }

        if (s.is_current_defined())
            out << " CURRENT " << s.current;
        if (s.is_voltage_defined())
            out << " VOLTAGE " << s.voltage;
        out << ";\n";
    }

    out << blank << "ENDIOLIST;\n";

    // NETWORK if parent module
    print_network(out, ident);

    out << "ENDMODULE;\n";
}

void Module::clear() {
    name.clear();
    xpos.clear();
    ypos.clear();
    iolist.clear();
}

void Module::print_network(std::ostream & out, 
    std::size_t ident) const {

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

void ParentModule::print_network(std::ostream & out, 
    std::size_t ident) const {
    std::string blank(ident, ' ');
    out << blank << "NETWORK;\n";
    for (const ParentModule::NetworkEntry &ne : network) {
        out << blank << blank << ParentModule::get_instance_name(ne);
        out << " " << ParentModule::get_module_name(ne);
        for (const std::string &sig : ParentModule::get_signal_names(ne))
            out << " " << sig;
        out << ";\n";
    }
    out << blank << "ENDNETWORK;\n";
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
