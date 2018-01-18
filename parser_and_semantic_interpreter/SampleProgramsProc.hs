module SampleProgramsProc where

-- Import Prelude with Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import AST for Proc
import SyntaxProc

-- Import parser
import ParseProc (parseProc)
import Text.Megaparsec (parse)

---
-- Samples programs
---

fac_loop :: String
fac_loop = "/*fac_loop (p.23)*/\n\
           \y:=1;\n\
           \while !(x=1) do (\n\
           \   y:=y*x;\n\
           \   x:=x-1\n\
           \)"

fac_loop_stm :: Stm
fac_loop_stm =
  Comp (Ass ("y") (N 1))
       (While (Neg (Eq (V "x") (N 1)))
         (Comp (Ass ("y") (Mult (V "y") (V "x")))
               (Ass ("x") (Sub (V "x")  (N 1))))
       )

------------------------------------------------------------

fac_call :: String
fac_call = "begin\n\
           \  proc fac is\n\
           \    begin\n\
           \      var z:=x;\n\
           \      if x=1 then\n\
           \        skip\n\
           \      else (\n\
           \        x:=x-1;\n\
           \        call fac;\n\
           \        y:=z*y )\n\
           \    end;\n\
           \  (y:=1;\n\
           \  call fac)\n\
           \end\n"

fac_call_stm :: Stm
fac_call_stm =
 Block []
       [ ("fac", Block [("z",V "x")]
                       []
                       (If (Eq (V "x") (N 1))
                          (Skip)
                          (Comp (Ass "x" (Sub (V "x") (N 1)))
                                (Comp (Call "fac")
                                      (Ass "y" (Mult (V "z") (V "y")))))))]
       (Comp (Ass "y" (N 1))
             (Call "fac"))

----------------------------------------------------------------

heavybrackets :: String
heavybrackets = "(begin\n\
                \   ((var x := 1;)\n\
                \    (var y := 2;)\n\
                \   )\n\
                \   ((proc p is (\n\
                \       skip)\n\
                \    ;)\n\
                \    (proc p2 is (\n\
                \       skip)\n\
                \    ;)\n\
                \   )\n\
                \   (skip);\n\
                \   (if (x=1) then (skip) else (skip;skip));\n\
                \   (while (true) do (skip))\n\
                \end)\n"

heavybrackets_stm :: Stm
heavybrackets_stm =
  Block [("x", N 1), ("y", N 2)]
        [("p", Skip), ("p2", Skip)]
        (Comp (Skip)
              (Comp (If (Eq (V "x") (N 1))
                        (Skip)
                        (Comp (Skip)
                              (Skip)))
                    (While (TRUE)
                           (Skip))))


----------------------------------------------------------------------

scopecheck :: String
scopecheck = "begin\n\
             \  var x:=0;\n\
             \  proc p is x:=x*2;\n\
             \  proc q is call p;\n\
             \  begin\n\
             \    var x:= 5;\n\
             \    proc p is x:=x+1;\n\
             \    call q;\n\
             \    y:=x\n\
             \  end\n\
             \end\n"

scopecheck_stm :: Stm
scopecheck_stm =
  Block [ ("x",N 0) ]
        [ ("p", Ass "x" (Mult (V "x") (N 2)))
        , ("q", Call "p")
        ]
        (Block [("x", N 5)]
               [("p", Ass "x" (Add (V "x") (N 1)))]
               (Comp (Call "q")
                     (Ass "y" (V "x"))))


---------------------------------------------------------------------

greedyelse :: String
greedyelse = "if (true) then\n\
             \  skip\n\
             \else\n\
             \  x := 5;\n\
             \  y := 4;\n\
             \skip\n" -- else is greedy so this should be included in it


greedyelse_stm :: Stm
greedyelse_stm =
  If (TRUE)
     (Skip)
     (Comp (Ass ("x") (N 5))
           (Comp (Ass ("y") (N 4))
                 (Skip)))


--------------------------------------------------------------------

greedydo :: String
greedydo = "while (true) do\n\
           \  skip;\n\
           \  skip;\n\
           \skip\n" -- do is greedy so should be included in it

greedydo_stm :: Stm
greedydo_stm =
  While (TRUE)
        (Comp (Skip) (Comp (Skip) (Skip)))


--------------------------------------------------------------

mutualrec :: String
mutualrec = "x := 3;\n\
            \r := 0;\n\
            \begin\n\
            \  proc isOdd is (\n\
            \    x := x - 1;\n\
            \    if (x=0) then (r := 1) else (call isEven)\n\
            \  );\n\
            \  proc isEven is (\n\
            \    x := x - 1;\n\
            \    if (x=0) then (r := 0) else (call isOdd)\n\
            \  );\n\
            \  call isOdd\n\
            \end\n"

mutualrec_stm :: Stm
mutualrec_stm = Comp (Ass "x" (N 3))
                 (Comp (Ass "r" (N 0))
                       (Block []
                              [("isOdd", Comp (Ass "x" (Sub (V "x") (N 1)))
                                              (If (Eq (V "x") (N 0)) (Ass "r" (N 1)) (Call "isEven")))
                              ,("isEven",Comp (Ass "x" (Sub (V "x") (N 1)))
                                              (If (Eq (V "x") (N 0)) (Ass "r" (N 0)) (Call "isOdd")))
                              ]
                              (Call "isOdd")))


--------------------------------------------------------------

selfrec :: String
selfrec = "// p incr's x until x=10, then sets x=20 and stops recursing\n\
          \x := 5;\n\
          \begin\n\
          \  proc p is (\n\
          \    x := x + 1;\n\
          \    if (x=10) then (x:=20) else (call p)\n\
          \  );\n\
          \  call p\n\
          \end\n"

selfrec_stm :: Stm
selfrec_stm = stm where Right stm = parse parseProc "" selfrec
