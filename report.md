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

## Requirements affected by functionality being refactored


## Existing test cases relating to refactored code
All of the test cases that utilize CppUTest (which should be all)
will be affected by our refactoring.

We knew that there was no possibility for us to refactor all of the tests, so instead we focused on the test file framework/private/test/filter_test.cpp.

## The refactoring carried out
There is no need for a UML diagram since this does not change any of the code, only the library that the code interfaces with.

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
Discussions within git are approximate
Johan 1h
Nikhil 1h
3.  reading documentation;
Johan 3h
4.  configuration;
Johan 2h
5.  analyzing code/output;
Johan 2h
6.  writing documentation;
Johan 2h
7.  writing code;
Johan 2h
8.  running code?
Johan 1h
## Overall experience
What are your main take-aways from this project? What did you learn?
Is there something special you want to mention here?
