### Parser and Natural Semantics for the 'Proc' Language in Haskell

Proc is an extension of the While language which includes conditionals, loops, blocks, procedures and recursion. The parser was written using the Megaparsec library written in Haskell. The semantic interpreter was written from scratch. I followed the textbook by Nielson & Nielson in which they derive the natural semantics for the While and Proc languages. The are three implementations for the semantic interpreter. One using dynamic scope, one using a mixture of static and dynamic scope and one using static scope only. I achieved a First Class grade with marks of 82%.

## Details

In [cw2.hs](cw2.hs), we import the parser and the three semantic interpreters (static scope, dynamic scope, mixed scope). It also imports [Test.hs](Test.hs) and runs the tests defined in there. [SampleProgramsProc.hs](SampleProgramsProc.hs) contains strings of valid and invalid Proc code to be used in testing. 

[ParseProc.hs](ParseProc.hs) contains the parser. It's written using MegaParsec for the basic combinators. I used a mixture of monadic and applicative style parser combinators. In the course exam, we were expected to write a parser, including the basic combinators, for a newly presented language. I achieved First Class marks (77%).

[SyntaxProc.hs](SyntaxProc.hs) contains the Abstract Syntax Tree defining a possible parse of a Proc program. The top level variable is called `Stm`. The parser takes a `String` and produces a `Stm`. The interpreters take the Abstract Syntax Trees and a State store and output a new state store.

[DynamicSemanticsProc.hs](DynamicSemanticsProc.hs), [SharedSemanticsProc.hs](SharedSemanticsProc.hs) and [StaticSemanticsProc.hs](StaticSemanticsProc.hs) have the three different semantic interpreters. They implement a natural semantics for the Proc language. This includes subroutines and recursion. For the exam, we also studied operational (small step), denotational and axiomatic semantics. I studied the textbook by Nielson & Nielson.

