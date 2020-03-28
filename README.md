# jthread
C++ class for a joining and cooperative interruptible thread (should become std::jthread) with stop_token helper

Draft implementation of the C++ paper P0660
  https://wg21.link/p0660

Authors:  Nicolai Josuttis (http://www.josuttis.com/contact.html) and Lewis Baker

The code is licensed under a Creative Commons Attribution 4.0 International License 
(http://creativecommons.org/licenses/by/4.0/).

TOC:
====

<b>source/</b>
- source code for the implementation
  and the test suite
  - to test the CV class extensions they are applied to a class condition_variable2

<b>tex/</b>
- final paper and proposed wording using C++ standard LaTeX style
  - to create the latest version:  <b>pdflatex std</b> 

<b>doc/</b>
- current and old documentations
  - currently mainly the proposals P0660*.pdf

