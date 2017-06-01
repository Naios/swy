
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

#ifndef FOR_EACH_DIAG
  #define FOR_EACH_DIAG(SEVERITY, NAME, MESSAGE)
#endif

#define DIAG_AS_ENUM(SEVERITY, NAME) SEVERITY##NAME

////////////////////
// Notes
// TODO remove this after rewrite
FOR_EACH_DIAG(Note, PreviousDeclarationHint,
  "Previously declared here")

FOR_EACH_DIAG(Note, PreviousTypeDeclarationHint,
  "Previously declared as {} here")

FOR_EACH_DIAG(Note, DeclarationHint,
  "'{}' is declared here")

FOR_EACH_DIAG(Note, DidYouMeanQuestion,
  "Did you mean {} '{}' declared here?")

FOR_EACH_DIAG(Note, InstantiatingMetaDecl,
  "Instantiating meta instantiation '{}'")

FOR_EACH_DIAG(Note, InstantiationExported,
  "Meta instantiation '{}' exported variable '{}' as constant value '{}'.")

////////////////////
// Warnings
FOR_EACH_DIAG(Warning, DidYouMeanEquals,
  "Did you intend to use '=='? ,-)")

////////////////////
// Errors
FOR_EACH_DIAG(Error, GenericParserFault,
  "{}")

FOR_EACH_DIAG(Error, FunctionNameReserved,
  "Name '{}' is reserved already and thus it "
  "can't be used as a function name!")

FOR_EACH_DIAG(Error, MetaNameReserved,
  "Name '{}' is reserved already and thus it "
  "can't be used as a meta name!")

// TODO remove this after rewrite
FOR_EACH_DIAG(Error, NameDeclaredAlready,
  "Name '{}' is already declared!")

FOR_EACH_DIAG(Error, NameTypeDeclaredAlready,
  "The name of {} '{}' is declared already!")

FOR_EACH_DIAG(Error, DeclarationUnknown,
  "Name '{}' isn't known in this scope!")

FOR_EACH_DIAG(Error, IntegralForMetaDecl,
  "Only integral values are allowed for meta arguments!")

FOR_EACH_DIAG(Error, OnlyIntPermitted,
  "Sorry, only type 'int' is usable as data type at the moment, "
  "thus '{}' isn't permitted :-(")

FOR_EACH_DIAG(Error, ArgumentTaken,
  "Argument name '{}' is already taken")

FOR_EACH_DIAG(Error, ConvertionFailure,
  "Failed to convert '{}' to an integer!")

FOR_EACH_DIAG(Error, FunctionCallReturnsVoid,
  "Tried to retrieve a result from function '{}' which "
  "returns void!")

FOR_EACH_DIAG(Error, FunctionCallArgCountMismatch,
  "Tried to call function '{}' with {} arguments, "
  "but expected {}!")

FOR_EACH_DIAG(Error, CanOnlyCallFunctions,
   "Can only apply the call operator to functions!")

FOR_EACH_DIAG(Error, InstantiatedNonMetaDecl,
  "Tried to instantiate the non meta declaration '{}'!")

FOR_EACH_DIAG(Error, InstantiationArgCountMismatch,
  "Tried to instantiate the meta declaration '{}' with {} arguments, "
  "but expected {}!")

FOR_EACH_DIAG(Error, InstantiationNoExport,
  "The meta instantiation of '{}' didn't export any "
  "declaration with it's name!")

#undef AS_ENUM
#undef FOR_EACH_DIAG
