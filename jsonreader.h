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
#include <stack>
#include "jsonstr.h"

class JsonReader
{
public:
	enum class NodeType
	{
		None,
		String,
		Int,
		Array,
		EndArray,
		Object,
		EndObject,
	};

public:
	JsonReader(const char * pszBlob, size_t cchBlob)
	{
		InitState(pszBlob, cchBlob);
	}

	JsonReader(const std::vector<char> & blob)
	{
		InitState(blob.data(), blob.size());
	}

	// read next element
	bool ReadNext()
	{
		char c;
		bool ret = false;

		if(_ReachedEnd)
		{
			return false;
		}

		_Name.resize(0);
		_Value.resize(0);

		for(;;)
		{
			c = ReadChar();
			if(c == '\0') 
			{
				break;
			}
			if(_State == ParseState::StartReadName)
			{
				if(IsWhite(c))
				{
					;
				}
				else if(c == '"')
				{
					_State = ParseState::ReadName;
				}
				else 
				{
					assert(false);
					break;
				}
			}
			else if(_State == ParseState::ReadName)
			{
				// for now ignore escaping
				if(c != '"')
				{
					_Name.push_back(c);
				}
				else
				{
					_State = ParseState::ReadColon;
				}
			}
			else if(_State == ParseState::ReadColon)
			{
				if(IsWhite(c))
				{
					;
				}
				else if(c == ':')
				{
					_State = ParseState::StartReadValue;
				}
				else 
				{
					assert(false);
					break;
				}
			}
			else if(_State == ParseState::StartReadValue)
			{
				if(IsWhite(c))
				{
					;
				}
				else if(c == '"')
				{
					_Type = NodeType::String;
					_State = ParseState::ReadStringValue;
				}
				else if(c == '[')
				{
					_Type = NodeType::Array;

					// start next iteration from reading values
					_State = ParseState::StartReadValue;
					_ParseStack.push(ParseNode(_Name, NodeType::Array));
					ret = true;
					break;
				}
				else if(c == '{')
				{
					_Type = NodeType::Object;

					// start next iteration from reading name
					_State = ParseState::StartReadName;
					_ParseStack.push(ParseNode(_Name, NodeType::Object));
					ret = true;
					break;
				}
				else if((c >= '0' && c <= '9') || c == '-')
				{
					_Type = NodeType::Int;
					_State = ParseState::ReadIntValue;

                    _Value.push_back(c);
                    if(!(PeekChar() >= '0' && PeekChar() <= '9'))
                    {
                        _IntValue = _atoi64(&_Value[0]);
                        _State = ParseState::EndReadValue;
						ret = true;
						break;
                    }
				}
				else if(c == ']')
				{
					// empty array
					_State = ParseState::EndReadValue;
					_Type = NodeType::EndArray;
					_Name = _ParseStack.top().Name;
					_ParseStack.pop();
					ret = true;
					break;
				}
				else
				{
					assert(false);
					break;
				}
			}
			else if(_State == ParseState::ReadStringValue)
			{
				// for now ignore escaping
                if(c == '\\')
                {
                    c = ReadUnescape();
					_Value.push_back(c);
                }
				else if(c != '"')
				{
					_Value.push_back(c);
				}
				else
				{
					_State = ParseState::EndReadValue;
					ret = true;
					break;
				}
			}
			else if(_State == ParseState::ReadIntValue)
			{
                _Value.push_back(c);
                if(!(PeekChar() >= '0' && PeekChar() <= '9'))
                {
                    _IntValue = _atoi64(&_Value[0]);
                    _State = ParseState::EndReadValue;
					ret = true;
					break;
                }
			}
			else if(_State == ParseState::EndReadValue)
			{
				if(IsWhite(c))
				{
					continue;
				}
				else if(c == ',')
				{
					// if we inside array, keep reading values
					// otherwise read name
					if(_ParseStack.size() > 0 && _ParseStack.top().Type == NodeType::Array)
					{
						_State = ParseState::StartReadValue;
					}
					else
					{
						_State = ParseState::StartReadName;
					}

					continue;
				}
				else if(c == '}')
				{
					// keep parse state as EndReadValue
					// on next iteration we might get another closure 
					_State = ParseState::EndReadValue;
					_Type = NodeType::EndObject;
					_Name = _ParseStack.top().Name;
					_ParseStack.pop();
					ret = true;
					break;
				}
				else if(c == ']')
				{
					// keep parse state as EndReadValue
					// on next iteration we might get another closure 
					_State = ParseState::EndReadValue;
					_Type = NodeType::EndArray;
					_Name = _ParseStack.top().Name;
					_ParseStack.pop();
					ret = true;
					break;
				}
				else
				{
					assert(false);
				}
			}
		}

		return ret;
	}

