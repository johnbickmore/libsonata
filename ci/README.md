Continuous Integration
**********************

Scripts related to continuous integration:

* cpp_test.sh: c++ build and tests
* python_test.sh: python build and tests
* check_clang_format.sh: check format of C++ code in last check-in, 
    will reformat it if it doesn't satisfy the style guide
* coverage_test.sh: run the unittest with coverage on, output a text coverage
  report, and lcov compatible outputs
