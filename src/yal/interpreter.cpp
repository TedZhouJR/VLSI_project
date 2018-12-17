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

#include "interpreter.h"
#include "module.h"
#include <unordered_map>

using namespace yal;

Interpreter::Interpreter() :
    m_scanner(*this),
    m_parser(m_scanner, *this) {
}

Interpreter::Interpreter(std::istream & is) : Interpreter() {
    switch_input_stream(is);
}

bool Interpreter::parse() {
    return !m_parser.parse();
}

void Interpreter::clear() {
    m_location.initialize();
    m_modules.clear();
    m_parent.clear();
}

std::ostream & Interpreter::print() const {
    return print(std::cout);
}

std::ostream & Interpreter::print(std::ostream & os, 
    const std::string & blank) const {
    for (const Module &m : m_modules)
        m.print(os, blank);
    if (!m_parent.name.empty())
        m_parent.print(os, blank);
    return os;
}

void Interpreter::switch_input_stream(std::istream &is) {
    m_scanner.switch_streams(&is, nullptr);
}

std::vector<std::size_t> Interpreter::make_module_index() const {
    unordered_map<string, size_t> map;
    map.reserve(m_modules.size());
    size_t cnt = 0;
    for (const Module &m : m_modules) {
        auto ib = map.emplace(m.name, cnt++);
        if (!ib.second) 
            throw runtime_error("Conflicting module name: " + m.name);
    }
    vector<size_t> index;
    index.reserve(m_parent.network.size());
    for (const auto &e : m_parent.network) {
        const string &name = ParentModule::get_module_name(e);
        auto it = map.find(name);
        if (it == map.end())
            throw runtime_error("Invalid module name: " + name);
        index.push_back(it->second);
    }
    return index;
}
