/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Parser.h"
#include "Variant.h"
#include <sstream>

using namespace std;

namespace PARSER
{
  struct op_map_t { OPERATOR_TYPE op; std::string name; };
  static op_map_t op_map[] = {{ NONE,    "" },
    { PERIOD,   "." },
    { COMMA,    "," },
    { LBRACKET, "(" },
    { RBRACKET, "(" },
    { EQUALS,   "==" },
    { NOTEQUALS,"!=" },
    { NOT,      "!" },
    { AND,      "&" },
    { OR,       "|" },
    { PLUS,     "+" },
    { MINUS,    "-" },
    { TIMES,    "*" },
    { DIVIDE,   "/" }};

  static std::string Operator(OPERATOR_TYPE op)
  {
    for (size_t i = 0; i < sizeof(op_map) / sizeof(op_map_t); i++)
      if (op == op_map[i].op)
        return op_map[i].name;
    return "";
  }
  
  static OPERATOR_TYPE Operator(const std::string &name)
  {
    for (size_t i = 0; i < sizeof(op_map) / sizeof(op_map_t); i++)
      if (name == op_map[i].name)
        return op_map[i].op;
    return NONE;
  }
  
  std::string Number::ToString() const
  {
    std::ostringstream str;
    str << value;
    return str.str();
  }

  CVariant BinaryOperation::Evaluate(Class *context) const
  {
    CVariant l = left->Evaluate(context);
    CVariant r = right->Evaluate(context);
    switch(type)
    {
      case TIMES:
        return l.asDouble() * r.asDouble();
      case DIVIDE:
        return l.asDouble() / r.asDouble();
      case MINUS:
        return l.asDouble() - r.asDouble();
      case PLUS:
      {
        if (l.isDouble())
          return l.asDouble() + r.asDouble();
        else if (l.isString())
          return l.asString() + r.asString();
        else if (l.isBoolean())
          return l.asBoolean() && r.asBoolean();
        break;
      }
      case EQUALS:
        return l == r;
      case NOTEQUALS:
        return !(l == r);
      case OR:
        return l.asBoolean() || r.asBoolean();
      case AND:
        return l.asBoolean() && r.asBoolean();
    }
    throw "Unknown type for operator";
  };

  const char *BinaryOperation::Type() const
  {
    switch(type)
    {
      case TIMES:
      case DIVIDE:
      case MINUS:
        return "number";
      case PLUS:
        return left->Type();
      case EQUALS:
      case NOTEQUALS:
      case OR:
      case AND:
        return "boolean";
    }
    return "";
  }

  CVariant UnaryOperation::Evaluate(Class *context) const
  {
    CVariant r = right->Evaluate(context);
    switch(type)
    {
      case NOT:
        return !r.asBoolean();
      case MINUS:
        return -r.asDouble();
    }
    throw "Unknown operator";
  }

  const char *UnaryOperation::Type() const
  {
    if (type == NOT || type == MINUS)
      return right->Type();
    return "";
  }

  CVariant Function::Evaluate(Class *context) const
  {
    throw "Function::Evaluate not implemented"; 
  }

  std::string Function::ToString() const
  {
    std::string ret = name + "(";
    for (std::vector<Atom *>::const_iterator i = params.begin(); i != params.end(); ++i)
    {
      if (i != params.begin())
        ret += ", ";
      ret += (*i)->ToString();
    }
    return ret + ")";
  }

  Function *Function::Create(const std::string &name, const std::vector<Atom*> &params) const
  {
    // TODO: Add something better here to indicate return codes
    Params p;
    for (vector<Atom*>::const_iterator i = params.begin(); i != params.end(); ++i)
      p.push_back((*i)->Type());

    if (func)
    {
      ClassFunction *new_func = func->Create(name, p);
      if (new_func)
        return new Function(name, params, new_func);
    }
    throw "Unimplemented function " + name;
  }

  const char *Function::Type() const
  {
    return "";
  }

  Class *Function::Call(Class *context)
  {
    Params p; p.reserve(params.size());
    for (vector<Atom*>::const_iterator i = params.begin(); i != params.end(); ++i)
      p.push_back((*i)->Evaluate(context));

    delete ans; ans = NULL;
    if (func)
      ans = func->Call(context, p);
    return ans;
  }

  CVariant FunctionCall::Evaluate(Class *context) const
  {
    Class *cls = context;
    for (vector<Function *>::const_iterator i = functions.begin(); i != functions.end(); ++i)
    {
      cls = (*i)->Call(cls);
      if (!cls)
        throw "Illegal function call";
    }
    return cls->Value();
  }

  std::string FunctionCall::ToString() const
  {
    std::string ret;
    for (vector<Function *>::const_iterator i = functions.begin(); i != functions.end(); ++i)
    {
      if (i != functions.begin())
        ret += ".";
      ret += (*i)->ToString();
    }
    return ret;
  }

