/*
The MIT License (MIT)

Copyright (c) 2023 Fredrik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

// this lib should not depend on anything in any other lib
// except libc++
#include <assert.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "Json.h"

using namespace Json;

std::stringstream
stringify(const std::string& in) {
  std::stringstream out; out << "\"";
  bool esc = false;
  const char *cur = in.c_str(),
              *end = in.c_str() + in.size();
  for (; cur != end; cur++) {
      if (esc) {
        out << *cur;
        esc = false;
        continue;
      }

      switch (*cur) {
      case '\\':
          esc = true;
          out << "\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '"': out << "\\\""; break;
      case '\r': out << "\\r"; break;
      case '\n': out << "\\n"; break;
      case '\t': out << "\\t"; break;
      default:
          out << *cur;
          esc = false;
      }
  }
  out << "\"";
  return out;
};

std::string
createIndent(int indent, int depth)
{
  int padNr = indent * depth;
  if (padNr < 1) return "";

  std::string str("\n");
  str.append(padNr, ' ');
  return str;
}

std::stringstream
serializeScalar(int indent, int depth, const std::string& toStr)
{
  std::stringstream out;
  out << createIndent(indent, depth) << toStr;
  return out;
}

/// returns the utf8 length of this string
std::size_t
utf8_strnlen(std::string_view str) {
  std::size_t len = 0;
  int ch = str[0];
  for (std::size_t i = 0; ch != '\0' && i < str.size(); ++i, ++len) {
    auto ch = str[i];
    if ((ch & 0xF0) == 0xF0) { len += 3; i += 3; }
    else if ((ch & 0xD0) == 0xD0) { len += 2; i += 2; }
    else if ((ch & 0xC0) == 0xC0) { len += 1; i += 1; }
  }
  return len;
}

// --------------------------------------------------------------

Exception::Exception(std::string what) :
  std::runtime_error(what)
{}

ParseException::ParseException(std::string what):
  Exception(what)
{}

// ---------------------------------------------------------------

void
VluBase::shallowMove(VluBase *to, VluBase&& rhs) const
{
  switch (m_type) {
  case BoolType:
    to->m_vlu.boolVlu = std::move(rhs.m_vlu.boolVlu);
    break;
  case NumberType:
    to->m_vlu.numVlu = std::move(rhs.m_vlu.numVlu);
    break;
  case StringType: {
    StrType ptr = std::move(rhs.m_vlu.strVlu);
    to->m_vlu.strVlu.swap(ptr);
  } break;
  case ArrayType: {
    auto ptr = std::move(rhs.m_vlu.arrVlu);
    to->m_vlu.arrVlu.swap(ptr);
    for (const auto& entry : *to->m_vlu.arrVlu) {
      entry->setParent(to);
    }
  } break;
  case ObjectType: {
    auto ptr = std::move(rhs.m_vlu.objVlu);
    to->m_vlu.objVlu.swap(ptr);
    for (const auto& entry : *to->m_vlu.objVlu) {
      entry.second->setParent(to);
    }
  } break;
  default: ; // nothing
  }
}

void
VluBase::shallowCopy(VluBase *to, const VluBase& other) const
{
  switch (m_type) {
  case BoolType:   to->m_vlu.boolVlu = other.m_vlu.boolVlu; break;
  case NumberType: to->m_vlu.numVlu = other.m_vlu.numVlu; break;
  case StringType: {
    auto ptr = std::make_unique<std::string>(*other.m_vlu.strVlu);
    to->m_vlu.strVlu.swap(ptr);
  } break;
  case ArrayType:
    if (!to->m_vlu.arrVlu) to->m_vlu.arrVlu = std::make_unique<ArrType>();
    else to->m_vlu.arrVlu->clear();

    to->m_vlu.arrVlu->reserve(other.m_vlu.arrVlu->size());
    for (const auto& entry : *other.m_vlu.arrVlu) {
      VluType subItm = copyCreate(*entry);
      subItm->setParent(to);
      to->m_vlu.arrVlu->push_back(std::move(subItm));
    }
    break;
  case ObjectType:
    if (!to->m_vlu.objVlu) to->m_vlu.objVlu = std::make_unique<ObjType>();
    else to->m_vlu.objVlu->clear();

    for (const auto& entry : *other.m_vlu.objVlu) {
      VluType subItm = copyCreate(*entry.second);
      subItm->setParent(to);
      to->m_vlu.objVlu->insert_or_assign(
        entry.first, std::move(subItm));
    }
    break;
  default: ; // nothing
  }
}

VluType
VluBase::copyCreate(const VluBase& vlu) const
{
  switch (vlu.type()) {
  case UndefinedType:
    return std::make_unique<Undefined>(vlu.m_parent);
  case NullType:
    return std::make_unique<Null>(vlu.m_parent);
  case BoolType:
    return std::make_unique<Bool>(vlu.m_vlu.boolVlu, vlu.m_parent);
  case NumberType:
    return std::make_unique<Number>(vlu.m_vlu.numVlu, vlu.m_parent);
  case StringType:
    return std::make_unique<String>(*vlu.m_vlu.strVlu, vlu.m_parent);
  case ArrayType: {
    auto itm = std::make_unique<Array>(vlu.m_parent);
    itm->m_vlu.arrVlu->reserve(vlu.m_vlu.arrVlu->size());
    for (const auto& subVlu : *vlu.m_vlu.arrVlu) {
      auto subItm = copyCreate(*subVlu);
      subItm->setParent(itm.get());
      itm->m_vlu.arrVlu->push_back(std::move(subItm));
    }
    return itm;
  }
  case ObjectType: {
    auto itm = std::make_unique<Object>(vlu.m_parent);

    for (const auto& entry : *vlu.m_vlu.objVlu) {
      VluType subItem = copyCreate(*entry.second);
      subItem->setParent(itm.get());
      itm->m_vlu.objVlu->insert(std::pair<std::string, VluType>(
        entry.first, std::move(subItem)
      ));
    }
    return itm;
  }
  default:
    return std::make_unique<VluBase>(vlu);
  }
}

bool
VluBase::isChildOf(const VluBase* other) const
{
  for (const VluBase* vlu = this; vlu != nullptr; vlu = vlu->m_parent) {
    if (vlu == other) return true;
  }

  return false;
}

VluBase::VluBase(Type type, const VluBase* parent) :
  m_type(type),
  m_vlu(type),
  m_parent(parent)
{}

VluBase::VluBase(const VluBase& other) :
  m_type(other.m_type),
  m_vlu(other.m_type),
  m_parent(other.m_parent)
{
  shallowCopy(this, other);
}

VluBase::VluBase(VluBase&& rhs) :
  m_type(std::move(rhs.m_type)),
  m_vlu(std::move(rhs.m_type)),
  m_parent(std::move(rhs.m_parent))
{
  shallowMove(this, std::move(rhs));
}

VluBase::~VluBase()
{}

VluBase&
VluBase::operator= (const VluBase& other)
{
  assert(m_type == other.m_type);
  shallowCopy(this, other);
  return *this;
}

VluBase&
VluBase::operator= (VluBase&& rhs)
{
  assert(m_type == rhs.m_type);
  shallowMove(this, std::move(rhs));
  return *this;
}

bool
VluBase::operator== (const VluBase &other) const
{
  if (m_type != other.m_type)
    return false;
  switch (m_type) {
  case UndefinedType: return false;
  case NullType: return true;
  case BoolType: return m_vlu.boolVlu == other.m_vlu.boolVlu;
  case NumberType: return m_vlu.numVlu == other.m_vlu.numVlu;
  case StringType: return *m_vlu.strVlu == *other.m_vlu.strVlu;
  case ArrayType:
    if (m_vlu.arrVlu->size() != other.m_vlu.arrVlu->size())
      return false;
    for (auto l = m_vlu.arrVlu->begin(), r = other.m_vlu.arrVlu->begin();
         l != m_vlu.arrVlu->end(); ++l, ++r)
    {
      if (**l != **r) return false;
    }
    return true;
  case ObjectType:
    if (m_vlu.objVlu->size() != other.m_vlu.objVlu->size())
      return false;
    for (auto l = m_vlu.objVlu->begin(), r = other.m_vlu.objVlu->begin();
        l != m_vlu.objVlu->end(); ++l, ++r)
    {
      if (l->first != r->first) return false;
      if (*l->second != *r->second) return false;
    }
    return true;
  default: assert(m_type > -1 && "unhandled type");
  }
  return false;
}

std::string
VluBase::toString() const
{
  switch (m_type) {
  case UndefinedType: return asUndefined()->toString();
  case NullType: return asNull()->toString();
  case BoolType: return asBool()->toString();
  case NumberType: return asNumber()->toString();
  case StringType: return asString()->toString();
  case ArrayType: return asArray()->toString();
  case ObjectType: return asObject()->toString();
  default: assert(m_type > -1 && "unhandled type");
  }
  return "";
}

std::stringstream
VluBase::serialize(int indent, int depth) const
{
  switch (m_type) {
  case UndefinedType: throw Exception("Can't serialize as a value is undefined");
  case NullType: return asNull()->serialize(indent, depth);
  case BoolType: return asBool()->serialize(indent, depth);
  case NumberType: return asNumber()->serialize(indent, depth);
  case StringType: return asString()->serialize(indent, depth);
  case ArrayType: return asArray()->serialize(indent, depth);
  case ObjectType: return asObject()->serialize(indent, depth);
  default: assert(m_type > -1 && "unhandled type");
  }
  return std::stringstream();
}

std::string_view
VluBase::typeName() const
{
  switch (m_type) {
  case UndefinedType: return "undefined";
  case NullType:      return "null";
  case BoolType:      return "boolean";
  case NumberType:    return "number";
  case StringType:    return "string";
  case ArrayType:     return "array";
  case ObjectType:    return "object";
  default: assert(m_type > -1 && "unhandled type");
  }
  return "*error*";
}

VluBase*
VluBase::asBase() const
{
  return static_cast<VluBase*>(const_cast<VluBase*>(this));
}

Undefined*
VluBase::asUndefined() const
{
  if (m_type != UndefinedType) throw Exception("Can't convert to Undefined");
  return dynamic_cast<Undefined*>(const_cast<VluBase*>(this));
}

Null*
VluBase::asNull() const
{
  if (m_type != NullType) throw Exception("Can't convert to Null");
  return dynamic_cast<Null*>(const_cast<VluBase*>(this));
}

Bool*
VluBase::asBool() const
{
  if (m_type != BoolType) throw Exception("Can't convert to Bool");
  return dynamic_cast<Bool*>(const_cast<VluBase*>(this));
}

Number*
VluBase::asNumber() const
{
  if (m_type != NumberType) throw Exception("Can't convert to Number");
  return dynamic_cast<Number*>(const_cast<VluBase*>(this));
}

String*
VluBase::asString() const
{
  if (m_type != StringType) throw Exception("Can't convert to String");
  return dynamic_cast<Json::String*>(const_cast<VluBase*>(this));
}

Array*
VluBase::asArray() const
{
  if (m_type != ArrayType) throw Exception("Can't convert to Array");
  return dynamic_cast<Json::Array*>(const_cast<VluBase*>(this));
}

Object*
VluBase::asObject() const
{
  if (m_type != ObjectType) throw Exception("Can't convert to Object");
  return dynamic_cast<Json::Object*>(const_cast<VluBase*>(this));
}

// -----------------------------------------------------------------------

Undefined::Undefined(const VluBase* parent) :
  VluBase(UndefinedType, parent)
{}

Undefined::Undefined(const Undefined& other) :
  VluBase(UndefinedType, other.m_parent)
{}

Undefined::~Undefined() {}

std::string
Undefined::toString() const
{
  return std::string("undefined");
}

std::stringstream
Undefined::serialize(int indent, int depth) const
{
  return serializeScalar(indent, depth, toString());
}

// -----------------------------------------------------------------------

Null::Null(const VluBase* parent) :
  VluBase(NullType, parent)
{
}

Null::Null(const Null& other) :
  VluBase(NullType, other.m_parent)
{ }

Null::~Null() { }

std::string
Null::toString() const
{
  return std::string("null");
}

std::stringstream
Null::serialize(int indent /* = 0*/, int depth /*= 0*/) const
{
  return serializeScalar(indent, depth, toString());
}

