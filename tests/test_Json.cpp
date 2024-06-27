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
#include "Json.h"
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

namespace Json {


TEST(UndefinedTest, asOther) {
  VluBase v(VluBase::UndefinedType, nullptr);
  EXPECT_NO_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(UndefinedTest, ConstructNoArgs) {
  Undefined n;
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&n), nullptr);
};
TEST(UndefinedTest, CopyConstructor) {
  Undefined u;
  Undefined u2(u);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&u2), nullptr);
  EXPECT_EQ(u2.type(), VluBase::UndefinedType);
};
TEST(UndefinedTest, toString) {
  EXPECT_STREQ(Undefined().toString().c_str(), "undefined");
};
TEST(UndefinedTest, serialize) {
  EXPECT_STREQ(Undefined().serialize(2,1).str().c_str(), "\n  undefined");
};
TEST(UndefinedTest, isUndefined) {
  EXPECT_EQ(Undefined().isUndefined(), true);
  EXPECT_EQ(Undefined().isNull(), false);
  EXPECT_EQ(Undefined().isBool(), false);
  EXPECT_EQ(Undefined().isNumber(), false);
  EXPECT_EQ(Undefined().isString(), false);
  EXPECT_EQ(Undefined().isArray(), false);
  EXPECT_EQ(Undefined().isObject(), false);
};
TEST(UndefinedTest, operatorEq) {
  Undefined u1, u2;
  EXPECT_EQ(u1 == u2, false);
};

// ------------------------------------------------------------

TEST(NullTest, asOther) {
  VluBase v(VluBase::NullType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_NO_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(NullTest, ConstructNoArgs) {
  Json::Null n;
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&n), nullptr);
};
TEST(NullTest, CopyConstructor) {
  Null u;
  Null u2(u);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&u2), nullptr);
  EXPECT_EQ(u2.type(), VluBase::NullType);
};
TEST(NullTest, toString) {
  EXPECT_STREQ(Null().toString().c_str(), "null");
};
TEST(NullTest, serialize) {
  EXPECT_STREQ(Null().serialize(2,1).str().c_str(), "\n  null");
};
TEST(NullTest, isNull) {
  Null v;
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), true);
  EXPECT_EQ(v.isBool(), false);
  EXPECT_EQ(v.isNumber(), false);
  EXPECT_EQ(v.isString(), false);
  EXPECT_EQ(v.isArray(), false);
  EXPECT_EQ(v.isObject(), false);
};
TEST(NullTest, operatorEq) {
  Null n1, n2;
  EXPECT_EQ(n1 == n2, true);
};

// ------------------------------------------------------------
TEST(BoolTest, asOther) {
  VluBase v(VluBase::BoolType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_NO_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(BoolTest, ConstructNoArgs) {
  Bool n(true);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&n), nullptr);
};
TEST(BoolTest, CopyConstructor) {
  Bool v(true);
  Bool v2(v);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v2), nullptr);
  EXPECT_EQ(v2.type(), VluBase::BoolType);
};
TEST(BoolTest, toString) {
  EXPECT_STREQ(Bool(true).toString().c_str(), "true");
  EXPECT_STREQ(Bool(false).toString().c_str(), "false");
};
TEST(BoolTest, serialize) {
  EXPECT_STREQ(Bool(true).serialize(2,1).str().c_str(), "\n  true");
  EXPECT_STREQ(Bool(false).serialize(2,2).str().c_str(), "\n    false");
};
TEST(BoolTest, isBool) {
  Bool v(true);
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), false);
  EXPECT_EQ(v.isBool(), true);
  EXPECT_EQ(v.isNumber(), false);
  EXPECT_EQ(v.isString(), false);
  EXPECT_EQ(v.isArray(), false);
  EXPECT_EQ(v.isObject(), false);
};
TEST(BoolTest, operatorEq) {
  Bool b1(true), b2(true), b3(false), b4(false);
  EXPECT_EQ(b1 == b2, true);
  EXPECT_EQ(b1 == b3, false);
  EXPECT_EQ(b3 == b4, true);
};
TEST(BoolTest, vlu) {
  EXPECT_EQ(Bool(false).vlu(), false);
  EXPECT_EQ(Bool(true).vlu(), true);
};

