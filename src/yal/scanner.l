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

%{
	#include <algorithm>
	#include <cstdlib>
	#include <iostream>
	#include "scanner.h"
	#include "interpreter.h"
	#include "parser.hpp"
	#include "location.hh"

	using namespace std;

	// Original yyterminate() macro returns int. Since we're using Bison 3 variants
	// as tokens, we must redefine it to change type from `int` to `Parser::semantic_type`
	#define yyterminate() yal::Parser::make_END(yal::location(m_driver.location()));

	// This will track current scanner location.
	// Action is called when length of the token is known.
	#define YY_USER_ACTION do { m_driver.step(); m_driver.columns(yyleng); } while (false);
	
	// !!!WARNING!!!
	// Location API is used, but the location is not initialized, 'cause I'm lazy. When making
	// a token with make_{something} method you can pass detailed token location. Current location
	// is accessible with m_driver.location() method. All puzzle elements are there - just
	// pass location value in every action code block below. I'm going to waste more time writing
	// this excuse than putting this boilerplate below...
	//
	// Location class can be found in location.hh and posistion.hh files. It's just a bit too much
	// boilerplate for this small example. Bummer.
%}

%option nodefault
%option noyywrap
%option c++
%option yyclass="Scanner"
%option prefix="yal_"

%%

"//".*		{	/* Single-line comment */	}

[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]		{	
				// Multi-line comment 	 
				m_driver.lines(static_cast<int>(std::count(yytext, yytext + yyleng, '\n')));
			}

[/][*]		{ 
				m_driver.m_parser.error(m_driver.location(), "Unterminated comment");
			}

IOLIST		{
				return yal::Parser::make_IOLIST(yal::location(m_driver.location())); 
			}

ENDIOLIST	{
				return yal::Parser::make_ENDIOLIST(yal::location(m_driver.location())); 
			}

NETWORK		{
				return yal::Parser::make_NETWORK(yal::location(m_driver.location())); 
			}

ENDNETWORK	{
				return yal::Parser::make_ENDNETWORK(yal::location(m_driver.location())); 
			}

MODULE		{
				return yal::Parser::make_MODULE(yal::location(m_driver.location())); 
			}

ENDMODULE	{
				return yal::Parser::make_ENDMODULE(yal::location(m_driver.location())); 
			}

TYPE		{
				return yal::Parser::make_TYPE(yal::location(m_driver.location())); 
			}

DIMENSIONS	{
				return yal::Parser::make_DIMENSIONS(yal::location(m_driver.location())); 
			}

CURRENT		{
				return yal::Parser::make_CURRENT(yal::location(m_driver.location())); 
			}

VOLTAGE		{
				return yal::Parser::make_VOLTAGE(yal::location(m_driver.location())); 
			}

STANDARD	{
				return yal::Parser::make_STANDARD(yal::location(m_driver.location())); 
			}

PAD			{
				return yal::Parser::make_PAD(yal::location(m_driver.location())); 
			}

GENERAL		{
				return yal::Parser::make_GENERAL(yal::location(m_driver.location())); 
			}

PARENT		{
				return yal::Parser::make_PARENT(yal::location(m_driver.location())); 
			}

B			{
				return yal::Parser::make_BIDIRECTIONAL(yal::location(m_driver.location())); 
			}

PI			{
				return yal::Parser::make_PAD_INPUT(yal::location(m_driver.location())); 
			}

PO			{
				return yal::Parser::make_PAD_OUTPUT(yal::location(m_driver.location())); 
			}

PB			{
				return yal::Parser::make_PAD_BIDIRECTIONAL(yal::location(m_driver.location())); 
			}

F			{
				return yal::Parser::make_FEEDTHROUGH(yal::location(m_driver.location())); 
			}

PWR			{
				return yal::Parser::make_POWER(yal::location(m_driver.location())); 
			}

GND			{
				return yal::Parser::make_GROUND(yal::location(m_driver.location())); 
			}

PDIFF		{
				return yal::Parser::make_PDIFF(yal::location(m_driver.location())); 
			}

NDIFF		{
				return yal::Parser::make_NDIFF(yal::location(m_driver.location())); 
			}

POLY		{
				return yal::Parser::make_POLY(yal::location(m_driver.location())); 
			}

METAL1		{
				return yal::Parser::make_METAL1(yal::location(m_driver.location())); 
			}

METAL2		{
				return yal::Parser::make_METAL2(yal::location(m_driver.location())); 
			}

[a-zA-Z][a-zA-Z0-9_]*	{ 
                return yal::Parser::make_STRING(yytext, yal::location(m_driver.location())); 
			 }
            
;           {
                return yal::Parser::make_SEMICOLON(yal::location(m_driver.location()));
            }

\n			{
				m_driver.lines();			
			}
            
[\r\t ]		{	/* ignore whitespace */		}
	
[-+]?([1-9][0-9]*|0)	{
				std::int32_t number = strtol(yytext, 0, 10);
				return yal::Parser::make_INTEGER(number, yal::location(m_driver.location()));
			}

[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?	{
				double number = strtod(yytext, nullptr);
				return yal::Parser::make_DOUBLE(number, yal::location(m_driver.location()));
			}

.           { 
				m_driver.m_parser.error(m_driver.location(), 
					std::string("Unknown character ") + yytext);
            }
            
<<EOF>>     {	return yyterminate();	}


%%