// -----------------------------------------------------------------

Bool::Bool(bool vlu, const VluBase* parent) :
  VluBase(BoolType, parent)
{
  m_vlu.boolVlu = vlu;
}

Bool::Bool(const Bool& other) :
  VluBase(other)
{
}

Bool::~Bool() {}

Bool&
Bool::operator= (const Bool& other)
{
  VluBase::operator=(other);
  return *this;
}

Bool&
Bool::operator= (Bool&& rhs)
{
  VluBase::operator=(std::move(rhs));
  return *this;
}

bool
Bool::vlu() const
{
  return m_vlu.boolVlu;
}

std::string
Bool::toString() const
{
  return m_vlu.boolVlu ? "true" : "false";
}

std::stringstream
Bool::serialize(int indent /* = 0 */, int depth /*= 0*/) const
{
  return serializeScalar(indent, depth, toString());
}

// -----------------------------------------------------------------

Number::Number(float vlu, const VluBase* parent) :
  VluBase(NumberType, parent)
{
  m_vlu.numVlu = vlu;
}

Number::Number(int vlu, const VluBase* parent) :
  VluBase(NumberType, parent)
{
  m_vlu.numVlu = static_cast<float>(vlu);
}

Number::Number(uint vlu, const VluBase* parent) :
  VluBase(NumberType, parent)
{
  m_vlu.numVlu = static_cast<float>(vlu);
}

