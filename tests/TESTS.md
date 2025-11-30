# Test Suite Documentation
## Overview
This is a lightweight test suite that provides a simple way to verify code.

## Running Tests
To run all tests:
```
make run
```
To just build the test_runner:
```
make build-tests
```
Building all the tests without running them is useful if you want to pass specific parameters into the test_runner.

## Adding New Tests

1. Make a new file for the component you are testing if it does not already exist. Name the file module_test.cc and include the tests.h header at the top. See the sha256_test.cc file for an example.
2. Declare all of your tests in the tests.h file and make sure it has its own section per cc file. Make sure to follow the pattern bellow:
```
bool DescriptiveTestName();
```
3. Implement the actual test.
4. Register the test in test_runner.cc:
```
Run(DescriptiveTestName);
```

