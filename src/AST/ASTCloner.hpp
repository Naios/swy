
/**
  Copyright(c) 2016 - 2017 Denis Blank <denis.blank at outlook dot com>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**/

#ifndef AST_CLONER_HPP_INCLUDED__
#define AST_CLONER_HPP_INCLUDED__

#include "ASTContext.hpp"
#include "SourceAnnotated.hpp"
#include "SourceLocation.hpp"

class ASTNode;
#define FOR_EACH_AST_NODE(NAME) class NAME##ASTNode;
#include "AST.inl"

/// The SourceRelocator provides methods for relocating the source location
/// of cloned ASTNodes properties.
class SourceRelocator {
public:
  SourceRelocator() = default;
  virtual ~SourceRelocator() = default;

  /// Relocates the given SourceLocation
  virtual SourceLocation relocate(SourceLocation const& loc) { return loc; }
  /// Relocates the given SourceRange
  virtual SourceRange relocate(SourceRange const& range) { return range; }
  /// Relocates the given SourceAnnotated
  template <typename Type, typename AnnotationType,
            typename = std::enable_if_t<
                std::is_same<AnnotationType, SourceLocation>::value ||
                std::is_same<AnnotationType, SourceRange>::value>>
  SourceAnnotated<Type, AnnotationType>
  relocate(SourceAnnotated<Type, AnnotationType> const& annotated) {
    return {*annotated, relocate(annotated.getAnnotation())};
  }
};

/// Provides methods for cloning an ASTNode itself without it's children.
///
/// A whole subtree can be cloned through invoking the cloner
/// together with the ASTLayoutReader.
class ASTCloner {
  ASTContext* context_;
  SourceRelocator* relocator_;

public:
  ASTCloner(ASTContext* context, SourceRelocator* relocator)
      : context_(context), relocator_(relocator) {}

  /// Clones the given node without it's children
  ASTNode* clone(ASTNode const* node);
/// Provide clone methods for all types of ASTNode's
#define FOR_EACH_AST_NODE(NAME)                                                \
  NAME##ASTNode* clone##NAME(NAME##ASTNode const* node);
#include "AST.inl"

private:
  /// Forwards to ASTContext::allocate
  template <typename T, typename... Args> T* allocate(Args&&... args) {
    return context_->allocate<T>(std::forward<Args>(args)...);
  }
  /// Relocates the given SourceLocation
  template <typename T> auto relocate(T&& type) {
    return relocator_->relocate(std::forward<T>(type));
  }
};

#endif // #ifndef AST_CLONER_HPP_INCLUDED__
