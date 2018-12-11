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

// module.h: YAL data types
// Author: LYL

#pragma once

#include <limits>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

namespace yal {

    // Correspond to an entry in an IOLIST.
    class Signal {
    public:
        enum class TerminalType {
            BIDIRECTIONAL,  // B
            PAD_INPUT,      // PI
            PAD_OUTPUT,     // PO
            PAD_BIDIRECTIONAL,  // PB
            FEEDTHROUGH,    // F
            POWER,          // PWR (VDD)
            GROUND          // GND (VSS)
        };

        enum class LayerType {
            PDIFF, NDIFF, POLY, METAL1, METAL2
        };

        Signal();

        Signal(const std::string &name, TerminalType terminal_type,
            int xpos, int ypos, int width, LayerType layer_type,
            double current = NaN(), double voltage = NaN());

        static constexpr double NaN() noexcept {
            return std::numeric_limits<double>::signaling_NaN();
        }

        bool is_current_defined() const noexcept;

        bool is_voltage_defined() const noexcept;

        std::string name;
        TerminalType terminal_type;
        int xpos, ypos;
        int width;
        LayerType layer_type;
        double current; // optional
        double voltage; // optional
    };

    // Basic module
    class Module {
    public:
        enum class ModuleType { STANDARD, PAD, GENERAL, PARENT };

        Module() = default;
        Module(const std::string &name, ModuleType type,
            const std::vector<int> &xpos, const std::vector<int> &ypos,
            const std::vector<Signal> &iolist);

        virtual ~Module() = default;

        std::ostream &print() const;

        std::ostream &print(std::ostream &os, 
            const std::string &blank = " ") const;

        // @return max(xpos) - min(xpos); -1 if xpos is empty
        int xspan() const;

        // @return max(ypos) - min(ypos); -1 if ypos is empty
        int yspan() const;

        void clear();

        std::string name;
        ModuleType type;
        std::vector<int> xpos;
        std::vector<int> ypos;
        std::vector<Signal> iolist;

    protected:
         virtual void print_network(std::ostream &os, 
             const std::string &blank) const;

    private:
        static int span(const std::vector<int> &v);
    };

    // A soft module whose type is PARENT
    class ParentModule : public Module {
    public:
        using NetworkEntry = std::tuple<std::string, std::string, 
            std::vector<std::string>>;

        ParentModule();

        ParentModule(const std::string &name, const std::vector<int> &xpos, 
            const std::vector<int> &ypos, const std::vector<Signal> &iolist, 
            const std::vector<NetworkEntry> &network);

        static const std::string &get_instance_name(const NetworkEntry &ne);

        static const std::string &get_module_name(const NetworkEntry &ne);

        static const std::vector<std::string> &get_signal_names(
            const NetworkEntry &ne);

        void clear();

        std::vector<NetworkEntry> network;

    protected:
        virtual void print_network(std::ostream &os, 
            const std::string &blank) const override;
    };

}
