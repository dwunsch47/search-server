# **Search Server**

Simple search server with ranking system

# **Design**


## `SearchServer`
- `SearchServer` constructor takes a string-type containter containing stop words which are ignored during "Document" parsing
"Document" is an internal data type used to store files for later search through


## `AddDocument`
- `AddDocument` takes documnet id, document string itself, status (ACTUAL, IRRELEVANT, BANNED, REMOVED), and vector of ratings


## `FindTopDocuments`
- Search is initialized by `FindTopDocuments`. It takes std::string_view of search request with optional fields such as Document status and/or execution policy.
- It supports either sequancial (default) or parallel execution.

- It calls `FindAllDocuments` and sorts its results according to ratings


## `FindAllDocuments`
- Actual search (without rating sorting) is done by `FindAllDocuments`. It utilises self-made ConcurrentMap to store results and allow parallel search
- It outputs unsorted vector of documents with appropriate rating and relevance

# **Usage**
- Min. C++ Version: C++17

- Clone repository and compile. `main.cpp` containts an example of how to use it