// ------------------------------------------------------------
TEST(NumberTest, NumberAsOther) {
  VluBase v(VluBase::NumberType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_NO_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(NumberTest, ConstructNoArgs) {
  Number v0(0), v1(0.1f);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v0), nullptr);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
};
TEST(NumberTest, CopyConstructor) {
  Number v(10);
  Number v2(v);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v2), nullptr);
  EXPECT_EQ(v2.type(), VluBase::NumberType);
};
TEST(NumberTest, toString) {
  EXPECT_STREQ(Number(10).toString().c_str(), "10");
  EXPECT_STREQ(Number(-10).toString().c_str(), "-10");
  EXPECT_STREQ(Number(-100.4f).toString().c_str(), "-100.400002");
};
TEST(NumberTest, serialize) {
  EXPECT_STREQ(Number(-100).serialize(2,1).str().c_str(), "\n  -100");
  EXPECT_STREQ(Number(30.5f).serialize(2,2).str().c_str(), "\n    30.500000");
};
TEST(NumberTest, isNumber) {
  Number v(1);
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), false);
  EXPECT_EQ(v.isBool(), false);
  EXPECT_EQ(v.isNumber(), true);
  EXPECT_EQ(v.isString(), false);
  EXPECT_EQ(v.isArray(), false);
  EXPECT_EQ(v.isObject(), false);
};
TEST(NumberTest, operatorEq) {
  Number b1(10), b2(10), b3(-0.5f), b4(-0.5f);
  EXPECT_EQ(b1 == b2, true);
  EXPECT_EQ(b1 == b3, false);
  EXPECT_EQ(b3 == b4, true);
};
TEST(NumberTest, vlu) {
  EXPECT_EQ(Number(3).vlu(), 3);
  EXPECT_EQ(Number(4.6f).vlu(), 4.6f);
};

// ------------------------------------------------------------

TEST(StringTest, asOther) {
  VluBase v(VluBase::StringType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_NO_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(StringTest, ConstructNoArgs) {
  String v0("zero"), v1("one");
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v0), nullptr);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
};
TEST(StringTest, CopyConstructor) {
  String v("copy");
  String v2(v);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v2), nullptr);
  EXPECT_EQ(v2.type(), VluBase::StringType);
};
TEST(StringTest, toString) {
  EXPECT_STREQ(String("str").toString().c_str(), "str");
  EXPECT_STREQ(String("s\nt\"r").toString().c_str(), "s\nt\"r");
};
TEST(StringTest, serialize) {
  EXPECT_STREQ(String("str").serialize(2,1).str().c_str(), "\n  \"str\"");
  EXPECT_STREQ(String("str2").serialize(2,2).str().c_str(), "\n    \"str2\"");
};
TEST(StringTest, isString) {
  String v("one");
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), false);
  EXPECT_EQ(v.isBool(), false);
  EXPECT_EQ(v.isNumber(), false);
  EXPECT_EQ(v.isString(), true);
  EXPECT_EQ(v.isArray(), false);
  EXPECT_EQ(v.isObject(), false);
};
TEST(StringTest, operatorEq) {
  String b1("one"), b2("one"), b3("two"), b4("two");
  EXPECT_EQ(b1 == b2, true);
  EXPECT_EQ(b1 == b3, false);
  EXPECT_EQ(b3 == b4, true);
};
TEST(StringTest, vlu) {
  EXPECT_EQ(String("one").vlu(), std::string("one"));
  EXPECT_EQ(String("two").vlu(), std::string("two"));
};

// --------------------------------------------------------------

