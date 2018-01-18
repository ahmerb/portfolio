module ParseProc where

-- Import Prelude with Num typeclass as qualified
import Prelude hiding (Num)
import qualified Prelude (Num)

-- Import Megaparsec
import Control.Monad (void)
import Text.Megaparsec hiding (parse)
import qualified Text.Megaparsec as M (parse)
import Text.Megaparsec.Expr
import Text.Megaparsec.String -- input stream is of type String
import qualified Text.Megaparsec.Lexer as L

-- Import syntactic types and AST for proc
import SyntaxProc

---
-- Wrapper function
---
parse :: String -> Stm
parse tks = stm
  where Right stm = M.parse parseProc "" tks


---
-- Lexemes
-- parsers that consume comments, whitespace, symbols, parens, literals, semicolons, operators, Var names and reserved words
---


-- space consumer
-- spaces takes three args: a parser for single whitespace char, are parser for line comments and a parser
-- for block comments
sc :: Parser ()
sc = L.space (void spaceChar) lineCmnt blockCmnt
  where lineCmnt  = L.skipLineComment "//"
        blockCmnt = L.skipBlockComment "/*" "*/"

-- lexeme parser
-- takes a parser and creates a parser that will consume any trailing whitespace
lexeme :: Parser a -> Parser a
lexeme = L.lexeme sc

-- symbol parser
-- takes a string and creates a parser that parses this fixed string
symbol :: String -> Parser String
symbol = L.symbol sc

-- parenthesis parser
-- parses something inbetween parenthesis
parens :: Parser a -> Parser a
parens = between (symbol "(") (symbol ")")

-- parenthesis parser
-- parses something that can be inbetween parenthesis
parensM :: Parser a -> Parser a
parensM p = try (parens p) <|> p

-- Num parser
-- parses a Num
num :: Parser Num
num = lexeme L.integer

-- semicolon parser
-- parses a semicolon
semi :: Parser String
semi = symbol ";"

-- On parsing reserved words, we need to first check that the parsed reserved word is not a prefix of a Var
-- On parsing Vars, we need to first check that the Var is not a reserved word

-- Define a list of reserved words
reservedWords :: [String]
reservedWords = [ "if", "then", "else", "while", "do", "skip" --control flow
                , "begin", "var", "end", "proc", "is", "call" --blocks and procs
                , "true", "false"                             --literals
                ]

-- reserved word parser
rword :: String -> Parser ()
rword w = string w *> notFollowedBy alphaNumChar *> sc

-- Var parser
var :: Parser Var
var = (lexeme . try) (p >>= check)
  where p       = (:) <$> letterChar <*> many alphaNumChar
        check x = if x `elem` reservedWords
                    then fail $ "keyword " ++ show x ++ " cannot be a variable name"
                    else return x


---
-- Statements
-- parses that product Stm nodes
---


-- Top level of our AST
-- Stm is our root level node. Wrap stm parser in `between sc eof`
-- so we can consume leading whitespace and consume trailing EOF.
parseProc :: Parser Stm
parseProc = between sc eof stm

-- Stm Parser
stm :: Parser Stm
stm = try stmCompLeft <|> parens stm <|> try stmComp <|> stm'

-- statement comp can be forced left assoc using parens
stmCompLeft :: Parser Stm
stmCompLeft = do
  stm1 <- parens stm
  void semi;
  stm2 <- stm
  return (Comp stm1 stm2)

stmComp :: Parser Stm
stmComp = do
  stm1 <- stm'
  void semi
  stm2 <- stm --statement composition is right associative
  return (Comp stm1 stm2)

stm' :: Parser Stm
stm' = sc *> (stmWhile <|> stmIf <|> stmSkip <|> stmAss <|> stmBlock <|> stmCall)

stmWhile :: Parser Stm
stmWhile = do
  rword "while"
  cond <- bexp
  rword "do"
  stm1 <- stm
  return (While cond stm1)

stmIf :: Parser Stm
stmIf = do
  rword "if"
  cond <- bexp
  rword "then"
  stmTrue <- stm
  rword "else"
  stmFalse <- stm
  return (If cond stmTrue stmFalse)

stmSkip :: Parser Stm
stmSkip = do
  rword "skip"
  return (Skip)

stmAss :: Parser Stm
stmAss = do
  varName <- var
  void (symbol ":=")
  expr <- aexp
  return (Ass varName expr)


---
-- Blocks and Procedures
---


-- Parse a block statement
stmBlock :: Parser Stm
stmBlock = do
  rword "begin"
  decV <- parensM dvars  --variable declarations
  decP <- parensM dprocs --procedure declarations
  body <- stm            --block body
  rword "end"
  return (Block decV decP body)

dvars :: Parser DecV
dvars = many $ parensM $ do
  rword "var"
  varName <- var
  void (symbol ":=")
  expr <- aexp
  void semi
  return (varName, expr)

dprocs :: Parser DecP
dprocs = many $ parensM $ do
  rword "proc"
  procName <- pname
  rword "is"
  body <- parens stm <|> stm' -- body must be singular stm, or within parens
  void semi
  return (procName, body)

pname :: Parser Pname
pname = var

-- Parse a call statement
stmCall :: Parser Stm
stmCall = do
  rword "call"
  procName <- pname
  return (Call procName)


---
-- Expressions
-- parsers that produce Aexp and Bexp nodes.
-- we defined the operator precedence to be the order they appear in the AST.
-- operators are left associative.
---


-- Parse an Aexp
aexp :: Parser Aexp
aexp = makeExprParser aTerm aOps

aTerm :: Parser Aexp
aTerm = parens aexp
  <|> V <$> var
  <|> N <$> num

aOps :: [[Operator Parser Aexp]]
aOps = [ [ InfixL (Mult <$ symbol "*") ]
       , [ InfixL (Add  <$ symbol "+")
         , InfixL (Sub  <$ symbol "-") ]
       ] -- Mult is higher precedence than Add and Sub


-- Parse a Bexp
-- we also define a parser rexp, to parse relational expressions, which are bexp's formed using aexp's.
bexp :: Parser Bexp
bexp = makeExprParser bTerm bOps

bTerm :: Parser Bexp
bTerm = parens bexp
  <|> (rword "true"  *> pure (TRUE))
  <|> (rword "false" *> pure (FALSE))
  <|> (rexp)

rexp :: Parser Bexp
rexp = do
  a1 <- aexp
  op <- relation
  a2 <- aexp
  return (op a1 a2)

relation :: Parser (Aexp -> Aexp -> Bexp)
relation = (symbol "=" *> pure Eq) <|> (symbol "<=" *> pure Le)

bOps :: [[Operator Parser Bexp]]
bOps = [ [ Prefix (Neg <$ symbol "!") ]
       , [ InfixL (And <$ symbol "&") ]
       ] -- Not is higher precedence than And