	// skips the content of current node
	// read to next 
	bool ReadNextValue()
	{
		if(!(_Type == NodeType::Array || _Type == NodeType::Object))
		{
			return ReadNext();
		}

		NodeType startType = _Type;
		NodeType endType = (_Type == NodeType::Array) ? NodeType::EndArray : NodeType::EndObject;
		int level = 1;

		// skip until matching end object
		_Skip = true;
		while(ReadNext())
		{
			if(_Type == startType)
			{
				level++;
			}
			else if(_Type == endType)
			{
				level--;
				if(level == 0)
				{
					break;
				}
			}
		}
		_Skip = false;
		if(level != 0)
		{
			return false;
		}

		return ReadNext();
	}

	bool IsEnd() { return _ReachedEnd; }
	NodeType Type() { return _Type; }
    bool IsString() { return _Type == NodeType::String; }
	const std::string & Name() { return _Name; }
	const std::string & Value() { return _Value; }
	int IntValue() { return (int)_IntValue; }
	__int64 Int64Value() { return _IntValue; }

private:
	bool IsWhite(char c)
	{
		return (c == ' ' || c == '\n');
	}

    /*
        Following chars are escaped
        \"
        \\
        \/
        \b
        \f
        \n
        \r
        \t
        \u four-hex-digits
    */

	// returns current char and moves cursor to next position
    char ReadChar()
	{
		if(_pszCur < _pszEnd)
		{
			char c = *_pszCur;
			_pszCur++;
			return c;
		}
		else
		{
			return '\0';
		}
    }
    char ReadUnescape()
	{
        char c = ReadChar();
        switch(c)
        {
            case '"': 
            case '\\': return c;
            case 'b': return '\b';
            case 'f': return '\f';
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            default:
			    assert(false);
                return '\0';
        }
	}

	// returns current char without reading it
	char PeekChar()
	{
		if(_pszCur < _pszEnd)
		{
			char c = *_pszCur;
			return c;
		}
		else
		{
			return '\0';
		}
	}
	void InitState(const char * pszBlob, size_t cchBlob)
	{
		_ReachedEnd = false;
		_pszBegin = pszBlob;
		_pszEnd = pszBlob + cchBlob;
		_pszCur = _pszBegin;

		// json starts with object or array
		_State = ParseState::StartReadValue;
		_Name.resize(0);
		_Value.resize(0);
	}

	enum class ParseState
	{
		StartReadName,
		ReadName,
		ReadColon,
		StartReadValue,
		ReadStringValue,
		ReadIntValue,
		EndReadValue,
		StartReadObject,
		StartReadArray,
	};
	ParseState _State;

	struct ParseNode
	{
		ParseNode(const std::string & name, NodeType type)
			: Name(name)
			, Type(type)
		{
		}

		std::string Name;
		NodeType Type;
	};

	// stack of nodes 
	std::stack<ParseNode> _ParseStack;
	const char * _pszBegin;
	const char * _pszCur;
	const char * _pszEnd;

    NodeType _Type;
	std::string _Name;
	std::string _Value;
    __int64 _IntValue;
	bool _ReachedEnd;
	bool _Skip;
};
