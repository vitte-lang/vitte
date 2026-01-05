The `driver` crate is effectively the "main" function for the 
compiler. It orchestrates the compilation process and "knits together"
the code from the other crates within c. This crate itself does
not contain any of the "main logic" of the compiler (though it does
have some code related to pretty printing or other minor compiler
options).

For more information about how the driver works, see the [c dev guide].

[c dev guide]: https://c-dev-guide.-lang.org/c-driver/intro.html