Number::Number(const Number& other) :
  VluBase(other)
{}

Number::~Number() { }

Number&
Number::operator= (const Number& other)
{
  VluBase::operator=(other);
  return *this;
}

Number&
Number::operator= (Number&& rhs)
{
  VluBase::operator=(std::move(rhs));
  return *this;
}

float
Number::vlu() const
{
  return m_vlu.numVlu;
}

std::string
Number::toString() const
{
  int intVlu = static_cast<int>(m_vlu.numVlu);
  if (intVlu == m_vlu.numVlu)
    return std::to_string(intVlu);
  return std::to_string(m_vlu.numVlu);
}

std::stringstream
Number::serialize(int indent /* = 0*/, int depth /*= 0*/) const
{
  return serializeScalar(indent, depth, toString());
}

// -----------------------------------------------------------

String::String(const std::string& vlu, const VluBase* parent) :
  VluBase(StringType, parent)
{
  m_vlu.strVlu = std::make_unique<std::string>(vlu);
}


String::String(std::string_view vlu, const VluBase* parent) :
  VluBase(StringType, parent)
{
  m_vlu.strVlu = std::make_unique<std::string>(vlu.data());
}

String::String(const char* vlu, const VluBase* parent) :
  VluBase(StringType, parent)
{
  m_vlu.strVlu = std::make_unique<std::string>(std::string(vlu));
}

