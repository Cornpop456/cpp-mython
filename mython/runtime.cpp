#include "runtime.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace runtime {

const string STR = "__str__"s;
const string EQ = "__eq__"s;
const string LT = "__lt__"s;

ObjectHolder::ObjectHolder(shared_ptr<Object> data)
    : data_(move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if (auto obj = object.TryAs<Bool>()) {
        return obj->GetValue() == true;
    }

    if (auto obj = object.TryAs<Number>()) {
        return !(obj->GetValue() == 0);
    }

    if (auto obj = object.TryAs<String>()) {
        return !(obj->GetValue().empty());
    }

    return false;
}

void ClassInstance::Print(ostream& out, Context& context) {
    if (HasMethod(STR, 0U)) {
        Call(STR, {}, context)->Print(out, context);
    } else {
        out << this;
    }
}

bool ClassInstance::HasMethod(const string& method, size_t argument_count) const {
    if (auto m = cls_.GetMethod(method)) {
        if (m->formal_params.size() == argument_count) {
            return true;
        }
    }

    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls) : cls_(cls) {
}

ObjectHolder ClassInstance::Call(const string& method,
    const vector<ObjectHolder>& actual_args,
    Context& context) {

    if (!HasMethod(method, actual_args.size())) {
        throw runtime_error("No method "s + method +" in class "s + cls_.GetName()
            + " with "s + to_string(actual_args.size()) + " arguments."s);
    }

    auto m = cls_.GetMethod(method);
    Closure args;
    args["self"s] = ObjectHolder::Share(*this);

    size_t index = 0;

    for (auto& param : m->formal_params) {
        args[param] = actual_args.at(index++);
    }

    return m->body->Execute(args, context);
}

Class::Class(string name, vector<Method> methods, const Class* parent)
    : name_{move(name)}, methods_{move(methods)}, parent_{parent} {
}

const Method* Class::GetMethod(const string& name) const {
    auto find_by_name = [&name](const Method& m){
        return m.name == name;
    };

    if(auto res = find_if(methods_.begin(), methods_.end(), find_by_name);
            res != methods_.end()) {
        return &(*res);
    } else {
        if (parent_ != nullptr) {
            return parent_->GetMethod(name);
        } else {
            return nullptr;
        }
    }
}

[[nodiscard]] const string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& out, [[maybe_unused]] Context& context) {
    out << "Class "s << name_;
}

void Bool::Print(ostream& out, [[maybe_unused]] Context& context) {
    out << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return CompareObjects(lhs, rhs, equal_to());
    }  catch (runtime_error&) {
        if (auto l = lhs.TryAs<ClassInstance>()) {
            return IsTrue(l->Call(EQ, {rhs}, context));
        }

        if (!lhs && !rhs) {
            return true;
        }

        throw;
    }
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return CompareObjects(lhs, rhs, less());
    }  catch (runtime_error&) {
        if (auto l = lhs.TryAs<ClassInstance>()) {
            return IsTrue(l->Call(LT, {rhs}, context));
        }

        throw;
    }
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Less(lhs, rhs, context) || Equal(lhs, rhs, context));
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime