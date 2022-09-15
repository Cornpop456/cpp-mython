#include "statement.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
const string EMPTY_OBJECT = "None"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_->Execute(closure, context);
    
    return closure.at(var_);
}

Assignment::Assignment(string var, unique_ptr<Statement> rv)
    : var_{move(var)}, rv_{move(rv)} {
}

VariableValue::VariableValue(const string& var_name)
    : var_name_{var_name} {
}

VariableValue::VariableValue(vector<string> dotted_ids) {
    if (auto size = dotted_ids.size(); size > 0) {
        var_name_ = move(dotted_ids.at(0));
        
        dotted_ids_.resize(size - 1);
        
        move(next(dotted_ids.begin()), dotted_ids.end(), dotted_ids_.begin());
    }
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& context) {
    if (closure.count(var_name_)) {
        auto result = closure.at(var_name_);
        
        if (dotted_ids_.size() > 0) {
            if (auto obj = result.TryAs<runtime::ClassInstance>()) {
                return VariableValue(dotted_ids_).Execute(obj->Fields(), context);
            } else {
                throw runtime_error("Var " + var_name_+ " is not class"s);
            }
        }
        
        return result;
    } else {
        throw runtime_error("Var "s + var_name_ + " not found"s);
    }
}

unique_ptr<Print> Print::Variable(const string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args_{move(args)} {
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    bool first = true;
    
    auto& out = context.GetOutputStream();
    
    for (auto& arg : args_) {
        if (!first) {
            out << " "s;
        }
        
        auto obj = arg->Execute(closure, context);
        
        if (obj) {
            obj->Print(out, context);
        } else {
            out << EMPTY_OBJECT;
        }
        
        first = false;
    }
    out << endl;
    
    return {};
}

MethodCall::MethodCall(unique_ptr<Statement> object, string method,
    vector<unique_ptr<Statement>> args)
        : object_{move(object)}, method_{move(method)}, args_{move(args)} {
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {

    if (auto class_instance =
        object_->Execute(closure, context).TryAs<runtime::ClassInstance>()) {
        vector<runtime::ObjectHolder> executed_args;
        
        for (auto &arg : args_) {
            executed_args.push_back(arg->Execute(closure, context));
        }
        
        return class_instance->Call(method_, executed_args, context);
    } else {
        throw runtime_error("Obj is not class instance"s);
    }
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    auto obj = argument_->Execute(closure, context);
    
    if (obj) {
        ostringstream out;
        obj->Print(out, context);
        
        return ObjectHolder::Own(runtime::String(out.str()));
    } else {
        return ObjectHolder::Own(runtime::String(EMPTY_OBJECT));
    }
}

// макрос, сокращающий дублирование кода при проверке типов
#define CHECK_TYPE(TYPE, LHS, RHS) LHS.TryAs<TYPE>() && RHS.TryAs<TYPE>()

// макрос, сокращающий дублирование кода при применени операции
#define COMPUTE_AS_TYPE(TYPE, OP, LHS, RHS) \
    ObjectHolder::Own(TYPE(LHS.TryAs<TYPE>()->GetValue() OP RHS.TryAs<TYPE>()->GetValue())); 

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_h = lhs_->Execute(closure, context);
    ObjectHolder rhs_h = rhs_->Execute(closure, context);

    if (CHECK_TYPE(runtime::Number, lhs_h, rhs_h)) {
        return COMPUTE_AS_TYPE(runtime::Number, +, lhs_h, rhs_h)
    }

    if (CHECK_TYPE(runtime::String, lhs_h, rhs_h)) {
        return COMPUTE_AS_TYPE(runtime::String, +, lhs_h, rhs_h)
    }
    
    if (auto left_class = lhs_h.TryAs<runtime::ClassInstance>()) {
        return left_class->Call(ADD_METHOD, {rhs_h}, context);
    }
    
    throw runtime_error("Can only add nums, strings, class instances with "s + ADD_METHOD);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_h = lhs_->Execute(closure, context);
    ObjectHolder rhs_h = rhs_->Execute(closure, context);

    if (CHECK_TYPE(runtime::Number, lhs_h, rhs_h)) {
        return COMPUTE_AS_TYPE(runtime::Number, -, lhs_h, rhs_h)
    }
    
    throw runtime_error("Can sub only nums"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_h = lhs_->Execute(closure, context);
    ObjectHolder rhs_h = rhs_->Execute(closure, context);

    if (CHECK_TYPE(runtime::Number, lhs_h, rhs_h)) {
        return COMPUTE_AS_TYPE(runtime::Number, *, lhs_h, rhs_h)
    }
    
    throw runtime_error("Can multiply only nums"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_h = lhs_->Execute(closure, context);
    ObjectHolder rhs_h = rhs_->Execute(closure, context);
    
    auto right = rhs_h.TryAs<runtime::Number>();
    
    if (right && right->GetValue() == 0) {
        throw runtime_error("Division by zero"s);
    }

    if (CHECK_TYPE(runtime::Number, lhs_h, rhs_h)) {
        return COMPUTE_AS_TYPE(runtime::Number, /, lhs_h, rhs_h)
    }
    
    throw runtime_error("Can divide only nums"s);
}

#undef CHECK_TYPE
#undef COMPUTE_AS_TYPE

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (auto &arg : args_) {
        arg->Execute(closure, context);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : cls_{move(cls)} {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(VariableValue object, string field_name,
    unique_ptr<Statement> rv)
        : object_{move(object)}, field_name_{move(field_name)}, rv_{move(rv)} {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    auto obj = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
    
    if (obj) {
        return obj->Fields()[field_name_] = rv_->Execute(closure, context);
    } else {
        throw runtime_error("Object is not class"s);
    }
}

IfElse::IfElse(unique_ptr<Statement> condition, unique_ptr<Statement> if_body,
    unique_ptr<Statement> else_body)
        : condition_{move(condition)}
        , if_body_{move(if_body)}
        , else_body_{move(else_body)} {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    } else if (else_body_) {
        return else_body_->Execute(closure, context);
    }
    
    return runtime::ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context))) {
            return ObjectHolder::Own(runtime::Bool(true));
    }

    return ObjectHolder::Own(runtime::Bool(runtime::IsTrue
        (rhs_->Execute(closure, context))));
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue
            (rhs_->Execute(closure, context))));
    }

    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    bool result = !runtime::IsTrue(argument_->Execute(closure, context));
    
    return ObjectHolder::Own(runtime::Bool(result));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(move(lhs), move(rhs)) {
    cmp_ = move(cmp);
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    auto result = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
    
    return runtime::ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& class_,
    vector<unique_ptr<Statement>> args) 
        : instance_{class_}, args_{move(args)} {
}

NewInstance::NewInstance(const runtime::Class& class_)
    : NewInstance{class_, {}} {
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    if (instance_.HasMethod(INIT_METHOD, args_.size())) {
        vector<runtime::ObjectHolder> executed_args;
        
        for (auto& arg : args_) {
            executed_args.push_back(arg->Execute(closure, context));
        }
        
        instance_.Call(INIT_METHOD, executed_args, context);
    }

    return runtime::ObjectHolder::Share(instance_);
}

MethodBody::MethodBody(unique_ptr<Statement>&& body)
    : body_{move(body)} {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
        return runtime::ObjectHolder::None();
    }  catch (runtime::ObjectHolder &result) {
        return result;
    }  catch (...) {
        throw;
    }
}

}  // namespace ast