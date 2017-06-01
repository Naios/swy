
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
lexer grammar GeneratedLexer;

// https://github.com/antlr/antlr4/blob/master/doc/lexer-rules.md

Return: 'return';
// Func: 'func';
Meta: 'meta';
If: 'if';
Else: 'else';
For: 'for';
Break: 'break';
Continue: 'continue';

OpenPar: '(';
ClosePar: ')';
OpenCurly: '{';
CloseCurly: '}';
Arrow: '->';
Comma: ',';
Semicolon: ';';

True: 'true';
False: 'false';

IntegerLiteral: (OperatorPlus | OperatorMinus)? Digit+;
fragment Digit: [0-9];

Identifier: LETTER (LETTER | Digit )*;
fragment LETTER : [a-zA-Z_];

OperatorMul: '*';
OperatorDiv: '/';
OperatorPlus: '+';
OperatorMinus: '-';
OperatorAssign: '=';
OperatorLessThan: '<';
OperatorGreaterThan:  '>';
OperatorLessThanOrEq: '<=';
OperatorGreaterThanOrEq:  '>=';
OperatorEqual: '==';
OperatorNotEqual:  '!=';

Comment: '//' ~[\r\n]* '\r'? '\n' -> skip;
MultilineComment : '/*' .*? '*/' -> skip;
Whitespace: [ \t\r\n]+ -> skip;
