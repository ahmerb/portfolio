module Test where

-- TODO: make procedure body's parse as small as possible, i.e. non greedy

-- Import Prelude but import Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import Data.Set
import Data.Set (Set)
import qualified Data.Set as Set

-- Import Megaparsec with parse as qualified
import Text.Megaparsec hiding (parse)
import qualified Text.Megaparsec as M (parse)

-- Import Parser and Three Different Semantics
import SyntaxProc
import ParseProc
import SharedSemanticsProc
import StaticSemanticsProc (s_static)
import qualified StaticSemanticsProc as Static
import DynamicSemanticsProc (s_dynamic)
import qualified DynamicSemanticsProc as Dynamic
import MixedSemanticsProc (s_mixed)
import qualified MixedSemanticsProc as Mixed

-- Import any other helpers/test stubs etc
import SampleProgramsProc

-- Import HUnit
import Test.HUnit

---
-- Run tests
---

testall = do
  putStrLn "\nRunning Parser Tests..."
  c1 <- testparser
  putStrLn "\nRunning Static Semantics Tests..."
  c2 <- teststatic
  putStrLn "\nRunning Dynamic Semantics Tests..."
  c3 <- testdynamic
  putStrLn "\nRunning Mixed Semantics Tests..."
  c4 <- testmixed
  putStrLn "\nTotals..."
  return (sumCounts [c1, c2, c3, c4])

testparser  = runTestTT parserTests
teststatic  = runTestTT staticTests
testdynamic = runTestTT dynamicTests
testmixed   = runTestTT mixedTests

-- Counts is the return type of runTestTT, which gives cases, tried, erros failures :: Int
-- this function takes a list of Counts and sums them
sumCounts :: [Counts] -> Counts
sumCounts [] = Counts 0 0 0 0
sumCounts (c:cs) = c `countsPlus` sumCounts cs
  where countsPlus :: Counts -> Counts -> Counts
        countsPlus (Counts a b c d) (Counts w x y z) = Counts (a+w) (b+x) (c+y) (d+z)

---
-- Test ParseProc
---


facloop = TestCase (assertEqual "fac_loop"
                                (fac_loop_stm)
                                (parse fac_loop))


faccall = TestCase (assertEqual "fac_call"
                                (fac_call_stm)
                                (parse fac_call))


_ass = "\n(x := 5\n\n)\n"
_ass_stm = Ass "x" (N 5)
ass_test = TestCase (assertEqual "assign stmnt"
                                 (_ass_stm)
                                 (parse _ass))


_skip = "(skip)"
_skip_stm = Skip
skip_test = TestCase (assertEqual "skip stmnt"
                                  (_skip_stm)
                                  (parse _skip))


_comp = "(skip; skip)"
_comp_stm = Comp Skip Skip
comp_test = TestCase (assertEqual "comp stmnt"
                                  (_comp_stm)
                                  (parse _comp))


_comp2 = "(skip;skip;skip)"
_comp2_stm = Comp Skip (Comp Skip Skip)
comp2_test = TestCase (assertEqual "comp stmnt is right assoc"
                                  (_comp2_stm)
                                  (parse _comp2))


_comp3 = "(skip;skip);skip"
_comp3_stm = Comp (Comp Skip Skip) Skip
comp3_test = TestCase (assertEqual "comp stmnt obeys bracketing"
                                   (_comp3_stm)
                                   (parse _comp3))


_block = "(begin var x := 5; var y := 6; skip end)"
_block_stm = Block [("x", N 5), ("y", N 6)] [] (Skip)
block_test = TestCase (assertEqual "a simple block with only 2 var decs"
                                   (_block_stm)
                                   (parse _block))


_call = "(call someproc)"
_call_stm = Call "someproc"
call_test = TestCase (assertEqual "a procedure call"
                                  (_call_stm)
                                  (parse _call))


heavybrackets_test = TestCase (assertEqual "complex statement with brackets around everything"
                                           (heavybrackets_stm)
                                           (parse heavybrackets))


greedyelse_test = TestCase (assertEqual "else is greedy"
                                        (greedyelse_stm)
                                        (parse greedyelse))

greedydo_test = TestCase (assertEqual "do is greedy"
                                      (greedydo_stm)
                                      (parse greedydo))



