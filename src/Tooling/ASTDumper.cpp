
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
#include "ASTDumper.hpp"

#include "llvm/ADT/Optional.h"
#include "llvm/ObjectYAML/YAML.h"
#include "llvm/Support/Casting.h"

#include "AST.hpp"
#include "ASTLayout.hpp"
#include "ASTStringer.hpp"

struct SimpleASTNode {
  llvm::Optional<ASTKind> kind;
  llvm::Optional<std::string> represents;
  llvm::Optional<std::vector<SimpleASTNode>> children;

  static SimpleASTNode createFrom(ASTNode const* node) {
    SimpleASTNode simpleNode;
    simpleNode.kind = node->getKind();

    simpleNode.represents = ASTStringer::toString(node);

    traverseNodeIf(node, pred::hasChildren(), [&](auto promoted) {
      auto children = promoted->children();
      if (!children.empty()) {
        simpleNode.children.emplace();
        for (auto child : children) {
          simpleNode.children->emplace_back(createFrom(child));
        }
      }
    });
    return simpleNode;
  }
};

template <> struct llvm::yaml::MappingTraits<SimpleASTNode> {
  static void mapping(llvm::yaml::IO& io, SimpleASTNode& node) {
    io.mapOptional("kind", node.kind);
    io.mapOptional("represents", node.represents);
    io.mapOptional("children", node.children);
  }
};

template <> struct llvm::yaml::ScalarEnumerationTraits<ASTKind> {
  static void enumeration(llvm::yaml::IO& io, ASTKind& value) {
#define FOR_EACH_AST_NODE(NAME) io.enumCase(value, #NAME, ASTKind::Kind##NAME);
#include "AST.inl"
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(SimpleASTNode)

struct SimpleLayoutASTNode {
  std::string kind;
  llvm::Optional<std::vector<SimpleLayoutASTNode>> children;

  static SimpleLayoutASTNode createUnstructured(Nullable<ASTNode const*> node) {
    SimpleLayoutASTNode flat;
    if (node) {
      flat.kind = ASTStringer::toTypeString(*node);
    } else {
      flat.kind = "<reduce marker>";
    }
    return flat;
  }

  static bool hasNodeChildren(ASTNode const* node) {
    return traverseNode(node,
                        decorate(identityOf<bool>(), pred::hasChildren()));
  }

  static std::size_t getFixedChildrenSize(ASTNode const* node) {
    return traverseNodeExpecting(
        node, pred::hasChildren(),
        decorate(identityOf<std::size_t>(), pred::getKnownAmountOfChildren()));
  }

  static SimpleLayoutASTNode
  createStructured(llvm::ArrayRef<Nullable<ASTNode*>>::const_iterator& itr,
                   llvm::ArrayRef<Nullable<ASTNode*>>::const_iterator end) {

    assert((itr != end) && "Malformed layout!");
    SimpleLayoutASTNode simpleNode;
    auto main = **itr;
    simpleNode.kind = ASTStringer::toTypeString(main);
    ++itr;

    // Return when the node has no children
    if (!hasNodeChildren(main)) {
      return simpleNode;
    }

    auto consume = [&] {
      assert((itr != end) && "Malformed layout!");
      assert(!itr->empty() && "Expected a valid node here!");
      simpleNode.children->push_back(createStructured(itr, end));
    };

    SimpleLayoutASTNode reduce;
    simpleNode.children.emplace();
    if (ASTLayoutWriter::isNodeRequiringReduceMarker(main)) {
      while (!itr->empty()) {
        consume();
      }
      assert((itr != end) && "Malformed layout!");
      assert(itr->empty() && "Expected a reduce marker here!");
      reduce.kind = "<reduce marker>";
      ++itr;
    } else {
      auto count = getFixedChildrenSize(main);
      for (decltype(count) i = 0U; i < count; ++i) {
        consume();
      }
      reduce.kind = "<obvious reduce>";
    }
    simpleNode.children->push_back(reduce);
    return simpleNode;
  }
};

template <> struct llvm::yaml::MappingTraits<SimpleLayoutASTNode> {
  static void mapping(llvm::yaml::IO& io, SimpleLayoutASTNode& node) {
    io.mapRequired("kind", node.kind);
    io.mapOptional("children", node.children);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(SimpleLayoutASTNode)

void dumpFlatLayout(llvm::raw_ostream& out,
                    llvm::ArrayRef<Nullable<ASTNode*>> layout) {

  llvm::yaml::Output yout(out);
  std::vector<SimpleLayoutASTNode> nodes;
  for (auto node : layout) {
    nodes.push_back(SimpleLayoutASTNode::createUnstructured(node));
  }
  yout << nodes;
  out.flush();
}

void dumpLayout(llvm::raw_ostream& out,
                llvm::ArrayRef<Nullable<ASTNode*>> layout) {
  llvm::yaml::Output yout(out);
  auto itr = layout.begin();
  auto end = layout.end();
  auto simpleNode = SimpleLayoutASTNode::createStructured(itr, end);
  assert((itr == end) && "Expected to be at the end of the layout!");
  yout << simpleNode;
  out.flush();
}

void dumpAST(llvm::raw_ostream& out, ASTNode const* astNode) {
  llvm::yaml::Output yout(out);
  auto simpleNode = SimpleASTNode::createFrom(astNode);
  yout << simpleNode;
  out.flush();
}