  const char *FunctionCall::Type() const
  {
    //    functions.back()
    return "???"; // TODO: Define the returntype of a function - it'll be based on the last class, right?
  }

  Atom *Parser::Parse(TokenStream &ts)
  {
    Atom *ret = ParseExpression(NULL, ts);
    if (ts.Peek().type != Token::END_OF_FILE)
      throw "Expected end of input";
    return ret;
  }

  Atom *Parser::ParseFunctionCall(Atom *in, TokenStream &ts, Function *parent)
  {
    vector<Function*> functions;
    Function *func = ParseFunction(in, ts, parent);
    functions.push_back(func);
    while (ts.Peek().type == Token::OPERATOR && ts.Peek().op == PERIOD)
    {
      ts.Next(); // ignore '.'
      func = ParseFunction(in, ts, func);
      functions.push_back(func);
    }
    return new FunctionCall(functions);
  }

  Function *Parser::ParseFunction(Atom *in, TokenStream &ts, Function *parent)
  {
    if (ts.Peek().type == Token::LABEL)
    {
      std::string name = ts.Get().sval;
      vector<Atom *> params;
      if (ts.Peek().type == Token::OPERATOR && ts.Peek().op == LBRACKET)
      {
        ts.Next(); // ignore '('
        while (ts.Peek().type != Token::OPERATOR || ts.Peek().op != RBRACKET)
        {
          params.push_back(ParseExpression(in, ts));
          // verify and skip the comma
          if (ts.Peek().type != Token::OPERATOR || ts.Peek().op != COMMA)
            break;
          ts.Next();
        }
        if (ts.Peek().type != Token::OPERATOR || ts.Get().op != RBRACKET)
          throw "Expected ')'";
      }
      if (parent)
        return parent->Create(name, params);
      return new Function(name, params);
    }
    throw "Expected function name";
  }

  Atom *Parser::ParseExpression(Atom *in, TokenStream &ts)
  {
    Atom *left = ParseAndExpression(in, ts);
    while (ts.Peek().type == Token::OPERATOR && ts.Peek().op == OR)
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseAndExpression(left, ts);
      left = new BinaryOperation(kind, left, right);
    }
    return left;
  }

  Atom *Parser::ParseAndExpression(Atom *in, TokenStream &ts)
  {
    Atom *left = ParseEqualityExpression(in, ts);
    while (ts.Peek().type == Token::OPERATOR && ts.Peek().op == AND)
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseEqualityExpression(left, ts);
      left = new BinaryOperation(kind, left, right);
    }
    return left;
  }

  Atom *Parser::ParseEqualityExpression(Atom *in, TokenStream &ts)
  {
    Atom *left = ParseArithExpression(in, ts);
    while (ts.Peek().type == Token::OPERATOR && (ts.Peek().op == EQUALS || ts.Peek().op == NOTEQUALS))
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseArithExpression(left, ts);
      left = new BinaryOperation(kind, left, right);
    }
    return left;
  }
  
  Atom *Parser::ParseArithExpression(Atom *in, TokenStream &ts)
  {
    Atom *left = ParseTerm(in, ts);
    while (ts.Peek().type == Token::OPERATOR && (ts.Peek().op == PLUS || ts.Peek().op == MINUS))
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseTerm(left, ts);
      left = new BinaryOperation(kind, left, right);
    }
    return left;
  }

  Atom *Parser::ParseTerm(Atom *in, TokenStream &ts)
  {
    Atom *left = ParseFactor(in, ts);
    while (ts.Peek().type == Token::OPERATOR && (ts.Peek().op == TIMES || ts.Peek().op == DIVIDE))
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseFactor(left, ts);
      left = new BinaryOperation(kind, left, right);
    }
    return left;
  }

  Atom *Parser::ParseFactor(Atom *in, TokenStream &ts)
  {
    if (ts.Peek().type == Token::OPERATOR && (ts.Peek().op == NOT || ts.Peek().op == MINUS))
    {
      OPERATOR_TYPE kind = ts.Get().op;
      Atom *right = ParseFactor(in, ts);
      return new UnaryOperation(kind, right);
    }
    return ParseAtom(in, ts);
  }

  Atom *Parser::ParseAtom(Atom *in, TokenStream &ts)
  {
    Atom *ret = NULL;
    if (ts.Peek().type == Token::OPERATOR && ts.Peek().op == LBRACKET)
    {
      ts.Next(); // skip '('
      ret = ParseExpression(in, ts);
      if (ts.Peek().type == Token::OPERATOR && ts.Get().op != RBRACKET)
        throw "Expected ')'";
    }
    else if (ts.Peek().type == Token::STRING)
      ret = new String(ts.Get().sval);
    else if (ts.Peek().type == Token::NUMBER)
      ret = new Number(ts.Get().dval);
    else if (ts.Peek().type == Token::BOOLEAN)
      ret = new Bool(ts.Get().dval == 1);
    else
      ret = ParseFunctionCall(in, ts, global);
    return ret;
  }
}
