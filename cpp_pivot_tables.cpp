/* Producing pivot tables within C++
(A work in progress)
By Kenneth Burchfiel
Released under the MIT License

(Note: I am grateful to Carter Skeel and Wendy Wang at the 
Institute for Family Studies for their openness to my making
this project open source. This code was written in my free time
but incorporates ideas from my work.)

This project shows how to create simple pivot tables within C++
via a custom function.

This project also makes extensive use of Vincent La's CSV Parser
(which, like this project, has been released under the MIT license.)
The following links proved especially helpful in drafting the
CSV import and export code in this project:
https://github.com/vincentlaucsb/csv-parser
https://vincela.com/csv/classcsv_1_1CSVRow.html 

# To dos:

1. Consider using tuples rather than strings for your index values
(as you haven't tried out that approach yet.)

1. Show how pivot tables can be created via data already stored in
memory as vectors of classes (which will be more realistic than,
say, vectors of maps in which every value has the same type).

2. Add in more documentation.

3. Add in timing-related code.

4. Consider making the examples more interesting/relevant.
(For instance, consider adding in code that will allow you to
show passenger-mile or passenger totals rather than just 
distance totals. In addition, consider multiplying distance
by departures so that you arrive at a more accurate/relevant 
distance total.)

5. Download data from other years also so that you end up with
a very large file; that will make it easier to determine how
well your program limits memory usage.

6. Add in a Python-based comparison program.

7. Consider creating an equivalent setup using the hmdf library.
(You may want to create a condensed version of the 
file that only includes the fields that you'll be using.)

Source for aviation data:
Air Carriers : T-100 Segment (All Carriers) table
from the Bureau of Transportation Statistics
https://www.transtats.bts.gov/DL_SelectFields.aspx?gnoyr_VQ=FMG&QO_
fu146_anzr=Nv4+Pn44vr45

*/

#include <iostream>
#include "csv.hpp"
#include <string>
#include <map>
#include <numeric> // for std::accumulate
#include <chrono>
#include <functional>

using namespace csv;


void scan_to_pivot(std::string& data_file_path, 
std::string& pivot_val, std::string index_headers,
long& rows_to_scan, std::string& pivot_file_path,
std::function<std::string(CSVRow)> index_gen)
{
/*This function creates a pivot table by scanning through a 
.csv file (rather than importing all of it into your RAM).
It thus helps limit the amount of RAM needed to create
the final table.

rows_to_scan: A long integer that allows you to scan in only
a subset of rows (which can be helpful for debugging/testing 
purposes). To scan all rows, set rows_to_scan to -1.


The value of index_gen() will be defined via a lambda function
whenever scan_to_pivot gets called. For more on the use of
std::function, see
https://www.geeksforgeeks.org/cpp/passing-a-function-as-
a-parameter-in-cpp/ .

(More documentation to come)
*/

    // Full 2024 file: (stored separately due to its size)
    CSVReader reader(data_file_path);

    // In this section of the project, we'll calculate the average
    // distance of airline flights by carrier, origin airport, and
    // a region code.

    // Importing data:
    /* This code was based on the example found at:
    https://github.com/vincentlaucsb/csv-parser?
    tab=readme-ov-file#reading-an-arbitrarily-large-file-with-iterators */

    // Creating a map that can be used to store values for our pivot
    // table calculations:
    // (Each key will be a unique combination of pivot_index values;
    // each corresponding value will be a vector that stores
    // sum and count information for our metric of interest
    // (in this case, distance).
    // We'll use this information to calculate average distances
    // later within the script.

    // Initializing a struct that can store results for each
    // pivot index combination:
    // Note that each value will be initialized as 0.0 (or, for 
    // long integers, 0) in order
    // to help ensure that our final output is correct. (I'm not sure
    // that these initial values are strictly necessary to specify, 
    // but it doesn't hurt to do so.)

    struct Pivot_Vals
    {
        double pivot_sum {0.0};
        long pivot_count {0};
        double pivot_mean {0.0};
    };

    std::unordered_map<std::string, Pivot_Vals> pivot_map;
    //  {0.0, 0.0}

    long scanned_rows = 0;
    for (CSVRow &row : reader)
    {

        if ((scanned_rows < rows_to_scan) || (rows_to_scan == -1))
        {   std::string pivot_index_vals = index_gen(row);
            // Adding the values corresponding to this set
            // of index variables to our map:
            // (If an entry for this index variable set
            // doesn't already exist, the map will create one
            // by default--which is very convenient.)
            pivot_map[pivot_index_vals].pivot_sum += row[
                pivot_val].get<double>();
            pivot_map[pivot_index_vals].pivot_count++;
        }
        else // In this case, we've scanned the requested
        // number of rows and can thus exit the loop early.
        {
            break;
        }
        scanned_rows++;
    }

    // Calculating means within pivot table, then writing
    // the table's output to a .csv file:
    // This will involve converting each row of our pivot table values
    // into a vector of strings, then exporting that vector to a file
    // via Vince La's csv-parser library.
    // This export will take place on a row-by-row basis, thus
    // preventing us from having to loop through our map twice
    // (once to calculate our means and again to export the table).

    /* The following code was based on
    https://github.com/vincentlaucsb/csv-parser?
    tab=readme-ov-file#writing-csv-files
    and
    https://en.cppreference.com/w/cpp/io/basic_ofstream.html . */
    std::ofstream ofs_pivot(pivot_file_path);
    auto row_writer = make_csv_writer(ofs_pivot);

    // Writing a header row for the .csv file:

    std::vector<std::string> header_row {index_headers};
    // Adding pivot value fields to this row:
    std::vector<std::string> value_fields{"Sum",
                                          "Count", "Mean"};
    for (std::string& field : value_fields)
    {
        header_row.push_back(field);
    }

    row_writer << header_row;

        // Iterating through the dictionary:
        // This code was based in part on P0W's response at
        // https://stackoverflow.com/a/26282004/13097194 .
        for (auto &[pivot_index, pivot_vals] : pivot_map)
    {
        // Calculating means:
        pivot_map[pivot_index].pivot_mean = pivot_map[
            pivot_index].pivot_sum / pivot_map[
                pivot_index].pivot_count;
        // std::cout << pivot_map[pivot_index].pivot_sum << " "
        // << pivot_map[pivot_index].pivot_count << " "
        // << pivot_map[pivot_index].pivot_mean << "\n";

        // Initializing a vector of strings that will store the
        // data for the current key/value pair as a copy of
        // the pivot_index data:
        std::vector<std::string> pivot_row_vector {pivot_index};
        // Adding results (in string form) to this vector:

        pivot_row_vector.push_back(
            std::to_string(pivot_map[pivot_index].pivot_sum));
        pivot_row_vector.push_back(
            std::to_string(pivot_map[pivot_index].pivot_count));
        pivot_row_vector.push_back(
            std::to_string(pivot_map[pivot_index].pivot_mean));

        // for (const auto &field : pivot_row_vector)
        // {
        //     std::cout << field << "\t";
        // }
        //std::cout << "\n";

        row_writer << pivot_row_vector;
    }

}

