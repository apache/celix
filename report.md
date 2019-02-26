# Report for assignment 4
Members:
Johan Johan Sjölén
Nikhil Modak
Jagan Moorthy

## Project
Name: Apache Celix
URL: https://celix.apache.org/
The Apache Celix project implements the OSGi specification for the C
and C++ languages.
OSGi is a specification for modular and dynamic
components first implemented for Java.

## Selected issue(s)
Title: Refactor CppUTest tests to gtest
URL: https://issues.apache.org/jira/projects/CELIX/issues/CELIX-447?filter=allopenissues

The issue is about moving the project over from CppUTest to GTest+GMock as the chosen testing platform.
The reasons for why are outlined in the issue (ease of integration, heavier usage in the community as a whole, certain IDE support).

## Onboarding experience

The building of the project was fine. Installing dependencies and so on was all described in sufficient detail.
However it was very difficult to run the tests of the project.
To succeed in running the tests we had to read several CMake files to find the required flags and in the end
we had to actually change the CMake files ourselves to get the tests to run (we have an issue about this in our Github with more details, see issue #2).
Many links in the repo lead to 404s, which was also annoying. There was a certain amount of documentation however, which helped.
The main communication channel was through a mailing list. These are usually more well-suited for long-term discussion and since our team 
did not have a lot of experience using one we did not end up contacting the team for support.

## Existing test cases relating to refactored code
All of the test cases that utilize CppUTest (which should be all) will be affected by our refactoring.

We knew that there was no possibility for us to refactor all of the tests, so instead we focused on the test file framework/private/test/filter_test.cpp.

## A test log
This is the run of the test log before the refactorings took place. Since the refactorings are about changing the tests this is really
not something useful, but it does prove that we ran the tests.

```
Running tests...
Test project /home/johan/src/celix/build
      Start  1: run_test_dfi
 1/19 Test  #1: run_test_dfi ......................   Passed    0.01 sec
      Start  2: attribute_test
 2/19 Test  #2: attribute_test ....................   Passed    0.00 sec
      Start  3: bundle_cache_test
 3/19 Test  #3: bundle_cache_test .................   Passed    0.00 sec
      Start  4: bundle_context_test
 4/19 Test  #4: bundle_context_test ...............   Passed    0.00 sec
      Start  5: bundle_revision_test
 5/19 Test  #5: bundle_revision_test ..............   Passed    0.00 sec
      Start  6: bundle_test
 6/19 Test  #6: bundle_test .......................   Passed    0.00 sec
      Start  7: capability_test
 7/19 Test  #7: capability_test ...................   Passed    0.00 sec
      Start  8: celix_errorcodes_test
 8/19 Test  #8: celix_errorcodes_test .............   Passed    0.00 sec
      Start  9: filter_test
 9/19 Test  #9: filter_test .......................   Passed    0.00 sec
      Start 10: framework_test
10/19 Test #10: framework_test ....................   Passed    0.00 sec
      Start 11: manifest_parser_test
11/19 Test #11: manifest_parser_test ..............   Passed    0.00 sec
      Start 12: manifest_test
12/19 Test #12: manifest_test .....................   Passed    0.00 sec
      Start 13: requirement_test
13/19 Test #13: requirement_test ..................   Passed    0.00 sec
      Start 14: service_reference_test
14/19 Test #14: service_reference_test ............   Passed    0.00 sec
      Start 15: service_registration_test
15/19 Test #15: service_registration_test .........   Passed    0.00 sec
      Start 16: service_registry_test
16/19 Test #16: service_registry_test .............   Passed    0.00 sec
      Start 17: service_tracker_customizer_test
17/19 Test #17: service_tracker_customizer_test ...   Passed    0.00 sec
      Start 18: wire_test
18/19 Test #18: wire_test .........................   Passed    0.00 sec
      Start 19: run_test_framework
19/19 Test #19: run_test_framework ................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 19

Total Test time (real) =   0.07 sec
```


## The refactoring carried out
There is no need for a UML diagram since this does not change any of the code, only the library that the code interfaces with.

The main refactoring of the code is done in the CMake files.

## Requirements

Requirements related to the functionality targeted by the refactoring are identified and described in a systematic way. Each requirement has a name (ID), title, and description, and other optional attributes (in case of P+, see below). The description can be one paragraph per requirement.

1. gtest-import Import Gtest into the project

First of all GTest needs to be imported such that it can be utilized for the tests. This will include
installing it on your personal system (this is a slight amount of work because only the headers are available in the Ubuntu package manager).


2. gmock-import Import GMock into the project

Now install GMock and import it into the project, same issues are current here.


3. test-refactor Refactor at least one test file into using GTest+GMock

Pick out a suitable testfile which at least does mocking and re-implement it with Gtest+Gmock

4. make-test Set-up `make test`

Running the tests should be done using `make test` just like it is done now for CppUTest


## What we've done

We did not finish the task. The main road block was in the project structure.
We did however:

1. Succesfully import GTest into the project
2. Succesfully import GMock into the project
3. Start a conversion of a test file.


## Test logs
Overall results with link to a copy of the logs (before/after 
refactoring).
The refactoring itself is documented by the git log.
## Effort spent
For each team member, how much time was spent in
1.  plenary discussions/meetings;
Johan 1h
Nikhil 1h
Jagan 1h
2.  discussions within parts of the group;
Discussions within Github and Messenger (time is an approximation of course)
Johan 1h
Nikhil 1h
Jagan 1h
3.  reading documentation;
Johan 3h
4.  configuration;
Johan 2h
5.  analyzing code/output;
Johan 2h
6.  writing documentation;
Johan 4h
7.  writing code;
Johan 2h
8.  running code?
Johan 1h

## Overall experience

This was our first assignment whose project used C/C++ as its main development languages, that turned out to be a major pain point.
Because of the different tooling (CMake/make contra Gradle or Maven) we had some initial issues setting things up for development.
The assignment also reinforced our belief that the initial install/build/test instructions of a project needs to be there and they need to be as
simple and painless as possible, at least for hobbyists. It's probably less of an issue if a software engineer takes some time setting up a project,
however if a team needs to evaluate several projects and pick one to use then we can see issues with building one project will put it behind the others.

Overall this went OK, a lot of what we did are hacks because we struggled with the project because of a lack of documentation and lack of communication channels (none of us are very experienced with mailing lists).
