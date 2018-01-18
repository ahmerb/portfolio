module SharedSemanticsProc where

-- Import Prelude but import Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import Data.Set with methods as qualified
import Data.Set (Set)
import qualified Data.Set as Set

-- Import the syntactic type definitions for Proc
import SyntaxProc


---
-- Semantic type definitions
---

-- The semantic types Z and T as being synonymous with Haskell's native integer and boolean types
type Z = Integer
type T = Bool

-- The set of all possible states as the set of all possible functions from variables to integers
type State = Var -> Z

-- initial state that maps everything to zero (State's are total)
_state_init :: State
_state_init = (\x -> 0)

---
-- Auxillary Semantic Functions
---

-- Given state, integer, variable, returns the updated state s[v |-> i]
update :: State -> Var -> Z -> State
update s v i = s'
  where s' :: State
        s' var
          | var == v  = i
          | otherwise = s var

-- Given state s', set of vars X, state s, returns s'[X |-> s]
updateX :: State -> Set Var -> State -> State
updateX s' xs s = \v -> if v `Set.member` xs
  then s v
  else s' v

-- Find the free variables in a aexp
fv_aexp :: Aexp -> Set Var
fv_aexp (N n)        = Set.empty
fv_aexp (V v)        = Set.singleton v
fv_aexp (Mult a1 a2) = Set.union (fv_aexp a1) (fv_aexp a2)
fv_aexp (Add  a1 a2) = Set.union (fv_aexp a1) (fv_aexp a2)
fv_aexp (Sub  a1 a2) = Set.union (fv_aexp a1) (fv_aexp a2)

-- Find the free variables in a bexp
fv_bexp :: Bexp -> Set Var
fv_bexp (TRUE)      = Set.empty
fv_bexp (FALSE)     = Set.empty
fv_bexp (Eq  a1 a2) = Set.union (fv_aexp a1) (fv_aexp a2)
fv_bexp (Le  a1 a2) = Set.union (fv_aexp a1) (fv_aexp a2)
fv_bexp (And b1 b2) = Set.union (fv_bexp b1) (fv_bexp b2)
fv_bexp (Neg b1)    = fv_bexp b1

-- Find the free variables in a DecV
fv_decv :: DecV -> Set Var
fv_decv []                = Set.empty
fv_decv ((v, aexp):decvs) = Set.union (Set.union (Set.singleton v) (fv_aexp aexp)) (fv_decv decvs)

-- Find the free variables in a DecP
fv_decp :: DecP -> Set Var
fv_decp []               = Set.empty
fv_decp ((p, stm):decps) = Set.union (fv_stm stm) (fv_decp decps)

-- Find the free variables in a Stm
fv_stm :: Stm -> Set Var
fv_stm (Ass x a)       = Set.union (Set.singleton x) (fv_aexp a)
fv_stm (Skip)          = Set.empty
fv_stm (Comp s1 s2)    = Set.union (fv_stm s1) (fv_stm s2)
fv_stm (If b s1 s2)    = Set.union (fv_bexp b) (Set.union (fv_stm s1) (fv_stm s2))
fv_stm (While b s)     = Set.union (fv_bexp b) (fv_stm s)
fv_stm (Block vs ps s) = Set.union (fv_decv vs) (Set.union (fv_decp ps) (fv_stm s))
fv_stm (Call p)        = Set.empty

---
-- Semantic Functions for Numerals, Arithmetics and Booleans
---

-- The semantic function N[.]: Num -> Z for numerals
n_val :: Num -> Z
n_val n = n

-- The semantic function A[.]: Aexp -> (State -> Z) for arithmetic expressions
a_val :: Aexp -> State -> Z
a_val (N num) s            = n_val num
a_val (V var) s            = s var
a_val (Mult aexp0 aexp1) s = a_val aexp0 s * a_val aexp1 s
a_val (Add  aexp0 aexp1) s = a_val aexp0 s + a_val aexp1 s
a_val (Sub  aexp0 aexp1) s = a_val aexp0 s - a_val aexp1 s

-- The semantic function B[.]: Bexp -> (State -> T) for booleans
b_val :: Bexp -> State -> T
b_val TRUE  s             = True
b_val FALSE s             = False
b_val (Eq  aexp0 aexp1) s = (a_val aexp0 s) ==  (a_val aexp1 s)
b_val (Le  aexp0 aexp1) s = (a_val aexp0 s) <=  (a_val aexp1 s)
b_val (And bexp0 bexp1) s = (b_val bexp0 s) &&  (b_val bexp1 s)
b_val (Neg bexp) s        = not (b_val bexp s)