TEST(ArrayTest, asOther) {
  VluBase v(VluBase::ArrayType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_NO_THROW(v.asArray());
  EXPECT_ANY_THROW(v.asObject());
};
TEST(ArrayTest, ConstructNoArgs) {
  Array v0, v1;
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v0), nullptr);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
};
TEST(ArrayTest, CopyConstructor) {
  Array v;
  Array v1(v);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
  EXPECT_EQ(v1.type(), VluBase::ArrayType);
};
TEST(ArrayTest, ConstructInitializerList) {
  Array a1 {Bool(true), String("hej"), Number(456)};
  EXPECT_EQ(a1.length(), 3);
  EXPECT_EQ(a1[0].type(), VluBase::BoolType);
  EXPECT_EQ(a1[1].type(), VluBase::StringType);
  EXPECT_EQ(a1[2].type(), VluBase::NumberType);
  EXPECT_EQ(a1[0].parent(), &a1);
  EXPECT_EQ(a1[1].parent(), &a1);
  EXPECT_EQ(a1[1].parent(), &a1);
};
TEST(ArrayTest, toString) {
  EXPECT_STREQ(Array().toString().c_str(), "Array()");
};
TEST(ArrayTest, serialize) {
  Array e;
  EXPECT_STREQ(e.serialize().str().c_str(), "[]");
  EXPECT_STREQ(e.serialize(2,1).str().c_str(), "\n  [\n  ]");
  EXPECT_STREQ(e.serialize(2,2).str().c_str(), "\n    [\n    ]");
  Array a; Null n; Number num(123);
  a.push(n);a.push(num);
  EXPECT_STREQ(a.serialize().str().c_str(), "[null,123]");
  EXPECT_STREQ(a.serialize(2,1).str().c_str(), "\n  [\n    null,\n    123\n  ]");
};
TEST(ArrayTest, isArray) {
  Array v;
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), false);
  EXPECT_EQ(v.isBool(), false);
  EXPECT_EQ(v.isNumber(), false);
  EXPECT_EQ(v.isString(), false);
  EXPECT_EQ(v.isArray(), true);
  EXPECT_EQ(v.isObject(), false);
};
TEST(ArrayTest, operatorEq) {
  Array b1, b2, b3{Null()}, b4{Null()};
  EXPECT_EQ(b1 == b2, true);
  EXPECT_EQ(b1 == b3, false);
  EXPECT_EQ(b3 == b4, true);
};
TEST(ArrayTest, empty) {
  Array a;
  EXPECT_EQ(a.length(), 0);
  EXPECT_EQ(!!a.pop(), false);
};
TEST(ArrayTest, push) {
  Bool b(true); String s("str"); Number n(123);
  Array a;
  a.push(b);
  EXPECT_EQ(a.length(), 1);
  a.push(std::unique_ptr<VluBase>(&s));
  EXPECT_EQ(a.length(), 2);
  a.push(n);
  EXPECT_EQ(a.length(), 3);

  EXPECT_EQ(a[0], b);
  EXPECT_EQ(a[0].parent(), &a);
  EXPECT_EQ(a[1], s);
  EXPECT_EQ(a[1].parent(), &a);
  EXPECT_EQ(a[2], n);
  EXPECT_EQ(a[2].parent(), &a);
};
TEST(ArrayTest, copyValuesOnCopyConstruct) {
  Array v1{Number(1234), String("nope"), Array{Null()}};
  Array v2(v1);
  EXPECT_EQ(v1.length(), v2.length());
  EXPECT_EQ(v1[0], v2[0]);
  EXPECT_EQ(v1[0].parent(), &v1);
  EXPECT_EQ(v2[0].parent(), &v2);
  EXPECT_EQ(v1[1], v2[1]);
  EXPECT_EQ(v1[1].parent(), &v1);
  EXPECT_EQ(v2[1].parent(), &v2);
  EXPECT_EQ(v1[2], v2[2]);
  EXPECT_EQ(v1[2].parent(), &v1);
  EXPECT_EQ(v2[2].parent(), &v2);
};
TEST(ArrayTest, pop) {
  Array v1{Number(123), String("str\nrts"), Bool(true)};
  EXPECT_EQ(v1.length(), 3);
  auto itm2 = v1.pop();
  EXPECT_EQ(*itm2->asBool(), Bool(true));
  EXPECT_NE(itm2->parent(), &v1);
  EXPECT_EQ(v1.length(),2);
  auto itm1 = v1.pop();
  EXPECT_EQ(*itm1->asString(), String("str\nrts"));
  EXPECT_NE(itm1->parent(), &v1);
  EXPECT_EQ(v1.length(),1);
  auto itm0 = v1.pop();
  EXPECT_EQ(*itm0->asNumber(), Number(123));
  EXPECT_NE(itm0->parent(), &v1);
  EXPECT_EQ(v1.length(), 0);
};
TEST(ArrayTest, operatorSet) {
  Array a1{Bool(true), Number(234)};
  Array a2;
  a2 = a1;
  ASSERT_EQ(a1.length(), a2.length());
  EXPECT_EQ(a2[0].asBool()->vlu(), true);
  EXPECT_EQ(a2[1].asNumber()->vlu(), 234);

  Array a3{Undefined()};
  EXPECT_ANY_THROW(a3=a1);
};

