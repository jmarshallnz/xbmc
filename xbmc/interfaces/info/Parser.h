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

#pragma once

#include <string>
#include <vector>
#include "utils/Variant.h"

namespace PARSER
{
  enum OPERATOR_TYPE { NONE, PERIOD, COMMA, LBRACKET, RBRACKET, EQUALS, NOTEQUALS, NOT, AND, OR, PLUS, MINUS, TIMES, DIVIDE };

  static std::string Operator(OPERATOR_TYPE op);
  static OPERATOR_TYPE Operator(const std::string &o);

  class Token
  {
  public:
    enum TOKEN_TYPE { END_OF_FILE = 0, OPERATOR, STRING, NUMBER, BOOLEAN, LABEL };
    Token(OPERATOR_TYPE o) : op(o), type(OPERATOR), sval(), dval(0) {}
    Token(bool v) : op(NONE), type(BOOLEAN), sval(), dval(v ? 1 : 0) {}
    Token(double v) : op(NONE), type(NUMBER), sval(), dval(v) {}
    Token(TOKEN_TYPE t, const std::string &s = "") : op(NONE), type(t), sval(s), dval(0) {}

    OPERATOR_TYPE op;
    TOKEN_TYPE type;
    std::string sval;
    double dval;
  };

  class TokenStream
  {
  public:
    virtual void Next()=0;
    virtual Token Peek() const=0;
    virtual Token Get()=0;
  };

  class Class;
  typedef std::vector<CVariant> Params;

  /*! \brief Interface for member functions of Classes.
   Contains functions to Call the member function and also a function to create
   a subfunction of the class this function returns.
   */
  class ClassFunction
  {
  public:
    virtual ClassFunction *Create(const std::string &name, const Params &params) const=0;
    virtual Class *Call(const Class *cls, const Params &params) const=0;
  };

  /*!\ brief Template class (functor) wrapping member functions
   Allows member functions to be called on given class instances as well
   as creation subfunctions of the returned classes.
   */
  template <class T, class S> class MemberFunction : public ClassFunction
  {
    Class *(T::*func)(const Params &params) const;
  public:
    MemberFunction() : func(NULL) {};
    MemberFunction(Class *(T::*f)(const Params &params) const) : func(f) {}

    virtual ClassFunction *Create(const std::string &name, const Params &params) const
    {
      return S::Create(name, params);
    }

    virtual Class *Call(const Class *instance, const Params &params) const
    {
      if (func)
        return (*((T*)instance).*func)(params);
      return NULL;
    }
  };

  /* base class wrapper for the return value from functions */
  class Class
  {
  public:
    static ClassFunction *Create(const std::string &name, const Params &params) { return NULL; }
    virtual CVariant Value() const { return CVariant(); }
  };

  class Atom
  {
  public:
    virtual std::string ToString() const=0;
    virtual CVariant Evaluate(Class *context) const=0;
    virtual const char *Type() const=0;
  };

  class String : public Atom
  {
    std::string value;
  public:
    String(std::string v) : value(v) {}

    virtual CVariant Evaluate(Class *context) const { return value; };
    virtual std::string ToString() const { return '"' + value + '"'; }
    virtual const char *Type() const { return "string"; }
  };

  class Number : public Atom
  {
    double value;
  public:
    Number(double v) : value(v) {}

    virtual CVariant Evaluate(Class *context) const { return value; };
    virtual std::string ToString() const;
    virtual const char *Type() const { return "number"; }
  };

  class Bool : public Atom
  {
    bool value;
  public:
    Bool(bool v) : value(v) {}

    virtual CVariant Evaluate(Class *context) const { return value; };
    virtual std::string ToString() const { return value ? "true" : "false"; }
    virtual const char *Type() const { return "bool"; }
  };

  class BinaryOperation: public Atom
  {
    OPERATOR_TYPE type;
    Atom *left, *right;
  public:
    BinaryOperation(OPERATOR_TYPE t, Atom *l, Atom *r) : type(t), left(l), right(r) {}
    virtual ~BinaryOperation()
    {
      delete left;
      delete right;
    }
    virtual CVariant Evaluate(Class *context) const;
    virtual std::string ToString() const { return '(' + left->ToString() + ')' + Operator(type) + '(' + right->ToString() + ')'; }
    virtual const char *Type() const;
  };

  class UnaryOperation : public Atom
  {
    OPERATOR_TYPE type;
    Atom *right;
  public:
    UnaryOperation(OPERATOR_TYPE t, Atom *r) : type(t), right(r) {}
    virtual ~UnaryOperation()
    {
      delete right;
    }
    virtual CVariant Evaluate(Class *context) const;
    virtual std::string ToString() const
    {
      std::string ret;
      ret = type;
      return ret + '(' + right->ToString() + ')';
    }
    virtual const char *Type() const;
  };

  class Function : public Atom
  {
    std::string name;
    std::vector<Atom *> params;
    ClassFunction *func;
    Class *ans;  ///< result caching
  public:
    Function(const std::string &n, const std::vector<Atom*> &p) : name(n), params(p), func(NULL), ans(NULL) {}
    Function(const std::string &n, const std::vector<Atom*> &p, ClassFunction *f) : name(n), params(p), func(f), ans(NULL) {}
    virtual ~Function()
    {
      for (std::vector<Atom *>::iterator i = params.begin(); i != params.end(); ++i)
        delete (*i);
      delete func;
      delete ans;
    }
    Function *Create(const std::string &name, const std::vector<Atom*> &params) const;
    Class *Call(Class *context);

    virtual CVariant Evaluate(Class *context) const;
    virtual std::string ToString() const;
    virtual const char *Type() const;
  };

  /*! \brief A FunctionCall is of the form
     <class>.<function>.<function>...

   It returns a Class when evaluated.
   */
  class FunctionCall : public Atom
  {
    std::vector<Function *> functions;
  public:
    FunctionCall(const std::vector<Function *> f) : functions(f) {}
    virtual ~FunctionCall()
    {
      for (std::vector<Function *>::iterator i = functions.begin(); i != functions.end(); ++i)
        delete (*i);
    }

    virtual CVariant Evaluate(Class *context) const;
    virtual std::string ToString() const;
    virtual const char *Type() const;
  };

  class Parser
  {
  public:
    Parser(Function *g = NULL) : global(g) {}
    Atom *Parse(TokenStream &ts);

  private:
    Function *ParseFunction(Atom *in, TokenStream &ts, Function *parent);
    Atom *ParseFunctionCall(Atom *in, TokenStream &ts, Function *parent);
    Atom *ParseExpression(Atom *in, TokenStream &ts);
    Atom *ParseAndExpression(Atom *in, TokenStream &ts);
    Atom *ParseEqualityExpression(Atom *in, TokenStream &ts);
    Atom *ParseArithExpression(Atom *in, TokenStream &ts);
    Atom *ParseTerm(Atom *in, TokenStream &ts);
    Atom *ParseFactor(Atom *in, TokenStream &ts);
    Atom *ParseAtom(Atom *in, TokenStream &ts);

    Function *global;
  };
  
};

void TestFuncCalling();
