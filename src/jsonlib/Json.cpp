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

// --------------------------------------------------------------

Exception::Exception(const char* what) :
  std::runtime_error(what)
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
  throw Exception(msg.str().c_str());
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
  throw Exception(msg.str().c_str());
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
VluType parseArray(std::stringstream& jsn);
VluType parseObject(std::stringstream& jsn);

void eatWhitespace(std::stringstream& jsn) {
  while (!jsn.eof() && isspace(jsn.peek())) {
    jsn.get();
    continue;
  }
}

void expect(const char *needle, std::stringstream& jsn) {
  eatWhitespace(jsn);
  while (!jsn.eof() && *needle != '\0') {
    int ch = jsn.get();
    if (ch != *needle) {
      jsn.unget();
      std::stringstream msg;
      msg << "Expected '" << needle << "' near: "
          << jsn.str().substr(jsn.tellg(), 20);
      throw Exception(msg.str().c_str());
    }
    needle++;
  }
  eatWhitespace(jsn);
}

VluType parseNumber(std::stringstream& jsn) {
  std::string buf;
  int prev = 0;
  while (!jsn.eof()) {
    int ch = jsn.get();
    if ((ch == '+' || ch == '-') && prev == 0) prev = ch; // ignore throw
    else if ((ch == '+' || ch == '-') && (prev == 'e' || prev == 'E'))
      prev = ch; // ignore throw
    else if (ch == ',' || ch == ']' || ch == '}' || isspace(ch)) {
      jsn.unget();
      break; // done with
    }
    else if ((ch < '0' || ch > '9') && ch != 'e' &&
         ch != 'E' && ch != '-' && ch != '.')
    {
      std::stringstream msg;
      msg <<"Invalid ch:" << (char)ch << " in number.";
      throw Exception(msg.str().c_str());
    }
    buf += ch;
    prev = ch;
  }
  return std::make_unique<Number>(std::stof(buf));
}

VluType parseString(std::stringstream& jsn) {
  std::string buf;
  bool esc = false;
  int ch = jsn.get();
  if (ch != '"') throw "Not a string";
  while (!jsn.eof()) {
    ch = jsn.get();
    switch (ch) {
    case '"':
      if (esc) { esc = false; buf += ch; }
      else return std::make_unique<String>(buf);
      break;
    case '\\':
      if (!esc) esc = true;
      else buf += ch;
      break;
    default: buf += ch;
    }
  }
  throw "String not terminated";
}

VluType parseValue(std::stringstream& jsn) {
  int ch = jsn.peek();
  switch (ch) {
  case '[': return parseArray(jsn);
  case '{': return parseObject(jsn);
  case '"': return parseString(jsn);
  case 'n':
    expect("null", jsn);
    return std::make_unique<Null>();
  case 't':
    expect("true", jsn);
    return std::make_unique<Bool>(true);
  case 'f':
    expect("false", jsn);
    return std::make_unique<Bool>(false);
  case '-': case '+': case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7': case '8': case '9':
    return parseNumber(jsn);
  default: {
    std::stringstream msg;
    msg << "Unhandled ch: " << (char)ch;
    throw Exception(msg.str().c_str());
  }
  }
  return std::make_unique<Undefined>();
}

VluType parseArray(std::stringstream& jsn) {
  auto root = std::make_unique<Array>();
  uint iter = 0;
  expect("[", jsn);
  while (!jsn.eof()) {
    if (iter++) expect(",", jsn);
    else eatWhitespace(jsn);
    root->push(parseValue(jsn));
    if (jsn.peek() == ']') {
      jsn.get(); break;
    }
  }
  return root;
}

VluType parseObject(std::stringstream& jsn) {
  auto root = std::make_unique<Object>();
  uint iter = 0;
  expect("{", jsn);
  while (!jsn.eof()) {
    if (iter++) expect(",", jsn);
    else eatWhitespace(jsn);
    auto key = parseString(jsn);
    expect(":", jsn);
    root->set(*key->asString(), parseValue(jsn));
    if (jsn.peek() == '}') {
      jsn.get(); break;
    }
  }
  return root;
}

VluType Json::parse(const std::string& jsnStr) {
  std::stringstream sjson; sjson << jsnStr;
  return parse(sjson);
}

VluType Json::parse(std::stringstream& jsn) {
  eatWhitespace(jsn);
  while (!jsn.eof()) {
    int ch = jsn.peek();
    switch (ch) {
    case '{':
      return parseObject(jsn);
    case '[':
      return parseArray(jsn);
    default:
      std::stringstream msg;
      msg << "Invalid in root " << (char)ch;
      throw Exception(msg.str().c_str());
    }
  }
  return std::make_unique<Undefined>();
}