// --------------------------------------------------------------

TEST(ObjectTest, asOther) {
  VluBase v(VluBase::ObjectType, nullptr);
  EXPECT_ANY_THROW(v.asUndefined());
  EXPECT_ANY_THROW(v.asNull());
  EXPECT_ANY_THROW(v.asBool());
  EXPECT_ANY_THROW(v.asNumber());
  EXPECT_ANY_THROW(v.asString());
  EXPECT_ANY_THROW(v.asArray());
  EXPECT_NO_THROW(v.asObject());
};
TEST(ObjectTest, ConstructNoArgs) {
  Object v0, v1;
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v0), nullptr);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
};
TEST(ObjectTest, CopyConstructor) {
  Object v;
  Object v1(v);
  EXPECT_NE(dynamic_cast<Json::VluBase*>(&v1), nullptr);
  EXPECT_EQ(v1.type(), VluBase::ObjectType);
};
TEST(ObjectTest, ConstructInitializerList) {
  Object o1 {{"b", Bool(true)}, {"s", String("hej")}, {"n", Number(456)}};
  EXPECT_EQ(o1.length(), 3);
  EXPECT_EQ(o1["b"].type(), VluBase::BoolType);
  EXPECT_EQ(o1["s"].type(), VluBase::StringType);
  EXPECT_EQ(o1["n"].type(), VluBase::NumberType);
  EXPECT_EQ(o1["b"].parent(), &o1);
  EXPECT_EQ(o1["s"].parent(), &o1);
  EXPECT_EQ(o1["n"].parent(), &o1);
};
TEST(ObjectTest, toString) {
  EXPECT_STREQ(Object().toString().c_str(), "Object()");
};
TEST(ObjectTest, serialize) {
  Object e;
  EXPECT_STREQ(e.serialize().str().c_str(), "{}");
  EXPECT_STREQ(e.serialize(2,1).str().c_str(), "\n  {}");
  e.set("key", Null());
  EXPECT_STREQ(e.serialize(2,2).str().c_str(), "\n    {\n      \"key\":null\n    }");
  Object o;
  o.set("n", std::make_unique<Null>());
  o.set("num", std::make_unique<Number>(123));
  EXPECT_STREQ(o.serialize().str().c_str(), "{\"n\":null,\"num\":123}");
  EXPECT_STREQ(o.serialize(2,1).str().c_str(), "\n  {\n    \"n\":null,\n    \"num\":123\n  }");
};
TEST(ObjectTest, isObject) {
  Object v;
  EXPECT_EQ(v.isUndefined(), false);
  EXPECT_EQ(v.isNull(), false);
  EXPECT_EQ(v.isBool(), false);
  EXPECT_EQ(v.isNumber(), false);
  EXPECT_EQ(v.isString(), false);
  EXPECT_EQ(v.isArray(), false);
  EXPECT_EQ(v.isObject(), true);
};
TEST(ObjectTest, operatorEq) {
  Object b1, b2, b3{{"n", Null()}}, b4{{"n",Null()}};
  EXPECT_EQ(b1 == b2, true);
  EXPECT_EQ(b1 == b3, false);
  EXPECT_EQ(b3 == b4, true);
};
TEST(ObjectTest, empty) {
  Object a;
  EXPECT_EQ(a.length(), 0);
  EXPECT_ANY_THROW(a.remove("n"));
};
TEST(ObjectTest, setAndGet) {
  Bool b(true); String s("str"); Number n(123);
  Object o;
  o.set("b", b);
  EXPECT_EQ(o.length(), 1);
  o.set(String("s"), std::unique_ptr<VluBase>(&s));
  EXPECT_EQ(o.length(), 2);
  o.set(std::string("num"), n);
  EXPECT_EQ(o.length(), 3);

  EXPECT_EQ(o["b"], b);
  EXPECT_EQ(o.get("b")->parent(), &o);
  EXPECT_EQ(o["s"], s);
  EXPECT_EQ(o.get("s")->parent(), &o);
  EXPECT_EQ(o["num"], n);
  EXPECT_EQ(o.get("num")->parent(), &o);
};
TEST(ObjectTest, copyValuesOnCopyConstruct) {
  Object v1{
    {"num", Number(1234)}, {"s",String("nope")}, {"a",Array{Null()}}};
  Object v2(v1);
  EXPECT_EQ(v1.length(), v2.length());
  EXPECT_EQ(v1["num"], v2["num"]);
  EXPECT_EQ(v1.get("num")->parent(), &v1);
  EXPECT_EQ(v2.get("num")->parent(), &v2);
  EXPECT_EQ(v1["s"], v2["s"]);
  EXPECT_EQ(v1.get("s")->parent(), &v1);
  EXPECT_EQ(v2.get("s")->parent(), &v2);
  EXPECT_EQ(v1["a"], v2["a"]);
  EXPECT_EQ(v1.get("a")->parent(), &v1);
  EXPECT_EQ(v2.get("a")->parent(), &v2);
};
TEST(ObjectTest, contains) {
  Object v1{{"num",Number(123)}};
  EXPECT_EQ(v1.contains("num"), true);
  EXPECT_EQ(v1.contains("s"), false);
  EXPECT_EQ(v1.contains("b"), false);
  String s("str\nrts");
  v1.set("s", s);
  EXPECT_EQ(v1.contains("num"), true);
  EXPECT_EQ(v1.contains("s"), true);
  EXPECT_EQ(v1.contains("b"), false);
  Bool b(true);
  v1.set("b", b);
  EXPECT_EQ(v1.contains("num"), true);
  EXPECT_EQ(v1.contains("s"), true);
  EXPECT_EQ(v1.contains("b"), true);
};
TEST(ObjectTest, remove) {
  Object v1{{"num", Number(123)},{"s",String("str\nrts")}, {"b",Bool(true)}};
  EXPECT_EQ(v1.length(), 3);
  auto itm2 = v1.remove("b");
  ASSERT_NE(itm2, nullptr);
  EXPECT_EQ(*itm2->asBool(), Bool(true));
  EXPECT_NE(itm2->parent(), &v1);
  EXPECT_EQ(v1.length(),2);

  auto itm1 = v1.remove("s");
  ASSERT_NE(itm1, nullptr);
  EXPECT_EQ(*itm1->asString(), String("str\nrts"));
  EXPECT_NE(itm1->parent(), &v1);
  EXPECT_EQ(v1.length(),1);

  auto itm0 = v1.remove("num");
  ASSERT_NE(itm0, nullptr);
  EXPECT_EQ(*itm0->asNumber(), Number(123));
  EXPECT_NE(itm0->parent(), &v1);
  EXPECT_EQ(v1.length(), 0);
};
TEST(ObjectTest, operatorSet) {
  Object o1{{"b",Bool(true)}, {"num", Number(234)}};
  Object o2;
  o2 = o1;
  ASSERT_EQ(o1.length(), o2.length());
  EXPECT_EQ(o2["b"].asBool()->vlu(), true);
  EXPECT_EQ(o2["num"].asNumber()->vlu(), 234);

  Object o3{{"u", Undefined()}};
  EXPECT_ANY_THROW(o3=o1);
};
TEST(ObjectTest, keys) {
  Object o1{{"n",Null()},{"s",String("nej")},{"num",Number(123)}};
  auto keys = o1.keys();
  ASSERT_EQ(keys.size(), 3);
  // std::map is not ordered to last insert, but keys name
  EXPECT_EQ(keys[0], "n");
  EXPECT_EQ(keys[1], "num");
  EXPECT_EQ(keys[2],"s");
};
TEST(ObjectTest, values) {
  Object o1{{"n",Null()},{"s",String("nej")},{"num",Number(123)}};
  auto values = o1.values();
  ASSERT_EQ(values.size(), 3);
  // std::map is not ordered to last insert, but keys name
  ASSERT_EQ(values[0]->isNull(), true);
  ASSERT_EQ(values[1]->isNumber(), true);
  ASSERT_EQ(values[2]->isString(), true);
  EXPECT_EQ(values[1]->asNumber()->vlu(),123);
  EXPECT_EQ(values[2]->asString()->vlu(), "nej");
};

