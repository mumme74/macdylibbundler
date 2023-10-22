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

#ifndef JSON_H
#define JSON_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>

/*
Valid Json types
string.
number.
object (JSON object)
array.
boolean.
null.
*/

namespace json {

class VluBase;
class Undefined;
class Null;
class Bool;
class Number;
class String;
class Array;
class Object;

typedef std::unique_ptr<VluBase> VluType;
typedef std::vector<VluType> ArrType;
typedef std::map<const std::string, json::VluType> ObjType;

class VluBase {
public:
  enum Type {
    // scalar types
    UndefinedType, NullType, BoolType, NumberType,
    // complex types, data on heap for these
    StringType, ArrayType, ObjectType
  };
protected:
  Type m_type;
  union vlu {
  public:
    vlu(Type type) :
      objVlu(nullptr)
    {
      switch (type) {
      case BoolType:   boolVlu = false; break;
      case NumberType: numVlu = 0.0f; break;
      case StringType: [[fallthrough]];
      case ArrayType:  [[fallthrough]];
      case ObjectType: [[fallthrough]];
      default: ; // init as nullptr
      }
    }
    ~vlu() {}
    bool boolVlu;
    float numVlu;
    std::unique_ptr<std::string> strVlu;
    std::unique_ptr<ArrType> arrVlu;
    std::unique_ptr<ObjType> objVlu;
  } m_vlu;
  const VluBase* m_parent;

  void shallowCopy(VluBase* to, const VluBase& other) const;
  std::unique_ptr<VluBase> copyCreate(const VluBase& vlu) const;
  bool isChildOf(const VluBase* other) const;

public:
  explicit VluBase(Type type, const VluBase* parent);
  VluBase(const VluBase& other);
  virtual ~VluBase();
  virtual VluBase& operator= (const VluBase& other);
  virtual bool operator== (const VluBase& other) const;
  virtual bool operator!= (const VluBase& other) const {
    return !operator==(other);
  }
  virtual std::string toString() const;
  virtual std::stringstream serialize(int indent = 0, int depth = 0) const;
  const VluBase* parent() const { return m_parent; }
  void setParent(const VluBase* parent) { m_parent = parent; }

  bool isUndefined() const { return m_type == UndefinedType; }
  bool isNull() const { return m_type == NullType; }
  bool isBool() const { return m_type == BoolType; }
  bool isNumber() const { return m_type == NumberType; }
  bool isString() const { return m_type == StringType; }
  bool isArray() const { return m_type == ArrayType; }
  bool isObject() const { return m_type == ObjectType; }
  Type type() const { return m_type; }

  VluBase*   asBase() const;
  Undefined* asUndefined() const;
  Null*      asNull() const;
  Bool*      asBool() const;
  Number*    asNumber() const;
  String*    asString() const;
  Array*     asArray() const;
  Object*    asObject() const;
};

class Undefined : public VluBase {
public:
  Undefined(const VluBase* parent = nullptr);
  Undefined(const Undefined& other);
  ~Undefined();
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
};

class Null : public VluBase {
public:
  Null(const VluBase* parent = nullptr);
  Null(const Null& other);
  ~Null();
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
};

class Bool : public VluBase {
public:
  Bool(bool vlu, const VluBase* parent = nullptr);
  Bool(const Bool& other);
  ~Bool();
  Bool& operator= (const Bool& other);
  bool vlu() const;
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
};


class Number : public VluBase {
public:
  Number(float vlu, const VluBase* parent = nullptr);
  Number(int vlu, const VluBase* parent = nullptr);
  Number(uint vlu, const VluBase* parent = nullptr);
  Number(const Number& other);
  ~Number();
  Number& operator= (const Number& other);
  bool operator< (const Number& other) const {
    return m_vlu.numVlu < other.m_vlu.numVlu;
  }
  bool operator<= (const Number& other) const {
    return m_vlu.numVlu <= other.m_vlu.numVlu;
  }
  float vlu() const;
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
};


class String : public VluBase {
public:
  String(const std::string& vlu, const VluBase* parent = nullptr);
  String(const String& other);
  ~String();
  String& operator= (const String& other);
  std::string vlu() const;
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
};

class Array : public VluBase {
public:
  Array(const VluBase* parent = nullptr);
  Array(const Array& other);
  Array(const std::initializer_list<VluBase>& args);
  ~Array();
  Array& operator= (const Array& other);
  VluBase& operator[] (size_t idx) const { return *at(idx); }
  VluBase* at(size_t idx) const;
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
  void push(std::unique_ptr<VluBase> vlu);
  void push(const VluBase& vlu) { push(copyCreate(vlu)); }
  VluType pop();
  size_t length() const;
};

class Object : public VluBase {
public:
  Object(const VluBase* parent = nullptr);
  Object(const Object& other);
  Object(const std::initializer_list<
    std::pair<std::string, VluBase>>& args);
  ~Object();
  Object& operator= (const Object& other);
  std::string toString() const;
  std::stringstream serialize(int indent = 0, int depth = 0) const;
  void set(const char* key, std::unique_ptr<VluBase> vlu);
  void set(const String& key, std::unique_ptr<VluBase> vlu) {
    set(key.vlu().c_str(), std::move(vlu));
  }
  void set(const String& key, const VluBase& vlu) {
    set(key, copyCreate(vlu));
  }
  void set(const char* key, const VluBase& vlu) {
    set(key, copyCreate(vlu));
  }
  VluBase* get(const char* key) const;
  VluBase* get(const String& key) const { return get(key.vlu().c_str()); }
  VluBase& operator[] (const String& key) const { return *get(key); }
  VluBase& operator[] (const char* key) const { return *get(key); }
  bool contains(const char* key) const;
  bool contains(const String& key) const {
     return contains(key.vlu().c_str());
  }
  VluType remove(const char* key);
  VluType remove(const String& key) { return remove(key.vlu().c_str()); }
  std::vector<std::string> keys() const;
  std::vector<VluBase*> values() const;
  size_t length() const { return m_vlu.objVlu->size(); }
};

VluType parse(const std::string& jsnStr);
VluType parse(std::stringstream& jsn);

}; // namespace json

#endif // JSON_H
