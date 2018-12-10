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

%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define parser_class_name { Parser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { yal }
%code requires
{
    #include <iostream>
    #include <string>
    #include <vector>
    #include <cstdint>
	#include <stdexcept>
	#include <sstream>
	#include <limits>
    #include "module.h"

    using namespace std;

    namespace yal {
        class Scanner;
        class Interpreter;

		/*
		class SyntaxError : public std::runtime_error {
		public:
			SyntaxError(const std::string &msg, const location &loc) : 
				runtime_error(msg), loc_(loc) {
					std::ostringstream out;
					out << "Syntax error (" << loc_ << "): " << msg;
					what_ = out.str();
				}

			virtual const char *what() const override noexcept {
				return what_.c_str();
			}

		private:
			location loc_;
			std::string what_;
		}
		*/
    }
}

// Bison calls yylex() function that must be provided by us to suck tokens
// from the scanner. This block will be placed at the beginning of IMPLEMENTATION file (cpp).
// We define this function here (function! not method).
// This function is called only inside Bison, so we make it static to limit symbol visibility for the linker
// to avoid potential linking conflicts.
%code top
{
    #include <iostream>
	#include <memory>	// for shared_ptr
    #include "scanner.h"
    #include "parser.hpp"
    #include "interpreter.h"
    #include "location.hh"
    
    // yylex() arguments are defined in parser.y
    static yal::Parser::symbol_type yylex(yal::Scanner &scanner, yal::Interpreter &driver) {
        return scanner.get_next_token();
    }
    
    // you can accomplish the same thing by inlining the code using preprocessor
    // x and y are same as in above static function
    // #define yylex(x, y) scanner.get_next_token()
    
    using namespace yal;
}

%lex-param { yal::Scanner &scanner }
%lex-param { yal::Interpreter &driver }
%parse-param { yal::Scanner &scanner }
%parse-param { yal::Interpreter &driver }
%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%token END 0 "end of file"
%token <std::string> STRING  "string";
%token <std::int32_t> INTEGER "number";
%token <double> DOUBLE "double"
%token SEMICOLON "semicolon";

%token MODULE "module";
%token ENDMODULE "endmodule";
%token TYPE "type";
%token DIMENSIONS "dimensions";

%token IOLIST "iolist";
%token ENDIOLIST "endiolist";
%token NETWORK "network";
%token ENDNETWORK "endnetwork";

%token CURRENT "current";
%token VOLTAGE "voltage";

%token STANDARD "standard";
%token PAD "pad";
%token GENERAL "general";
%token PARENT "parent";

%token BIDIRECTIONAL "bidirectional"
%token PAD_INPUT "pad input"
%token PAD_OUTPUT "pad output"
%token PAD_BIDIRECTIONAL "pad bidirectional"
%token FEEDTHROUGH "feedthrough"
%token POWER "power"
%token GROUND "ground"

%token PDIFF "pdiff"
%token NDIFF "ndiff"
%token POLY "poly"
%token METAL1 "metal1"
%token METAL2 "metal2"

%type<std::string> Head String StringOrInteger
%type<yal::Module::ModuleType> Type ModuleType;
%type<std::pair<std::vector<int>, std::vector<int> > > Dimensions DimensionList;
%type<std::vector<yal::Signal> > Iolist IolistBody;
%type<yal::Signal> IolistEntry;
%type<yal::Signal::TerminalType> TerminalType;
%type<yal::Signal::LayerType> LayerType;
%type<double> Double Current Voltage;
%type<std::vector<yal::ParentModule::NetworkEntry> > Network NetworkBody;
%type<yal::ParentModule::NetworkEntry> NetworkEntry;

%start File

%%

File		:	File Module
			|	// empty
			;

Module		:	Head Type Dimensions Iolist Network ENDMODULE SEMICOLON
				{
					if ($5.empty()) {
						// Parent module
						driver.m_modules.emplace_back($1, $2, $3.first, $3.second, $4);
					} else {
						// Hard module
						driver.m_parent = yal::ParentModule($1, $3.first, $3.second, $4, $5);
					}
				}
			;

String		:	STRING
				{
					$$ = std::move($1);
				}
			|	GROUND
				{
					$$ = "GND";
				}
			;

Double		:	DOUBLE
				{
					$$ = $1;
				}
			|	INTEGER
				{
					$$ = $1;
				}
			;

StringOrInteger:	String
				{
					$$ = std::move($1);
				}
			|	INTEGER
				{
					$$ = std::to_string($1);
				}
			;

Head		:	MODULE String SEMICOLON
				{
					$$ = std::move($2);
				}	
			;

Type		:	TYPE ModuleType SEMICOLON
				{
					$$ = $2;
				}
			;

ModuleType	:	STANDARD	
				{
					$$ = yal::Module::ModuleType::STANDARD; 
				}
			|	PAD	
				{
					$$ = yal::Module::ModuleType::PAD; 
				}
			|	GENERAL		
				{
					$$ = yal::Module::ModuleType::GENERAL; 
				}
			|	PARENT
				{
					$$ = yal::Module::ModuleType::PARENT; 
				}
			;

Dimensions	:	DIMENSIONS DimensionList SEMICOLON
				{
					$$ = std::move($2);
				}
			|	// empty
			;

DimensionList:	DimensionList INTEGER INTEGER
				{
					$$ = std::move($1);
					$$.first.push_back($2);
					$$.second.push_back($3);
				}
			|	// empty
			;

Iolist		:	IOLIST SEMICOLON IolistBody ENDIOLIST SEMICOLON
				{
					$$ = std::move($3);
				}
			|	// empty
			;

IolistBody	:	IolistBody IolistEntry SEMICOLON
				{
					$$ = std::move($1);
					$$.push_back(std::move($2));
				}
			|	// empty
			;

IolistEntry	:	String TerminalType INTEGER INTEGER INTEGER LayerType Current Voltage
				{
					$$ = yal::Signal($1, $2, $3, $4, $5, $6, $7, $8);
				}
			;

TerminalType:	BIDIRECTIONAL
				{
					$$ = yal::Signal::TerminalType::BIDIRECTIONAL;
				}
			|	PAD_INPUT
				{
					$$ = yal::Signal::TerminalType::PAD_INPUT;
				}
			|	PAD_OUTPUT
				{
					$$ = yal::Signal::TerminalType::PAD_OUTPUT;
				}
			|	PAD_BIDIRECTIONAL
				{
					$$ = yal::Signal::TerminalType::PAD_BIDIRECTIONAL;
				}
			|	FEEDTHROUGH
				{
					$$ = yal::Signal::TerminalType::FEEDTHROUGH;
				}
			|	POWER
				{
					$$ = yal::Signal::TerminalType::POWER;
				}
			|	GROUND
				{
					$$ = yal::Signal::TerminalType::GROUND;
				}
			;

LayerType	:   PDIFF
				{
					$$ = yal::Signal::LayerType::PDIFF;
				}
			|	NDIFF
				{
					$$ = yal::Signal::LayerType::NDIFF;
				}
			|	POLY
				{
					$$ = yal::Signal::LayerType::POLY;
				}
			|	METAL1
				{
					$$ = yal::Signal::LayerType::METAL1;
				}
			|	METAL2
				{
					$$ = yal::Signal::LayerType::METAL2;
				}
			;

Current		:	CURRENT Double
				{
					$$ = $2;
				}
			|	// empty
				{
					$$ = yal::Signal::NaN();
				}
			;

Voltage		:	VOLTAGE Double
				{
					$$ = $2;
				}
			|	// empty
				{
					$$ = yal::Signal::NaN();
				}
			;

Network		:	NETWORK SEMICOLON NetworkBody ENDNETWORK SEMICOLON
				{
					$$ = std::move($3);
				}
			|	// empty
			;

NetworkBody	:	NetworkBody	NetworkEntry SEMICOLON
				{
					$$ = std::move($1);
					$$.push_back(std::move($2));				
				}
			|	// empty
			;

NetworkEntry:	NetworkEntry StringOrInteger
				{
					$$ = std::move($1);
					std::get<2>($$).push_back(std::move($2));
				}
			|	String String
				{
					std::get<0>($$) = std::move($1);
					std::get<1>($$) = std::move($2);
				}
			;
    
%%

// Bison expects us to provide implementation - otherwise linker complains
void yal::Parser::error(const location_type &loc, const std::string &msg) {
	throw syntax_error(loc, msg);
}