String::String(const String& other) :
  VluBase(other)
{}

String::~String()
{}

String&
String::operator= (const String& other)
{
  VluBase::operator=(other);
  return *this;
}

String&
String::operator= (String&& rhs)
{
  VluBase::operator=(std::move(rhs));
  return *this;
}

std::string
String::vlu() const
{
  return *m_vlu.strVlu;
}

std::string
String::toString() const
{
  return *m_vlu.strVlu;
}

std::stringstream
String::serialize(int indent /* = 0*/, int depth /*= 0*/) const
{
  std::stringstream out;
  out << createIndent(indent, depth) << stringify(*m_vlu.strVlu).str();
  return out;
}

// ---------------------------------------------------------

Array::Array(const VluBase* parent) :
  VluBase(ArrayType, parent)
{
  m_vlu.arrVlu = std::make_unique<ArrType>();
}

Array::Array(ArrInitializer args) :
  VluBase(ArrayType, nullptr)
{
  m_vlu.arrVlu = std::make_unique<ArrType>();
  m_vlu.arrVlu->reserve(args.size());
  for (const auto& entry : args) {
    auto itm = copyCreate(entry);
    itm->setParent(this);
    m_vlu.arrVlu->push_back(std::move(itm));
  }
}

Array::Array(const Array& other) :
  VluBase(other)
{}

Array::Array(const std::vector<std::string>& stringList):
  VluBase(ArrayType, nullptr)
{
  for (const auto& str : stringList)
    push(std::make_unique<String>(str));
}

Array::~Array()
{}

Array&
Array::operator= (const Array& other)
{
  if (!m_vlu.arrVlu->empty())
    throw Exception("Can't set to a non empty array");
  VluBase::operator=(other);
  return *this;
}

Array&
Array::operator= (Array&& rhs)
{
  if (!m_vlu.arrVlu->empty())
    throw Exception("Can't set to a non empty array");
  VluBase::operator=(std::move(rhs));
  return *this;
}

int
Array::indexOf(VluBase search) const
{
  int idx = -1;
  for (const auto& itm : *m_vlu.arrVlu) {
    ++idx;
    if (*itm == search)
      return idx;
  }
  return idx;
}

