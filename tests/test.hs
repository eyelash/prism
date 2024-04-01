-- single line comment

{-
multi
line
comment
-}

import Control.Monad (forM_)

(<--) = (+)
(-->) = (+)

strings :: IO ()
strings = do
    return "hello"

    return ()

numbers :: IO ()
numbers = do
    -- integers
    return 1
    return 0x1
    return 0o1

    -- floating-point
    return 1.2
    return 1.2e3
    return 1e3
    return 1e+3
    return 1e-3

    return ()

controlFlow :: Bool -> IO ()
controlFlow b =
    if b then
        return ()
    else
        forM_ [0..9] (\i -> return ())

main :: IO ()
main = do
    return ()
