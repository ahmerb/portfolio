{-#LANGUAGE InstanceSigs#-}
{-#LANGUAGE TypeSynonymInstances#-}
{-#LANGUAGE FlexibleInstances#-}

module StaticSemanticsProc where

-- Import Prelude but import Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import syntactic and semantic type definitions etc for Proc
import SyntaxProc
import SharedSemanticsProc

-- Import Set and toList from Data.Set
import Data.Set (Set)
import qualified Data.Set as Set (toList)

---
-- Semantic type definitions
---

-- The semantic type Loc, to represent locations
type Loc = Integer

-- The semantic type Store, of to represent mapping from locs to values
type Store = Loc -> Z

-- Holds the next free address in the store
next :: Loc
next = 0 -- is a special token to be passed into a Sto. Initial store should map next to 1

-- The type for procedure environments, Env_p
data EnvP = EnvP (Pname -> (Stm, EnvV, EnvP))

-- The type for variable environments, Env_v
type EnvV = Var -> Loc

-- The configs for statement's transition system
data ConfigP = InterP Stm Store | FinalP Store

-- The configs for block's transition system
data ConfigD = InterD DecV EnvV Store | FinalD EnvV Store


---
-- Auxillary semantic functions
---

-- Given envp, pname and (stm, envv, envp'), return envp[ p |-> (stm ,envv, envp')]
updateEnvP :: EnvP -> Pname -> (Stm, EnvV, EnvP) -> EnvP
updateEnvP (EnvP f) p (stm, envv, envp) = EnvP $ \pname -> if pname == p
  then (stm, envv, envp)
  else f pname

updateEnvV :: EnvV -> Var -> Loc -> EnvV
updateEnvV envv v l = envv'
  where envv' :: EnvV
        envv' var
          | var == v  = l
          | otherwise = envv var

updateSto :: Store -> Loc -> Z -> Store
updateSto sto l i = sto'
  where sto' :: Store
        sto' loc
          | loc == l  = i
          | otherwise = sto loc

-- Increments a location by one
new :: Loc -> Loc
new x = x + 1

-- The function upd_p, which updates a procedure environment
upd_p :: (DecP, EnvV, EnvP) -> EnvP
upd_p ([]               , envv, envp) = envp
upd_p ((pname, pbody):ps, envv, envp) = upd_p (ps, envv, envp')
  where envp' = updateEnvP envp pname (pbody, envv, envp)


---
-- Natural Semantics with Static Scope
---

-- The function for evaluating a Proc Stm with the interface as described by the CW spec
s_static :: Stm -> State -> State
s_static stm s = finalStore . envv
  where (envv, sto) = toEnvVAndSto stm s
        finalStore  = s_ns stm envv sto

-- The natural semantics for Proc, implemented with static scope
s_ns :: Stm -> EnvV -> Store -> Store
s_ns stm envv sto = sto'
  where FinalP sto' = ns_stm (envv, _envp_init) (InterP stm sto)

-- the below return undefined given undefined var/proc/loc, not a trivial value.
-- so, if some var/proc/location is not defined, an error is raised.
-- as suggested by https://www.cs.bris.ac.uk/forum/display-thread.jsp?id=12614

-- sets all variables (none are defined on init) to undefined
_envv_init :: EnvV
_envv_init = \_ -> 0--undefined

-- sets all procs to undefined
_envp_init :: EnvP
_envp_init = EnvP $ \_ -> undefined

-- initial store maps next to 1, all other locations to undefined
_sto_init :: Store
_sto_init  = (\l -> if l == next
  then 1
  else undefined)


---
-- Transition system for statements (define relation ->)
---

ns_stm :: (EnvV, EnvP) -> ConfigP -> ConfigP

-- [ass_ns]
ns_stm (envv, envp) (InterP (Ass x a) sto) = FinalP sto'
  where sto' = updateSto sto l v
        l    = envv x
        v    = a_val a (sto . envv)

-- [skip_ns]
ns_stm (envv, envp) (InterP (Skip) sto) = FinalP sto

-- [comp_ns]
ns_stm (envv, envp) (InterP (Comp s1 s2) sto) = FinalP sto''
  where FinalP sto'  = ns_stm (envv, envp) (InterP s1 sto )
        FinalP sto'' = ns_stm (envv, envp) (InterP s2 sto')

-- [if^tt_ns], [if^ff_ns]
ns_stm (envv, envp) (InterP (If b s1 s2) sto) = FinalP sto'
  where FinalP sto'
          | b_val b (sto . envv) = ns_stm (envv, envp) (InterP s1 sto)
          | otherwise            = ns_stm (envv, envp) (InterP s2 sto)

-- [while^tt_ns], [while^ff_ns]
ns_stm (envv, envp) (InterP (While b s) sto)  = FinalP sto''
  where FinalP sto''
          | b_val b (sto . envv) = ns_stm (envv, envp) (InterP (While b s) sto')
          | otherwise            = FinalP sto
        FinalP sto' = ns_stm (envv, envp) (InterP s sto)

-- [block_ns]
ns_stm (envv, envp) (InterP (Block decvs decps s) sto) = FinalP sto''
  where FinalP sto''      = ns_stm (envv', envp') (InterP s sto')
        FinalD envv' sto' = ns_decv (InterD decvs envv sto)
        envp'             = upd_p (decps, envv', envp)

-- [call^rec_ns]
ns_stm (envv, EnvP envp) (InterP (Call p) sto) = FinalP sto'
  where FinalP sto'       = ns_stm (envv', updateEnvP envp' p (s, envv', envp')) (InterP s sto)
        (s, envv', envp') = envp p

---
-- Transition system for blocks (define relation ->_D)
---

ns_decv :: ConfigD -> ConfigD

-- [none_ns]
ns_decv (InterD [] envv sto) = FinalD envv sto

-- [var_ns]
ns_decv (InterD ((x, aexp):decvs) envv sto) = finalConfig
  where finalConfig = ns_decv $ InterD decvs envv' sto''
        l           = sto next
        v           = a_val aexp (sto . envv)
        envv'       = updateEnvV envv x l
        sto'        = updateSto sto l v
        sto''       = updateSto sto' next (new l)



---
-- Helpers
---


toEnvVAndSto :: Stm -> State -> (EnvV, Store)
toEnvVAndSto stm s = (envv, sto)
  where -- get the set of free vars in the statement
        fvs = Set.toList $ fv_stm stm
        -- compute envv
        envv = fst $ associateEachEnvVWithAUniqueLocation fvs
        associateEachEnvVWithAUniqueLocation = foldr f1 k1
        f1 = (\fv (envv, i) -> (updateEnvV envv fv (i+1), i+1))
        k1 = (_envv_init, 1)
        -- compute sto
        sto = fst $ associateEachLocationWithTheValueGivenByTheState fvs
        associateEachLocationWithTheValueGivenByTheState = foldr f2 k2
        f2 = (\fv (sto, i) -> (updateSto sto (i+1) (s fv), i+1))
        k2 = (_sto_init, 1)