VluBase*
Array::at(size_t idx) const
{
  return (*m_vlu.arrVlu)[idx]->asBase();
}

std::string
Array::toString() const
{
  return "Array()";
}

std::stringstream
Array::serialize(int indent /* = 0 */, int depth /*= 0*/) const
{
  uint iter = 0;
  std::stringstream out;
  auto ind = createIndent(indent, depth);
  out << ind << "[";
  for (const auto& vlu : *m_vlu.arrVlu) {
    if (iter++) out << ",";
    out << vlu->serialize(indent, depth + 1).str();
  }
  out << ind << "]";

  return out;
}

void
Array::push(std::unique_ptr<VluBase> vlu)
{
  if (isChildOf(vlu.get())) throw Exception("Cyclic dependency");
  vlu->setParent(this);
  m_vlu.arrVlu->push_back(std::move(vlu));
}

void
Array::unshift(std::unique_ptr<VluBase> vlu)
{
  if (isChildOf(vlu.get())) throw Exception("Cyclic dependency");
  vlu->setParent(this);
  m_vlu.arrVlu->emplace(m_vlu.arrVlu->begin(), std::move(vlu));
}

VluType
Array::pop()
{
  if (m_vlu.arrVlu->empty()) return nullptr;
  auto vlu = std::move(m_vlu.arrVlu->back());
  m_vlu.arrVlu->pop_back();
  vlu->setParent(nullptr);
  return vlu;
}

VluType
Array::shift()
{
  if (m_vlu.arrVlu->empty()) return nullptr;
  auto vlu = std::move(m_vlu.arrVlu->front());
  m_vlu.arrVlu->erase(m_vlu.arrVlu->begin());
  vlu->setParent(nullptr);
  return vlu;
}

size_t
Array::length() const
{
  return m_vlu.arrVlu->size();
}

// ------------------------------------------------------------------

Object::Object(const VluBase* parent) :
  VluBase(ObjectType, parent)
{
  m_vlu.objVlu = std::make_unique<ObjType>();
}

Object::Object(ObjInitializer args) :
  VluBase(ObjectType, nullptr)
{
  m_vlu.objVlu = std::make_unique<ObjType>();
  for (const auto& entry : args) {
    auto itm = copyCreate(entry.second);
    itm->setParent(this);
    m_vlu.objVlu->insert(std::pair<std::string, VluType>(
      entry.first, std::move(itm)
    ));
  }
}

Object::Object(const Object& other) :
  VluBase(other)
{}

Object::~Object()
{}

Object&
Object::operator= (const Object& other)
{
  if (!m_vlu.objVlu->empty())
    throw Exception("Can't set to a non empty json::Object");
  VluBase::operator= (other);
  return *this;
}

Object&
Object::operator= (Object&& rhs)
{
  if (!m_vlu.objVlu->empty())
    throw Exception("Can't set to a non empty json::Object");
  VluBase::operator=(std::move(rhs));
  return *this;
}

std::string
Object::toString() const
{
  return "Object()";
}

std::stringstream
Object::serialize(int indent, int depth) const
{
  uint iter = 0;
  std::stringstream out;
  auto ind = createIndent(indent, depth);
  out << ind << "{";
  auto p = out.tellp();
  std::string str;
  for (const auto& entry : *m_vlu.objVlu) {
    if (iter++) out << ",";
    str = entry.second->serialize(indent, depth + 2).str();
    str = str.substr(str.find_first_not_of("\n"));
    out << createIndent(indent, depth+1)
        << stringify(entry.first).str() << ":"
        << str.substr(str.find_first_not_of(' '));
  }
  if (p < out.tellp()) {
    if (indent > 0 && ind.size() && ind[0] != '\n')
      out << '\n';
    out << ind;
  }

  out << "}";


  return out;
}

void
Object::set(const char* key, std::unique_ptr<VluBase> vlu)
{
  if (isChildOf(vlu.get())) throw Exception("Cyclic dependency");
  vlu->setParent(this);
  m_vlu.objVlu->insert_or_assign(std::string(key), std::move(vlu));
}

VluBase*
Object::get(const char *key) const
{
  auto iter = m_vlu.objVlu->find(key);
  if (iter != m_vlu.objVlu->end())
    return iter->second.get();
  std::stringstream msg;
  msg << "Key " << key << " not found in object!";
  throw Exception(msg.str());
}