int main()
{auto program_start_time = std::chrono::high_resolution_clock::now();
std::string data_file_path {"/home/kjb3/D1V1/Documents/\
Large_Datasets/BTS/T_T100_SEGMENT_ALL_CARRIER_2024.csv"};
std::string index_headers {"CARRIER|ORIGIN|REGION"};
std::string pivot_val {"DISTANCE"};
long rows_to_scan {-1}; // Use -1 to scan all rows
std::string pivot_file_path {
"../mean_dist_by_carrier_origin_region.csv"};

// Calling scan_to_pivot to calculate average distance values
// by carrier, origin, and region:
scan_to_pivot(data_file_path, pivot_val, index_headers,
rows_to_scan, pivot_file_path, [&](
    CSVRow row) -> std::string {
    return {row["CARRIER"].get() + "|" + row["ORIGIN"].get() + "|" + 
    row["REGION"].get()};});

// Updating several variables, then calling scan_to_pivot a 
// second time to calculate mean distance values by 
// carrier and origin only:

pivot_file_path = "../mean_dist_by_carrier_origin.csv";
index_headers = "CARRIER|ORIGIN";

scan_to_pivot(data_file_path, pivot_val, index_headers,
rows_to_scan, pivot_file_path, [&](
    CSVRow row) -> std::string {
    return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};});

// The following code is useful for checking the program's
// RAM usage after all other code has been executed.
// std::cout << "Press any key to exit.";
// char key;
// std::cin >> key;

auto program_end_time = std::chrono::high_resolution_clock::now();
auto program_run_time = std::chrono::duration<double>(
    program_end_time - program_start_time).count();
std::cout << "The program finished running after "
<< program_run_time << " seconds.\n";
}

// Runtime calcs:
// (These all use the original, non-truncated versions of the 
// datasets.)
// Original approach (prior to implementing lambda function for
// developing pivot index): 7.44317 seconds (1st run); 7.32014
// seconds (second run); 7.24371 seconds (3rd run).

// New approach (that uses lambda function for creating vector-
// based pivot index: 7.02943 seconds (1st run);
// 7.06246 seconds (2nd run); 7.09754 seconds (3rd run)

// New approach (that uses strings rather than vectors to store
// data): 5.91007 seconds; 5.88203 seconds; and 5.97761 seconds.

// New approach (that uses unordered rather than ordered maps):
// 5.87901 seconds; 5.92743 seconds; and 5.84311 seconds.