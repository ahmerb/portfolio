module SyntaxProc where

-- Import Prelude with Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

---
-- Syntactic type definitions.
---


-- Define syntactic categories
type Num   = Integer
type Var   = String
type Pname = String
type DecV  = [(Var, Aexp)]
type DecP  = [(Pname, Stm)]


-- Define Abstract Syntax Tree for Proc

data Aexp = N Num
          | V Var
          | Mult Aexp Aexp
          | Add  Aexp Aexp
          | Sub  Aexp Aexp
          deriving (Show, Eq, Read)

data Bexp = TRUE
          | FALSE
          | Neg Bexp
          | Eq  Aexp Aexp
          | Le  Aexp Aexp
          | And Bexp Bexp
          deriving (Show, Eq, Read)

data Stm  = Ass Var Aexp
          | Skip
          | Comp Stm Stm
          | If Bexp Stm Stm
          | While Bexp Stm
          | Block DecV DecP Stm
          | Call Pname
          deriving (Show, Eq, Read)
