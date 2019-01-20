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

#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <vector>

#include "scanner.h"
#include "parser.hpp"  
#include "position.hh"

namespace yal {

    // forward declare our simplistic AST node class so we
    // can declare container for it without the header
    class Module;
    class ParentModule;

    /**
     * This class is the interface for our scanner/lexer. The end user
     * is expected to use this. It drives scanner/lexer, keeps
     * parsed AST and generally is a good place to store additional
     * context data. Both parser and lexer have access to it via internal
     * references.
     */
    class Interpreter {
    public:
        using location_type = class location;

        friend class Parser;
        friend class Scanner;

        Interpreter();

        explicit Interpreter(std::istream &is);

        // Run parser. Results are stored inside.
        // @return true on success, false on failure
        // @throw yal::Parser::syntax_error when things go wrong
        bool parse();
        
        // Clear AST.
        void clear();

        // Print AST in YAL format.
        std::ostream &print() const;

        // Print AST in YAL format.
        std::ostream &print(std::ostream &os, 
            const std::string &blank = " ") const;
        
        // Switch scanner input stream. Default is standard input (std::cin).
        // It will also reset AST.
        void switch_input_stream(std::istream &is);

        const std::vector<Module> &modules() const noexcept {
            return m_modules;
        }

        const ParentModule &parent_module() const noexcept {
            return m_parent;
        }

        // Compute indices of modulenames in the parent module.
        // @throw runtime_error if module name conflict or invalid
        std::vector<std::size_t> make_module_index() const;

    private:
        void columns(int count = 1) {
            m_location.columns(count);
        }

        void lines(int count = 1) {
            m_location.lines(count);
        }

        void step() {
            m_location.step();
        }

        const location_type &location() const noexcept {
            return m_location;
        }

        Scanner m_scanner;
        Parser m_parser;
        location_type m_location;       // Used by scanner
        std::vector<Module> m_modules;  // Not including parent module
        ParentModule m_parent;          // Top-level module
    };

}