// -------------------------------------------------------------

TEST(ParseTest, asStringStream) {
  std::stringstream in;
  in << "[null,123]";
  auto vlu = parse(in);
  EXPECT_EQ(vlu->isArray(), true);
  EXPECT_EQ((*vlu->asArray())[0].isNull(), true);
  EXPECT_EQ((*vlu->asArray())[1].isNumber(), true);
  EXPECT_EQ((*vlu->asArray())[1].asNumber()->vlu(), 123);
};
TEST(ParseTest, asString) {
  std::string in("[null,123]");
  auto vlu = parse(in);
  EXPECT_EQ(vlu->isArray(), true);
  EXPECT_EQ((*vlu->asArray())[0].isNull(), true);
  EXPECT_EQ((*vlu->asArray())[1].isNumber(), true);
  EXPECT_EQ((*vlu->asArray())[1].asNumber()->vlu(), 123);
};
TEST(ParseTest, asCString) {
  auto vlu = parse("[null,123]");
  EXPECT_EQ(vlu->isArray(), true);
  EXPECT_EQ((*vlu->asArray())[0].isNull(), true);
  EXPECT_EQ((*vlu->asArray())[1].isNumber(), true);
  EXPECT_EQ((*vlu->asArray())[1].asNumber()->vlu(), 123);
};
TEST(ParseTest, objectRoot) {
  auto vlu = parse("{ \
    \"n\":null,\"b\":false,\"num\":123, \
    \"s\":\"hej!\",\"a\":[null],\"o\":{\"subNum\":321}}");
  ASSERT_EQ(vlu->isObject(), true);
  auto root = *vlu->asObject();
  EXPECT_EQ(root.length(), 6);
  EXPECT_EQ(root.parent(), nullptr);
  EXPECT_EQ(root["n"].isNull(), true);
  EXPECT_EQ(root["n"].parent(), &root);
  EXPECT_EQ(root["b"].asBool()->vlu(), false);
  EXPECT_EQ(root["b"].parent(), &root);
  EXPECT_EQ(root["num"].asNumber()->vlu(), 123);
  EXPECT_EQ(root["num"].parent(), &root);
  EXPECT_EQ(root["s"].asString()->vlu(), "hej!");
  EXPECT_EQ(root["s"].parent(), &root);
  EXPECT_EQ(root["a"].asArray()->length(), 1);
  EXPECT_EQ(root["a"].parent(), &root);
  EXPECT_EQ(*root["a"].asArray()->at(0), Null());
  EXPECT_EQ(root["a"].asArray()->at(0)->parent(), root.get("a"));
  EXPECT_EQ(root["o"].asObject()->length(), 1);
  EXPECT_EQ(root["o"].parent(), &root);
  EXPECT_EQ(root["o"].asObject()->get("subNum")->asNumber()->vlu(), 321);
  EXPECT_EQ(root["o"].asObject()->get("subNum")->parent(), root.get("o"));
  //std::cout << "ser:" << root.serialize(2,1).str() << std::endl;
};
TEST(ParseTest, numberParse) {
  VluType vlu = parse("{\"a\":023,\"b\":0.123, \
      \"c\":-45, \"d\": +65, \
      \"e\":3e2,\"f\":1E-9}");
  ASSERT_EQ(vlu->isObject(), true);
  auto root = *vlu->asObject();
  ASSERT_EQ(root.length(), 6);
  EXPECT_EQ(root["a"].asNumber()->vlu(), 23);
  EXPECT_FLOAT_EQ(root["b"].asNumber()->vlu(), 0.123);
  EXPECT_EQ(root["c"].asNumber()->vlu(),-45);
  EXPECT_EQ(root["d"].asNumber()->vlu(), +65);
  EXPECT_EQ(root["e"].asNumber()->vlu(), 300);
  EXPECT_FLOAT_EQ(root["f"].asNumber()->vlu(), 0.000000001);

};
TEST(ParseTest, invalidRoot) {
  EXPECT_ANY_THROW(parse("undefined"));
  EXPECT_ANY_THROW(parse("null"));
  EXPECT_ANY_THROW(parse("123"));
  EXPECT_ANY_THROW(parse("\"fail\""));
};
TEST(ParseTest, invalid) {
  EXPECT_ANY_THROW(parse("{undefined}"));
  EXPECT_ANY_THROW(parse("{null}"));
  EXPECT_ANY_THROW(parse("[undefined]"));
  EXPECT_ANY_THROW(parse("{o:123}"));
  EXPECT_ANY_THROW(parse("{\"o:345}"));
  EXPECT_ANY_THROW(parse("[null. 123]"));
  EXPECT_ANY_THROW(parse("{\"o\":546 \"p\":\"str\"}"));
}
} // namespace Json