bool
Object::contains(const char *key) const
{
  return m_vlu.objVlu->find(key) != m_vlu.objVlu->end();
}

VluType
Object::remove(const char* key)
{
  auto iter = m_vlu.objVlu->find(key);
  if (iter != m_vlu.objVlu->end()) {
    auto ptr = std::move(iter->second);
    m_vlu.objVlu->erase(iter);
    ptr->setParent(nullptr);
    return ptr;
  }
  std::stringstream msg;
  msg << "Key " << key << " not found in object!";
  throw Exception(msg.str());
}

std::vector<std::string>
Object::keys() const
{
  std::vector<std::string> k;
  k.reserve(m_vlu.objVlu->size());

  for(const auto& entry : *m_vlu.objVlu)
    k.push_back(entry.first);

  return k;
}

std::vector<VluBase*>
Object::values() const
{
  std::vector<VluBase*> v;
  v.reserve(m_vlu.objVlu->size());

  for(const auto& entry : *m_vlu.objVlu) {
    v.push_back(entry.second.get());
  }

  return v;
}

// --------------------------------------------------------------------


Parser::Parser() :
  m_src{}, m_pos{0}
{}

VluType
Parser::parse(std::string_view src)
{
  m_src = src.data();
  m_pos = 0;
  int ch;

  eatWhitespace();
  while ((ch = peek()) != -1) {
    switch (ch) {
    case '{':
      return parseObject();
    case '[':
      return parseArray();
    default:
      std::stringstream msg;
      msg << "Invalid in root " << (char)ch;
      throw exceptionAt(msg);
    }
  }
  return std::make_unique<Undefined>();
}

void
Parser::eatWhitespace() {
  while (!eof() && isspace(peek()))
    get();
}

void
Parser::expect(const char *needle) {
  eatWhitespace();
  const char* needlePtr = needle;
  int ch;
  while ((ch = peek(needlePtr - needle)) != -1 &&
          *needlePtr != '\0'
  ) {
    if (ch != *needlePtr) {
      std::stringstream msg;
      msg << "Expected '" << needle;
      throw exceptionAt(msg);
    }
    needlePtr++;
  }
  m_pos += needlePtr - needle;
  eatWhitespace();
}

VluType
Parser::parseNumber() {
  std::string baseBuf, exponBuf;
  int prev = 0, ch;
  bool hasDot = false;

  auto endOfNumber = [&](int ch) {
    return ch == ',' || ch == ']' || ch == '}';
  };
  auto finish = [&]()->VluType {
    float num = std::stof(baseBuf);
    if (exponBuf.size()) {
      int exp = std::stoi(exponBuf);
      num *= std::pow(10, exp);
    }
    return std::make_unique<Number>(num);
  };

  // parse base
  while ((ch = get()) != -1) {
    if (ch <= '9' && ch > '0')
      baseBuf += ch;
    else if (ch == '0' && (prev != 0 || peek() == '.'))
      baseBuf += ch;
    else if (ch == '.' && !hasDot) {
      baseBuf += ch;
      hasDot = true;
    } else if (ch == '-' && prev == 0)
      baseBuf = ch;
    else if (ch == 'e' || ch == 'E') {
      prev = 'e';
      break; // parse exponent hereafter
    } else if (endOfNumber(ch) || isspace(ch)) {
      unget();
      return finish();
    } else {
      std::stringstream msg;
      msg <<"Invalid ch: '" << (char)ch << "' in number.";
      throw exceptionAt(msg);
    }
    prev = ch;
  };

  // parse exponent
  while ((ch = get()) != -1) {
    if (ch > '0' && ch <= '9')
      exponBuf += ch;
    else if (ch == '0' && prev != 'e')
      exponBuf += ch;
    else if ((ch == '+' || ch == '-') && prev == 'e')
      exponBuf += ch;
    else if (endOfNumber(ch) || isspace(ch)) {
      unget();
      return finish();
    } else {
      std::stringstream msg;
      msg <<"Invalid ch: '" << (char)ch << "' in number exponent.";
      throw exceptionAt(msg);
    }
    prev = ch;
  }

  std::stringstream msg;
  msg << "Invalid ch: '" << (char)ch << "' in number.";
  throw exceptionAt(msg);
}

