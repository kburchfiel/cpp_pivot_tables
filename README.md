# C++ Pivot Tables

By Ken Burchfiel

Released under the MIT License

This project demonstrates how to use C++, in conjunction with Vince La's CSV parser library, to create pivot tables of data. The two pviot table functions provided here (whose definitions are available in cpp_pivot_tables.cpp) compute mean, sum, and count data for an arbitrary number of index and value variables.

The first function within cpp_pivot_tables.cpp, scan_to_pivot(), allows pivot table data to get calculated one CSV row at a time. This makes it more feasible to calculate summary statistics for very large dataset on computers with limited memory. The second function, in_memory_pivot(), works with in-memory table data. Both functions allow pivot table output to get stored as a .csv file, but this is optional for in_memory_pivot().

I hope to provide more documentation on these functions in the future, but I plan to attend to some other C++ projects first.

NOTE: I have not extensively tested these functions; as a result, please use them at your own risk, especially if your tables have missing data!