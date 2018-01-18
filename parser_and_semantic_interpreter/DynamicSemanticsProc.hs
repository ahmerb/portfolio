module DynamicSemanticsProc where

-- Import Prelude but import Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import syntactic and semantic type definitions etc for Proc
import SyntaxProc
import SharedSemanticsProc

-- Import Set from Data.Set
import Data.Set (Set)

---
-- Semantic type definitions
---

-- Procedure environment, that maps proc names to proc bodies
type EnvP = Pname -> Stm

-- The configs for statement's transition system
data ConfigP = InterP Stm State | FinalP State

-- The configs for block's transition system
data ConfigD = InterD DecV State | FinalD State


---
-- Auxillary semantic functions
---

-- Given envp, pname and (stm, envv, envp'), return envp[ p |-> (stm ,envv, envp')]
updateEnvP :: EnvP -> Pname -> Stm -> EnvP
updateEnvP envp p stm = \pname -> if pname == p
  then stm
  else envp pname

upd_p :: (DecP, EnvP) -> EnvP
upd_p ([], envp)                   = envp
upd_p ((pname, pbody):decps, envp) = upd_p (decps, updateEnvP envp pname pbody)

---
-- Natural Semantics with Static Scope
---

-- The function for evaluating a Proc Stm with the interface as described by the CW spec
s_dynamic :: Stm -> State -> State
s_dynamic stm s = s_ns stm _envp_init s

-- The natural semantics for Proc, implemented with static scope
s_ns :: Stm -> EnvP -> State -> State
s_ns stm envp s = s'
  where FinalP s' = ns_stm envp (InterP stm s)

-- the below return undefined given undefined var/proc/loc, not a trivial value.
-- so, if some var/proc/location is not defined, an error is raised.
-- as suggested by https://www.cs.bris.ac.uk/forum/display-thread.jsp?id=12614


-- initial EnvP, sets all pnames to undefined
_envp_init :: EnvP
_envp_init = \_ -> undefined

---
-- Transition system for statements (define relation ->)
---

ns_stm :: EnvP -> ConfigP -> ConfigP

-- [ass_ns]
ns_stm envp (InterP (Ass x aexp) s) = FinalP s'
  where s' = update s x (a_val aexp s)

-- [skip_ns]
ns_stm envp (InterP (Skip) s) = FinalP s

-- [comp_ns]
ns_stm envp (InterP (Comp stm1 stm2) s) = FinalP s''
  where FinalP s'  = ns_stm envp (InterP stm1 s )
        FinalP s'' = ns_stm envp (InterP stm2 s')

-- [if^tt_ns], [if^ff_ns]
ns_stm envp (InterP (If b stm1 stm2) s) = FinalP s'
  where FinalP s'
          | b_val b s = ns_stm envp (InterP stm1 s)
          | otherwise = ns_stm envp (InterP stm2 s)

-- [while^tt_ns], [while^ff_ns]
ns_stm envp (InterP (While b stm1) s)  = FinalP s''
  where FinalP s''
          | b_val b s = ns_stm envp (InterP (While b stm1) s')
          | otherwise = FinalP s
        FinalP s' = ns_stm envp (InterP stm1 s)

-- [block_ns]
ns_stm envp (InterP (Block decvs decps stm) s) = FinalP $ updateX s'' dvs s
  where FinalP s'' = ns_stm (upd_p (decps, envp)) (InterP stm s')
        FinalD s'  = ns_decv (InterD decvs s)
        dvs        = fv_decv decvs

-- [call^rec_ns]
ns_stm envp (InterP (Call p) s) = FinalP s'
  where FinalP s' = ns_stm envp (InterP stm s)
        stm       = envp p

---
-- Transition system for blocks (define relation ->_D)
---

ns_decv :: ConfigD -> ConfigD

-- [none_ns]
ns_decv (InterD [] s) = FinalD s

-- [var_ns]
ns_decv (InterD ((x, aexp):decvs) s) = finalConfig
  where finalConfig = ns_decv $ InterD decvs s'
        s'          = update s x (a_val aexp s)

---
-- Helpers
---
