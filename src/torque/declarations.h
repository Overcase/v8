// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TORQUE_DECLARATIONS_H_
#define V8_TORQUE_DECLARATIONS_H_

#include <string>

#include "src/torque/declarable.h"
#include "src/torque/scope.h"
#include "src/torque/utils.h"

namespace v8 {
namespace internal {
namespace torque {

class Declarations {
 public:
  Declarations()
      : unique_declaration_number_(0),
        current_generic_specialization_(nullptr) {}

  Declarable* TryLookup(const std::string& name) { return chain_.Lookup(name); }

  Declarable* Lookup(const std::string& name) {
    Declarable* d = TryLookup(name);
    if (d == nullptr) {
      std::stringstream s;
      s << "cannot find \"" << name << "\"";
      ReportError(s.str());
    }
    return d;
  }

  Declarable* LookupGlobalScope(const std::string& name) {
    Declarable* d = chain_.LookupGlobalScope(name);
    if (d == nullptr) {
      std::stringstream s;
      s << "cannot find \"" << name << "\" in global scope";
      ReportError(s.str());
    }
    return d;
  }

  const Type* LookupType(const std::string& name);
  const Type* LookupGlobalType(const std::string& name);
  const Type* GetType(TypeExpression* type_expression);

  const Type* GetFunctionPointerType(TypeVector argument_types,
                                     const Type* return_type);

  Builtin* FindSomeInternalBuiltinWithType(const FunctionPointerType* type);

  Value* LookupValue(const std::string& name);

  Macro* LookupMacro(const std::string& name, const TypeVector& types);

  Builtin* LookupBuiltin(const std::string& name);

  Label* LookupLabel(const std::string& name);

  Generic* LookupGeneric(const std::string& name);

  const AbstractType* DeclareAbstractType(const std::string& name,
                                          const std::string& generated,
                                          const std::string* parent = nullptr);

  void DeclareTypeAlias(const std::string& name, const Type* aliased_type);

  Label* DeclareLabel(const std::string& name);

  Macro* DeclareMacro(const std::string& name, const Signature& signature);

  Builtin* DeclareBuiltin(const std::string& name, Builtin::Kind kind,
                          bool external, const Signature& signature);

  RuntimeFunction* DeclareRuntimeFunction(const std::string& name,
                                          const Signature& signature);

  Variable* DeclareVariable(const std::string& var, const Type* type);

  Parameter* DeclareParameter(const std::string& name,
                              const std::string& mangled_name,
                              const Type* type);

  Label* DeclarePrivateLabel(const std::string& name);

  void DeclareConstant(const std::string& name, const Type* type,
                       const std::string& value);

  Generic* DeclareGeneric(const std::string& name, Module* module,
                          GenericDeclaration* generic);

  TypeVector GetCurrentSpecializationTypeNamesVector();

  ScopeChain::Snapshot GetScopeChainSnapshot() { return chain_.TaskSnapshot(); }

  std::set<const Variable*> GetLiveVariables() {
    return chain_.GetLiveVariables();
  }

  Statement* next_body() const { return next_body_; }

  void PrintScopeChain() { chain_.Print(); }

  class NodeScopeActivator;
  class GenericScopeActivator;
  class ScopedGenericSpecializationKey;
  class ScopedGenericScopeChainSnapshot;

 private:
  Scope* GetNodeScope(const AstNode* node);
  Scope* GetGenericScope(Generic* generic, const TypeVector& types);

  template <class T>
  T* RegisterDeclarable(std::unique_ptr<T> d) {
    T* ptr = d.get();
    declarables_.push_back(std::move(d));
    return ptr;
  }

  void Declare(const std::string& name, std::unique_ptr<Declarable> d) {
    chain_.Declare(name, RegisterDeclarable(std::move(d)));
  }

  int GetNextUniqueDeclarationNumber() { return unique_declaration_number_++; }

  void CheckAlreadyDeclared(const std::string& name, const char* new_type);

  int unique_declaration_number_;
  ScopeChain chain_;
  const SpecializationKey* current_generic_specialization_;
  Statement* next_body_;
  std::vector<std::unique_ptr<Declarable>> declarables_;
  Deduplicator<FunctionPointerType> function_pointer_types_;
  std::map<std::pair<const AstNode*, TypeVector>, Scope*> scopes_;
  std::map<Generic*, ScopeChain::Snapshot> generic_declaration_scopes_;
};

class Declarations::NodeScopeActivator {
 public:
  NodeScopeActivator(Declarations* declarations, AstNode* node)
      : activator_(declarations->GetNodeScope(node)) {}

 private:
  Scope::Activator activator_;
};

class Declarations::GenericScopeActivator {
 public:
  GenericScopeActivator(Declarations* declarations,
                        const SpecializationKey& key)
      : activator_(declarations->GetGenericScope(key.first, key.second)) {}

 private:
  Scope::Activator activator_;
};

class Declarations::ScopedGenericSpecializationKey {
 public:
  ScopedGenericSpecializationKey(Declarations* declarations,
                                 const SpecializationKey& key)
      : declarations_(declarations) {
    declarations->current_generic_specialization_ = &key;
  }
  ~ScopedGenericSpecializationKey() {
    declarations_->current_generic_specialization_ = nullptr;
  }

 private:
  Declarations* declarations_;
};

class Declarations::ScopedGenericScopeChainSnapshot {
 public:
  ScopedGenericScopeChainSnapshot(Declarations* declarations,
                                  const SpecializationKey& key)
      : restorer_(declarations->generic_declaration_scopes_[key.first]) {}
  ~ScopedGenericScopeChainSnapshot() {}

 private:
  ScopeChain::ScopedSnapshotRestorer restorer_;
};

}  // namespace torque
}  // namespace internal
}  // namespace v8

#endif  // V8_TORQUE_DECLARATIONS_H_
