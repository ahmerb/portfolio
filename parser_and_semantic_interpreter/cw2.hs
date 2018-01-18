module CW2 where

-- Import Prelude but import Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import Megaparsec with parse as qualified
import Text.Megaparsec hiding (parse)
import qualified Text.Megaparsec as M (parse)

-- Import Parser and Three Different Semantics
import SyntaxProc
import ParseProc (parse)
import qualified ParseProc as Parse
import SharedSemanticsProc
import StaticSemanticsProc (s_static)
import qualified StaticSemanticsProc as Static
import DynamicSemanticsProc (s_dynamic)
import qualified DynamicSemanticsProc as Dynamic
import MixedSemanticsProc (s_mixed)
import qualified MixedSemanticsProc as Mixed

import qualified Test

-- Runs the tests I made
runMyTests = Test.testall
