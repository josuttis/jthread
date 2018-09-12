# jthread
C++ class for a joining and cooperative interruptible thread (should become std::jthread) 

Draft implementation of the C++ paper P0660
  https://wg21.link/p0660

Author:  Nicolai Josuttis
Contact: http://www.josuttis.de/contact.html

The code examples are licensed under a Creative Commons Attribution 4.0 International License. 
  http://creativecommons.org/licenses/by-sa/4.0/

TOC:
====

source/
- source code for the implementation
  and the test suite
  - to test the CV class extensions they are applied to a class condition_variable2

tex/
- final paper and proposed wording using C++ standard LaTeX style
  - to create the latest version:  pdflatex std 

doc/
- current and old documentations
  - mainly currently the proposals P0660*.pdf

