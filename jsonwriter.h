// Copyright (c) 2013 Alexandre Grigorovitch (alexezh@gmail.com).
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include <string>
#include <vector>
#include "jsonstr.h"

class JsonWriter
{
public:
	JsonWriter()
		: _AddSep(false)
		, _Ident(0)
	{
	}
	
	JsonWriter& WriteName(const char * pszName, size_t cchName)
	{
		WriteNameWorker(pszName, cchName);
		return *this;
	}
	JsonWriter& WriteName(const std::string & name)
	{
		WriteNameWorker(name.c_str(), name.length());
		return *this;
	}

	JsonWriter& WriteStringPair(const char * pszName, size_t cchName, const char * pszValue, size_t cchValue)
	{
		WriteNameWorker(pszName, cchName);
		cchValue = (cchValue > 0) ? cchValue : strlen(pszValue);
		WriteString(pszValue, cchValue);
		_AddSep = true;
		return *this;
	}
	JsonWriter& WriteStringPair(const char * pszName, size_t cchName, const std::string & value)
	{
		WriteNameWorker(pszName, cchName);
		WriteString(value.c_str(), value.length());
		_AddSep = true;
		return *this;
	}
	JsonWriter& WriteStringValue(const char * pszValue, size_t cchValue)
	{
		cchValue = (cchValue > 0) ? cchValue : strlen(pszValue);
		WriteString(pszValue, cchValue);
		_AddSep = true;
		return *this;
	}
	JsonWriter& WriteStringValue(const std::string & value)
	{
		WriteString(value.c_str(), value.length());
		_AddSep = true;
		return *this;
	}

	JsonWriter& WriteIntValue(int val)
	{
		// convert int to string and append
		std::vector<char> arVal;
		arVal.resize(32);
		_itoa_s(val, &arVal[0], arVal.size(), 10);
		arVal.resize(strlen(&arVal[0]));
		_Result.insert(_Result.end(), arVal.begin(), arVal.end());
		_AddSep = true;
		return *this;
	}

	JsonWriter& WriteIntPair(const char * pszName, size_t cchName, int val)
	{
		WriteName(pszName, cchName);
		WriteIntValue(val);
		return *this;
	}

	JsonWriter& WriteInt64Value(__int64 val)
	{
		// convert int to string and append
		std::vector<char> arVal;
		arVal.resize(64);
		_i64toa_s(val, &arVal[0], arVal.size(), 10);
		arVal.resize(strlen(&arVal[0]));
		_Result.insert(_Result.end(), arVal.begin(), arVal.end());
		_AddSep = true;
		return *this;
	}

	JsonWriter& WriteInt64Pair(const char * pszName, size_t cchName, __int64 val)
	{
		WriteName(pszName, cchName);
		WriteInt64Value(val);
		return *this;
	}

	JsonWriter& WriteObjectValueStart()
	{
		if(_AddSep)
		{
			_Result.push_back(',');
			_AddSep = false;
		}

		_Result.push_back('\n');
		_Result.push_back('{');
		_Ident++;
		return *this;
	}
	
	JsonWriter& WriteObjectValueEnd()
	{
		_Ident--;
		_Result.push_back('}');
		_AddSep = true;
		return *this;
	}

	JsonWriter& WriteArrayValueStart()
	{
		if(_AddSep)
		{
			_Result.push_back(',');
			_AddSep = false;
		}

		_Result.push_back('[');
		return *this;
	}
	
	JsonWriter& WriteArrayValueEnd()
	{
		_Result.push_back(']');
		_AddSep = true;
		return *this;
	}

	const std::vector<char> & Result() { return _Result; }

private:
	void WriteNameWorker(const char * pszName, size_t cchName)
	{
		if(_AddSep)
		{
			_Result.push_back(',');
			_AddSep = false;
		}

		WriteString(pszName, cchName);
		_Result.push_back(':');
	}

	void WriteString(const char * pszVal, long cchVal)
	{
		_Result.push_back('"');
		for(;*pszVal;pszVal++)
		{
            char c = *pszVal;
			// escape
            switch(c)
            {
                case '"': 
                case '\\': _Result.push_back('\\'); _Result.push_back(c); break;
                case '\b': _Result.push_back('\\'); _Result.push_back('b'); break;
                case '\f': _Result.push_back('\\'); _Result.push_back('f'); break;
                case '\n': _Result.push_back('\\'); _Result.push_back('n'); break;
                case '\r': _Result.push_back('\\'); _Result.push_back('r'); break;
                case '\t': _Result.push_back('\\'); _Result.push_back('t'); break;
                default:
			        _Result.push_back(*pszVal);
                    break;
            }
		}
		_Result.push_back('"');
	}

private:
	std::vector<char> _Result;
	int _Ident;
	std::vector<char> _IdentBuf;
	bool _AddSep;
};
