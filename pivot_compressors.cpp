// pivot_compressors.cpp
// By Ken Burchfiel
// Released under the MIT License

// This C++ file will provide definitions for at least
// one pivot table function that can help 'compress'
// a dataset into a smaller, easier-to-handle file.

#include "pivot_compressors.h"
#include "csv.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <numeric> // for std::accumulate
#include <string>

using namespace csv;

void scan_to_pivot(
    std::string &data_file_path, std::vector<std::string> &value_fields,
    std::string index_headers, long &rows_to_scan, std::string &pivot_file_path,
    std::function<std::string(CSVRow)> index_gen,
    std::map<std::string, std::vector<std::string>> &include_map,
    std::map<std::string, std::vector<std::string>> &exclude_map) {
  /*This function creates a pivot table by scanning through a
  .csv file (rather than importing all of it into your RAM), thus making
  it more feasible to process very large .csv files on computers with
  limited RAM.

  Note to self: when building out the version of this function that
  processes in-memory data saved as rows of structs,
  you would need one vector for your value names, and another vector
  that retrieves (via a lambda function wrapper) the values that
  correspond to those names. (Use a double as your type in order to
  make the function more flexible.) That should allow you to adapt this
  function to in-memory structs.

  rows_to_scan: A long integer that allows you to scan in only
  a subset of rows (which can be helpful for debugging/testing
  purposes). To scan all rows, set this variable to -1.

  value_fields: A vector containing all fields for which you would like
  to calculate count, sum, and mean data.

  index_gen(): A function that will determine, for each row, what
  values to use as the basis of the pivot index. The value of
  index_gen() will be defined via a lambda function
  whenever scan_to_pivot gets called. For more on the use of
  std::function, see the 'Lambdas with Function Wrapper'
  section of
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

  auto function_start_time = std::chrono::high_resolution_clock::now();

  // Initializing a CSVReader object that will allow us to
  // iterate through our .csv file, one row at a time:
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
  // each corresponding value will be an array of structs with the
  // same length as that of value_fields. That way, one struct
  // can be created, and easily accessed, for each value.)
  // Note: although it results in longer processing time,
  // I chose to use a regular map here, rather than an unordered
  // map, because I wanted the final output to be in alphabetical order.
  std::map<std::string, std::vector<Pivot_Vals>> pivot_map;

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
      for (auto const &[field, field_vals] : include_map) {
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
      for (auto const &[field, field_vals] : exclude_map) {
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
          if (pivot_map.contains(pivot_index_vals) == false)
          // We'll now add one Pivot_Vals object for each
          // value field to this map so that separate sum, mean,
          // and count values can get calculated for each of them.
          // I had tried to avoid this step by (1) using arrays
          // of Pivot_Vals objects as values within pivot_map and (2)
          // using the length of value_fields to specify the number
          // of Pivot_Vals objects to include. However, I encountered
          // issues with this approach, so I instead shifted to the
          // following approach, in which I use a loop to add the number
          // of necessary Pivot_Vals objects to the map. This check and
          // loop probably do slow down the function a bit.
          {
            for (const auto &x : value_fields) {
              Pivot_Vals pv{};
              pivot_map[pivot_index_vals].push_back(pv);
            }
          }
          // Updating the sum and count values within each value field's
          // correponding Pivot_Vals struct:
          for (int vfi = 0; vfi < value_fields.size(); vfi++)
          // vfi = 'value field index'
          {
            pivot_map[pivot_index_vals][vfi].pivot_sum +=
                row[value_fields[vfi]].get<double>();
            pivot_map[pivot_index_vals][vfi].pivot_count++;
          }
        }
      }
      scanned_rows++; // This number should be incremented regardless
      // of whether or not the current row qualified for inclusion
      // in the dataset.
    } else // In this case, we've scanned the requested
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
  // Adding pivot value fields to this row: (These will be prefaced
  // with value field names for easier identification.
  std::vector<std::string> value_aggregates{"Sum", "Count", "Mean"};

  for (const auto &value_field : value_fields) {
    for (std::string &aggregate : value_aggregates) {
      header_row.push_back(value_field + "_" + aggregate);
    }
  }

  row_writer << header_row;

  for (auto &[pivot_index, pivot_val_array] : pivot_map) {
    // Initializing a vector of strings that will store the
    // data for the current key/value pair as a copy of
    // the aggregate values for the current pivot_index entry:
    std::vector<std::string> pivot_row_vector{pivot_index};
    // Adding results (in string form) to this vector:
    for (int vfi = 0; vfi < value_fields.size(); vfi++) {
      // Calculating means for each value field:
      pivot_map[pivot_index][vfi].pivot_mean =
          pivot_map[pivot_index][vfi].pivot_sum /
          pivot_map[pivot_index][vfi].pivot_count;
      // std::cout << pivot_map[pivot_index].pivot_sum << " "
      // << pivot_map[pivot_index].pivot_count << " "
      // << pivot_map[pivot_index].pivot_mean << "\n";

      // Adding the sum, count, and mean aggregate values for this
      // particular value field to pivot_row_vector:
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][vfi].pivot_sum));
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][vfi].pivot_count));
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][vfi].pivot_mean));

      // for (const auto &field : pivot_row_vector)
      // {
      //     std::cout << field << "\t";
      // }
      // std::cout << "\n";
    }
    // Writing this completed row to a .csv file:
    row_writer << pivot_row_vector;
  }

  auto function_end_time = std::chrono::high_resolution_clock::now();
  auto function_run_time =
      std::chrono::duration<double>(function_end_time - function_start_time)
          .count();
  std::cout << "Finished processing the " << scanned_rows << "-row dataset in "
            << function_run_time << " seconds.\n";
}

std::map<std::string, std::map<std::string, Pivot_Vals>> in_memory_pivot(
    std::vector<std::map<std::string, 
    std::variant<std::string, double>>>
        &table_rows,
    std::vector<std::string> &index_fields,
    std::vector<std::string> &value_fields,
    bool save_to_csv,
    std::string &pivot_file_path,
    std::map<std::string, std::vector<std::string>> &string_include_map,
    std::map<std::string, std::vector<std::string>> &string_exclude_map,
    std::map<std::string, std::vector<double>> &double_include_map,
    std::map<std::string, std::vector<double>> &double_exclude_map)
/* This function is similar to scan_to_pivot() except that it processes
in-memory data rather than that from a .csv file. This approach allows for
faster processing time at the expense of RAM usage.

For more information on these arguments and on the code itself, 
see the corresponding documentation within scan_to_pivot().

table_rows: a vector of maps of (string, variant) pairs that stores data to
be pivoted.
(The advantage of this approach over a regular struct is that
it's more flexible, as you can use this same type for
multiple datasets with different field names. Another major
advantage is that the map approach allows values to be accessed
by string-based names, which helps make the output easier
to interpret and work with.)
(Another alternative would be to use std::string in place of
std::variants, then convert certain fields (e.g. value columns)
to a numeric type when needed. However, doing these conversions
ahead of time may save processing time if the table will get used
mulitple times.)
Although a regular vector (of strings or variants) could be used
in place of a map, having a field name assigned to each
value should assist with readability and make the code less error-prone.

index_fields and value_fields: vectors of pivot index and value
fields, respectively. The names in these vectors should match
the keys within the maps found within table_rows.

save_to_csv: Set to true to save the output of this script to a local
.csv file; set to false to skip this step. Either way, the output of 
the pivot table will be returned as a map.
*/
{
  auto function_start_time = std::
  chrono::high_resolution_clock::now();

  // The output of our pivot table will be stored as a map.
  // The keys of this map will be unique pivot index value combinations,
  // and the values themselves will be maps. The keys of these 'sub-maps'
  // will be pivot value names, and the values will be Pivot_Vals
  // objects. This approach will make it easier to link different
  // aggregate values to their corresponding value fields.
  std::map<std::string, std::map<std::string, Pivot_Vals>> pivot_map;

  for (int i = 0; i < table_rows.size(); i++)
  {
    std::map<std::string, std::variant<std::string, double>> row =
        table_rows[i];
    bool include_row = true;

    // Note that this function includes both string-based and 
    // double-based inclusion and exclusion maps so that certain
    // double-typed fields can also get excluded.
    for (auto const &[field, field_vals] : string_include_map) {

      if (std::ranges::contains(field_vals,
                                std::get<std::string>(row[field])) == false) {
        include_row = false; 
        break; 
      }
    }

    for (auto const &[field, field_vals] : string_exclude_map) {
      if (std::ranges::contains(field_vals,
                                std::get<std::string>(row[field]))) {
        include_row = false;
        break;
      }
    }

    for (auto const &[field, field_vals] : double_include_map) {

      if (std::ranges::contains(field_vals,
                                std::get<double>(row[field])) == false) {
        include_row = false; 
        break; 
      }
    }

    for (auto const &[field, field_vals] : double_exclude_map) {
      if (std::ranges::contains(field_vals,
                                std::get<double>(row[field]))) {
        include_row = false;
        break;
      }
    }


    if (include_row == true) {

      // Creating a grouped representation of all pivot index
      // values in the form of a string (with pipe separators
      // added in for easier readability):
      {
        std::string pivot_index_vals = "";
        for (int j = 0; j < index_fields.size();
             j++) { // The following code could be replaced with a lambda
          // function if needed/preferred.
          pivot_index_vals += std::get<std::string>(row[index_fields[j]]);
          // Adding a spacer between pivot index fields:
          if (j != (index_fields.size() - 1)) {
            pivot_index_vals += "|";
          }
        }
        // std::cout << pivot_index_vals << "\n";

        if (pivot_map.contains(pivot_index_vals) == false)
        {
          for (const auto &value_field : value_fields) {
            //Pivot_Vals pv{};
            pivot_map[pivot_index_vals][value_field] = Pivot_Vals {};
          }
        }
        // Updating the sum and count values within each value field's
        // correponding Pivot_Vals struct:
        for (const auto &value_field : value_fields)
        {
          pivot_map[pivot_index_vals][value_field].pivot_sum +=
              std::get<double>(row[value_field]);
          pivot_map[pivot_index_vals][value_field].pivot_count++;
        }
      }
    }
  }


  if (save_to_csv)
  {
  std::ofstream ofs_pivot(pivot_file_path);
  auto row_writer = make_csv_writer(ofs_pivot);

  std::vector<std::string> header_row;

  std::string pivot_index_header_field = "";
  for (int j = 0; j < index_fields.size();
       j++) {
    pivot_index_header_field += index_fields[j];
    if (j != (index_fields.size() - 1)) {
      pivot_index_header_field += "|";
    }
  }

  header_row.push_back(pivot_index_header_field);

  std::vector<std::string> value_aggregates{"Sum", "Count", "Mean"};

  for (const auto &value_field : value_fields) {
    for (std::string &aggregate : value_aggregates) {
      header_row.push_back(value_field + "_" + aggregate);
    }
  }

  row_writer << header_row;

  // for (const std::string &header_field : header_row) {
  //   std::cout << header_field << "\t";
  // }
  // std::cout << "\n";
  for (auto &[pivot_index, pivot_val_array] : pivot_map) {
    std::vector<std::string> pivot_row_vector{pivot_index};
    for (const auto &value_field : value_fields) {
      pivot_map[pivot_index][value_field].pivot_mean =
          pivot_map[pivot_index][value_field].pivot_sum /
          pivot_map[pivot_index][value_field].pivot_count;
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][value_field].pivot_sum));
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][value_field].pivot_count));
      pivot_row_vector.push_back(
          std::to_string(pivot_map[pivot_index][value_field].pivot_mean));
    }
    row_writer << pivot_row_vector;
  }
}

  auto function_end_time = std::chrono::high_resolution_clock::now();
  auto function_run_time =
      std::chrono::duration<double>(function_end_time - function_start_time)
          .count();
  std::cout << "Finished processing the " << table_rows.size() << "-row dataset in "
            << function_run_time << " seconds.\n";

return pivot_map;
}