parserTests = TestList [ TestLabel "facLoop" facloop
                       , TestLabel "facCall" faccall
                       , TestLabel "ass" ass_test
                       , TestLabel "skip" skip_test
                       , TestLabel "comp1" comp_test
                       , TestLabel "comp2" comp2_test
                       , TestLabel "comp3" comp3_test
                       , TestLabel "block" block_test
                       , TestLabel "proc" call_test
                       , TestLabel "heavybrackets" heavybrackets_test
                       , TestLabel "greedyelse" greedyelse_test
                       , TestLabel "greedydo" greedydo_test
                       ]

---
-- Test Static Semantics
---

scopecheck_static_test = TestCase (assertEqual "scope check program from pg59 nielson"
                                        (5)
                                        (s' "y"))
  where s' = s_static scopecheck_stm _state_init


fac_call_static_test = TestCase (assertEqual "fac_call"
                                             (6)      -- 3! = 6
                                             (s'' "y"))
  where s'  = update _state_init "x" 3 -- give initial value of x as 3
        s'' = s_static fac_call_stm s'


selfrec_static_test = TestCase (assertEqual "selfrec"
                                            (20)
                                            (s' "x"))
  where s' = s_static selfrec_stm _state_init

mutualrec_static_test = TestCase (assertEqual "mutualrec"
                                               (1)
                                               (s' "r"))
  where s' = s_static mutualrec_stm _state_init

--- Test Conversion of State to EnvV and Store
s1 = \v -> case v of
  "x"       -> 7
  "y"       -> 8
  "z"       -> 9
  otherwise -> 0
stm1 = Ass ("x") (Mult (V "y") (V "z"))
conversion1_test = TestCase (do (envv, sto) <- return $ Static.toEnvVAndSto stm1 s1
                                -- envv
                                assertEqual "envv x" 4 (envv "x")
                                assertEqual "envv y" 3 (envv "y")
                                assertEqual "envv z" 2 (envv "z")
                                -- store
                                assertEqual "store 1 (x)" (s1 "x") (sto 4)
                                assertEqual "store 2 (y)" (s1 "y") (sto 3)
                                assertEqual "store 3 (z)" (s1 "z") (sto 2))


staticTests = TestList [ TestLabel "scopecheck" scopecheck_static_test
                       , TestLabel "fac_call" fac_call_static_test
                       , TestLabel "state to store . envv" conversion1_test
                       , TestLabel "selfrec" selfrec_static_test
                       , TestLabel "mutualrec" mutualrec_static_test
                       ]


---
-- Test dynamic semantics
---

scopecheck_dynamic_test = TestCase (assertEqual "scope check program from pg59 nielson"
                                                (6)
                                                (s' "y"))
  where s' = s_dynamic scopecheck_stm _state_init


fac_call_dynamic_test = TestCase (assertEqual "fac_call"
                                             (120)      -- 5! = 120
                                             (s'' "y"))
  where s'  = update _state_init "x" 5 -- give initial value of x as 5
        s'' = s_dynamic fac_call_stm s'


selfrec_dynamic_test = TestCase (assertEqual "selfrec"
                                           (20)
                                           (s' "x"))
  where s' = s_dynamic selfrec_stm _state_init

mutualrec_dynamic_test = TestCase (assertEqual "mutualrec"
                                               (1)
                                               (s' "r"))
  where s' = s_dynamic mutualrec_stm _state_init


dynamicTests = TestList [ TestLabel "scopecheck" scopecheck_dynamic_test
                        , TestLabel "fac_call" fac_call_dynamic_test
                        , TestLabel "selfrec" selfrec_dynamic_test
                        , TestLabel "mutualrec" mutualrec_dynamic_test
                        ]


---
-- Test mixed semantics
---

scopecheck_mixed_test = TestCase (assertEqual "scope check program from pg59 nielson"
                                 (10)
                                 (s' "y"))
  where s' = s_mixed scopecheck_stm _state_init


fac_call_mixed_test = TestCase (assertEqual "fac_call"
                                            (120)      -- 5! = 120
                                            (s'' "y"))
  where s'  = update _state_init "x" 5 -- give initial value of x as 5
        s'' = s_mixed fac_call_stm s'


selfrec_mixed_test = TestCase (assertEqual "selfrec"
                                           (20)
                                           (s' "x"))
  where s' = s_mixed selfrec_stm _state_init

mutualrec_mixed_test = TestCase (assertEqual "mutualrec"
                                               (1)
                                               (s' "r"))
  where s' = s_mixed mutualrec_stm _state_init

mixedTests = TestList [ TestLabel "scopecheck" scopecheck_mixed_test
                      , TestLabel "fac_call" fac_call_mixed_test
                      , TestLabel "selfrec" selfrec_mixed_test
                      , TestLabel "mutualrec" mutualrec_mixed_test
                      ]