VluType
Parser::parseString() {
  std::string buf;
  int ch = get();
  if (ch != '"') throw "Not a string";

  while ((ch = get()) != -1) {
    switch (ch) {
    case '"': return std::make_unique<String>(buf);
    case '/': {
      std::stringstream msg;
      msg << "Char '" << ch << "' not allowed unescaped in string.";
      throw exceptionAt(msg);
    } break;
    case '\\':
      while (ch && (ch = get()) != -1) {
        switch (ch) {
        case '"': buf += '"'; ch = 0; break;
        case '\\': buf += '\\'; ch = 0; break;
        case '/': buf += '/'; ch = 0; break;
        case 'b': buf += '\b'; ch = 0; break;
        case 'f': buf += '\f'; ch = 0; break;
        case 'n': buf += '\n'; ch = 0; break;
        case 'r': buf += '\r'; ch = 0; break;
        case 't': buf += '\t'; ch = 0; break;
        case 'u': {
          // specialcase utf 8  2 bytes
          std::string hex{"0x"};
          buf += std::stoi(hex+m_src.substr(m_pos-1, 2));
          buf += std::stoi(hex+m_src.substr(m_pos+1, 2));
          m_pos += 3;
          ch = 0;
        } break;
        default:
          std::stringstream msg;
          msg << "Unrecognized escape sequence \\" << ch;
          throw exceptionAt(msg);
        }
      }
      break;
    default: buf += ch;
    }
  }
  std::stringstream msg;
  msg << "String not terminated";
  throw exceptionAt(msg);
}

VluType
Parser::parseValue() {
  int ch = peek();
  switch (ch) {
  case '[': return parseArray();
  case '{': return parseObject();
  case '"': return parseString();
  case 'n':
    expect("null");
    return std::make_unique<Null>();
  case 't':
    expect("true");
    return std::make_unique<Bool>(true);
  case 'f':
    expect("false");
    return std::make_unique<Bool>(false);
  case '-': case '+': case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7': case '8': case '9':
    return parseNumber();
  default: {
    std::stringstream msg;
    msg << "Unhandled ch: " << (char)ch;
    throw exceptionAt(msg);
  }
  }
}

VluType
Parser::parseArray() {
  auto root = std::make_unique<Array>();
  expect("[");
  while (!eof()) {
    eatWhitespace();
    root->push(parseValue());
    eatWhitespace();
    if (peek() == ']') {
      get(); break;
    }
    expect(",");
  }
  return root;
}

VluType
Parser::parseObject() {
  auto root = std::make_unique<Object>();
  int ch;
  expect("{");
  while ((ch = peek()) != -1) {
    eatWhitespace();
    auto key = parseString();
    expect(":");
    eatWhitespace();
    root->set(*key->asString(), parseValue());
    eatWhitespace();
    if (peek() == '}') {
      get(); break;
    }
    expect(",");
  }
  return root;
}


ParseException
Parser::exceptionAt(std::stringstream& msg)
{
  size_t pos = 0, prevPos = 0, lineNr = 0;
  std::string line;
  do {
    pos = m_src.find('\n', prevPos);
    line = m_src.substr(prevPos, pos);
    if (pos < m_src.size())
      prevPos = pos + 1;
    ++lineNr;
  } while(pos < m_pos);

  auto colNr = m_pos + 1 - (lineNr > 1 ? prevPos : 0);

  auto fromPos = m_pos > 30 ? m_pos -30 : 0;
   // make sure problemStr does not contain \n
  size_t p = fromPos, endPos = fromPos + 60;
  while ((p = m_src.find('\n', p)) != std::string::npos) {
    if (p < m_pos)
      fromPos = p+1;
    else {
      endPos = p;
      break;
    }
    ++p;
  }

  auto strEnd = m_src.substr(m_pos, endPos - m_pos);
  auto strStart = m_src.substr(fromPos, m_pos - fromPos);
  auto lenToPos = utf8_strnlen(strStart) +1;
  auto len = strStart.size();

  // make assume a tab is 4 spaces
  p = 0;
  while ((p = strStart.find('\t', p)) != std::string::npos) {
    lenToPos += 4;
    ++p;
  }

  msg << " Line " << lineNr << " col " << colNr
      << "\n" << strStart << strEnd << "\n"
      << std::setw(lenToPos) << std::setfill(' ')
      << "^\n";

  return msg.str();
}

VluType Json::parse(std::string_view jsnStr)
{
  Parser parser;
  return parser.parse(jsnStr);
}
