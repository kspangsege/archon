Bugs found in Doxygen
=====================

Bug tracker: https://github.com/doxygen/doxygen/issues

- Weird compile time errors when enabling clang parsing and specifying a directory among source files. Improve explanation      
- Classes nested in private part of other classes are included, at least when they are defined outside the class. Also true when enabling Clang parsing. They should have been excluded. Should respect config param EXTRACT_PRIVATE. Improve explanation. Example: archon::core::BasicStringTemplate<S, A>::Substitution (Doxygen 1.8.17)
- Cannot refer to other namespace level function in same header file. Improve explanation. Example        
- Failure to pass function signature in reference (\ref) when it contains parameter packs `...`. Improve explanation. Example        

Missing features:

- Specification of minimum required doxygen version in Doxyfile.
