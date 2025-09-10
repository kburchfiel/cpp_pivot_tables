# C++ Pivot Tables

By Ken Burchfiel

Released under the MIT License

This project demonstrates how to use C++, in conjunction with [Vince La's CSV parser library](https://github.com/vincentlaucsb/csv-parser), to create pivot tables of data. The two pivot table functions provided here (whose definitions are available in [pivot_compressors.cpp](https://github.com/kburchfiel/cpp_pivot_tables/blob/main/pivot_compressors.cpp)) compute mean, sum, and count data for an arbitrary number of index and value variables.

The first function within pivot_compressors.cpp, `scan_to_pivot()`, allows aggregate count and sum values to get calculated one CSV row at a time. This makes it more feasible to produce pivot tables for very large datasets on computers with limited memory. The second function, `in_memory_pivot()`, works with in-memory table data. Both functions allow pivot table output to get stored as a .csv file, but this is optional for `in_memory_pivot()`.

The pivot_compressors.cpp file provides more documentation on these functions; in addition, usage examples are available within [cpp_pivot_tables.cpp](https://github.com/kburchfiel/cpp_pivot_tables/blob/main/cpp_pivot_tables.cpp). I may add additional documentation to this project in the future, but I would like to attend to some other C++ projects first.

NOTE: I have not extensively tested these functions; as a result, please use them at your own risk, especially if your tables have missing data!
