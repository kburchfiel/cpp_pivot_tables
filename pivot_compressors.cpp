// pivot_compressors.cpp
// By Ken Burchfiel
// Released under the MIT License

// This C++ file will provide definitions for at least
// one pivot table function that can help 'compress'
// a dataset into a smaller, easier-to-handle file.

#include "pivot_compressors.h"
#include "csv.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <numeric> // for std::accumulate
#include <string>
#include "pivot_compressors.h"

using namespace csv;

void scan_to_pivot(std::string &data_file_path, std::string &pivot_val,
                   std::string index_headers, long &rows_to_scan,
                   std::string &pivot_file_path,
                   std::function<std::string(CSVRow)> index_gen,
                   std::map<std::string, std::vector<std::string>>
                       &include_map,
                  std::map<std::string, std::vector<std::string>>
                       &exclude_map) {
  /*This function creates a pivot table by scanning through a
  .csv file (rather than importing all of it into your RAM).
  It thus helps limit the amount of RAM needed to create
  the final table.

  rows_to_scan: A long integer that allows you to scan in only
  a subset of rows (which can be helpful for debugging/testing
  purposes). To scan all rows, set this variable to -1.

  index_gen(): A function that will determine, for each row, what
  values to use as the basis of the pivot index. The value of 
  index_gen() will be defined via a lambda function
  whenever scan_to_pivot gets called. For more on the use of
  std::function, see
  https://www.geeksforgeeks.org/cpp/passing-a-function-as-
  a-parameter-in-cpp/ .

  For instance, if you wanted to use 'CARRIER' and 'ORIGIN' fields
  as your pivot index, you would pass the following argument to the
  pivot_gen parameter:

  [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      }

  This function will, for each row in the CSV file, generate a pivot_index
  string made up of that row's CARRIER and ORIGIN values, separated by a pipe.
  (There's no limit on the number of pipe-separated values you wish to pass
  within index_gen, though the more you include, the larger your final file
  will most likely be.)


  include_map and exclude_map: Maps that allow you to specify which
  field values for a given field to include or exclude, respectively.
  Within these maps, keys are field names, and values are all of the
  field values that you wish to include or exclude from your analysis.

  For example, if you wished to only include UA, AA, and DL flights
  that originated at JFK, LAX, ORD, MIA, or ATL within an analysis,
  you could pass the following to include_map:

    std::map<std::string, std::vector<std::string>> include_map {
      {"CARRIER", {"UA", "AA", "DL"}}, {
        "ORIGIN", {"JFK", "LAX", "ORD", "MIA", "ATL"}}};

  Similarly, if you wished to evaluate only international traffic by
  excluding all flights with a destination within the US, you could
  pass the following to exclude_map:

  std::map<std::string, std::vector<std::string>> exclude_map{
      {"DEST_COUNTRY", {"US"}}};




  (More documentation to come)
  */

  // Importing data:
  /* This code was based on the example found at:
  https://github.com/vincentlaucsb/csv-parser?
  tab=readme-ov-file#reading-an-arbitrarily-large-file-with-iterators */ 
  CSVReader reader(data_file_path);


  // Initializing a struct that can store results for each
  // pivot index combination:
  // Note that each value will be initialized as 0.0 (or, for
  // long integers, 0) in order
  // to help ensure that our final output is correct. (I'm not sure
  // that these initial values are strictly necessary to specify,
  // but it shouldn't hurt to do so.)

  // Creating a map that can be used to store values for our pivot
  // table calculations:
  // (Each key will be a unique combination of pivot_index values;
  // each corresponding value will be a vector that stores
  // sum and count information for our metric of interest.

  struct Pivot_Vals {
    double pivot_sum{0.0};
    long pivot_count{0};
    double pivot_mean{0.0};
  };

  std::unordered_map<std::string, Pivot_Vals> pivot_map;
  //  {0.0, 0.0}

  long scanned_rows = 0;
  for (CSVRow &row : reader) {
    bool include_row = true;
    if ((scanned_rows < rows_to_scan) || (rows_to_scan == -1))
    // Checking, based on include_map and exclude_map,
    // whether this particular row should be included in our output:
    {
    // Iterating through our map of values to include for each field:
    // This code was based in part on P0W's response at
    // https://stackoverflow.com/a/26282004/13097194 .
      for (auto const& [field, field_vals]: include_map) 
      {
        // Checking to see whether the row's value is present within
        // our list of field values:
        if (std::ranges::contains(field_vals, row[field].get()) == false) {
          include_row = false; // This row will now be skipped.
          break; // There's no need to go through any other key/value
          // pairs within this map now that we know that we won't be 
          // using this row.
        }
      }
      // Performing similar steps for exclude_map: (The only difference
      // is that we'll now set include_row to false if we *do* encounter
      // a given value within field_vals.)
      for (auto const& [field, field_vals]: exclude_map) 
      {
        if (std::ranges::contains(field_vals, row[field].get())) {
          include_row = false;
          break;
        }
      }
    if (include_row == true) {
      {
        std::string pivot_index_vals = index_gen(row);
        // Adding the values corresponding to this set
        // of index variables to our map:
        // (If an entry for this index variable set
        // doesn't already exist, the map will create one
        // by default--which is very convenient.)
        pivot_map[pivot_index_vals].pivot_sum += row[pivot_val].get<double>();
        pivot_map[pivot_index_vals].pivot_count++;
      }
    }
    scanned_rows++;
  }
  else // In this case, we've scanned the requested
       // number of rows and can thus exit the loop early.
  {
    break;
  }
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

std::vector<std::string> header_row{index_headers};
// Adding pivot value fields to this row:
std::vector<std::string> value_fields{"Sum", "Count", "Mean"};
for (std::string &field : value_fields) {
  header_row.push_back(field);
}

row_writer << header_row;


for (auto &[pivot_index, pivot_vals] : pivot_map) {
  // Calculating means:
  pivot_map[pivot_index].pivot_mean =
      pivot_map[pivot_index].pivot_sum / pivot_map[pivot_index].pivot_count;
  // std::cout << pivot_map[pivot_index].pivot_sum << " "
  // << pivot_map[pivot_index].pivot_count << " "
  // << pivot_map[pivot_index].pivot_mean << "\n";

  // Initializing a vector of strings that will store the
  // data for the current key/value pair as a copy of
  // the pivot_index data:
  std::vector<std::string> pivot_row_vector{pivot_index};
  // Adding results (in string form) to this vector:

  pivot_row_vector.push_back(std::to_string(pivot_map[pivot_index].pivot_sum));
  pivot_row_vector.push_back(
      std::to_string(pivot_map[pivot_index].pivot_count));
  pivot_row_vector.push_back(std::to_string(pivot_map[pivot_index].pivot_mean));

  // for (const auto &field : pivot_row_vector)
  // {
  //     std::cout << field << "\t";
  // }
  // std::cout << "\n";

  row_writer << pivot_row_vector;
}
}