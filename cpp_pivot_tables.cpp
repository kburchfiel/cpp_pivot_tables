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

1. Continue updating your in-memory function; for instance, consider 
adding in double-based include/exclude maps. In addition, allow the user
to specify whether or not to print the output to a .csv file; have the
function return its pivot map (with strings as keys and vectors of 
value-field structs) either way. (You may also want to find a way to return
the header row for easier interpretability; you could do so by returning a
pair, a map, or even a struct.)

2. Once this code is more or less finalized, move it over to pivot_compressors.csv
and convert it into a function.

3. Add in more documentation.

4. Download data from other years also so that you end up with
a very large file; that will make it easier to determine how
well your program limits memory usage.

5. Add in a Python-based comparison program.

6. Consider creating an equivalent setup using the hmdf library.
(You may want to create a condensed version of the
file that only includes the fields that you'll be using.)

Source for aviation data:
Air Carriers : T-100 Segment (All Carriers) table
from the Bureau of Transportation Statistics
https://www.transtats.bts.gov/DL_SelectFields.aspx?gnoyr_VQ=FMG&QO_
fu146_anzr=Nv4+Pn44vr45

*/

#include "csv.hpp"
#include "pivot_compressors.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using namespace csv;

int main() {
  auto program_start_time = std::chrono::high_resolution_clock::now();
  std::string data_file_path{"/home/kjb3/D1V1/Documents/\
Large_Datasets/BTS/T_T100_SEGMENT_ALL_CARRIER_2024.csv"};
  std::string index_headers{"CARRIER|ORIGIN|REGION"};
  std::vector<std::string> value_fields{"PASSENGERS", "SEATS",
                                        "DEPARTURES_PERFORMED"};
  long rows_to_scan{100}; // Use -1 to scan all rows

  std::string pivot_file_path{
      "../pax_seats_deps_by_carrier_origin_region_filtered.csv"};

  // Defining maps that will allow us to determine which values to include
  // or exclude for various fields:

  // std::vector<std::map<std::string, std::vector<std::string>>> include_map{
  //     {{"CARRIER", {"UA", "AA", "DL"}}}};

  // The keys within include_map are fields; their corresponding values
  // are vectors of values for that field to include.
  std::map<std::string, std::vector<std::string>> include_map{
      {"CARRIER", {"UA", "AA", "DL"}},
      {"ORIGIN", {"JFK", "LAX", "ORD", "MIA", "ATL"}}};
  std::map<std::string, std::vector<std::string>> exclude_map{
      {"DEST_COUNTRY", {"US"}}};

  // Calling scan_to_pivot to calculate values
  // by carrier, origin, and region:
  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get() + "|" +
                row["REGION"].get()};
      },
      include_map, exclude_map);

  // Unfiltered equivalent:

  pivot_file_path = "../pax_seats_deps_by_carrier_origin_region.csv";
  std::map<std::string, std::vector<std::string>> unfiltered_map{};

  // Calling scan_to_pivot to calculate values
  // by carrier, origin, and region:
  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get() + "|" +
                row["REGION"].get()};
      },
      unfiltered_map, unfiltered_map);

  // Updating several variables, then calling scan_to_pivot a
  // second time to calculate metrics by
  // carrier and origin only:

  pivot_file_path = "../pax_seats_deps_by_carrier_origin_filtered.csv";
  index_headers = "CARRIER|ORIGIN";

  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      },
      include_map, exclude_map);

  // Producing an unfiltered equivalent:
  pivot_file_path = "../pax_seats_deps_by_carrier_origin.csv";

  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      },
      unfiltered_map, unfiltered_map);

  // Testing out in-memory pivot tables (either via a function or
  // through a set of pre-defined code)
  // (Consider looking into templates as a way to allow various
  // structs to get passed to this function)

  // Trying to use a vector of maps of (string, variant) pairs
  // to store data:
  // (The advantage of this approach over a regular struct is that
  // it's more flexible, as you can use this same type for
  // multiple datasets with different field names. Another major
  // advantage is that the map approach allows values to be accessed
  // by string-based names, which helps make the output easier
  // to interpret and work with.)
  // (Another alternative would be to use std::string in place of
  // std::variants, then convert certain fields (e.g. value columns)
  // to a numeric type when needed. However, doing these conversions
  // ahead of time may save processing time if the table will get used
  // mulitple times.)
  // Although a regular vector (of strings or variants) could be used
  // in place of a map, having a field name assigned to each
  // value should assist with readability and make the code less error-prone.
  std::vector<std::map<std::string, std::variant<std::string, double>>>
      table_rows;

  CSVReader reader(data_file_path);

  // Reading CSV data into table_rows so that the data will be
  // available in RAM for further analyses:

  std::vector<std::string> string_fields{"CARRIER", "ORIGIN", "REGION",
                                         "DEST_COUNTRY"};
  std::vector<std::string> double_fields{"PASSENGERS", "SEATS",
                                         "DEPARTURES_PERFORMED"};

  for (CSVRow &row : reader) {
    // Initializing a new map that will hold the string- and double-typed
    // column values that we wish to include within our in-memory table:
    std::map<std::string, std::variant<std::string, double>> table_row;
    for (std::string &string_field : string_fields) {
      table_row[string_field] = row[string_field].get();
    }
    for (std::string &double_field : double_fields) {
      table_row[double_field] = row[double_field].get<double>();
    }
    // Adding this completed map (which represents a single row of data)
    // to our vector of rows:
    table_rows.push_back(table_row);
  }

  // I originally used the following approach to populate this table, but I
  // realized that this code could be simplified using the approach shown above.

  // table_rows.push_back({{"CARRIER", row["CARRIER"].get()},
  //                       {"ORIGIN", row["ORIGIN"].get()},
  //                       {"REGION", row["REGION"].get()},
  //                       {"DEST_COUNTRY", row["DEST_COUNTRY"].get()},
  //                       {"PASSENGERS", row["PASSENGERS"].get<double>()},
  //                       {"SEATS", row["SEATS"].get<double>()},
  //                       {"DEPARTURES_PERFORMED",
  //                       row["DEPARTURES_PERFORMED"].get<double>()}});}

  // Confirming that the following code will allow me to access variants:
  for (int i = 0; i < 10; i++) {
    std::cout << "\n"
              << std::get<std::string>(table_rows[i]["CARRIER"]) << "\t"
              << std::get<double>(table_rows[i]["PASSENGERS"]);
  }

  // Creating a pivot table: (Port this code into a function
  // once you've finished working on it.)

  // Specifying index fields and value fields:

  std::vector<std::string> index_fields{"CARRIER", "ORIGIN", "REGION",
                                        "DEST_COUNTRY"};
  value_fields = {"PASSENGERS", "SEATS", "DEPARTURES_PERFORMED"};

  // Function definition could begin here.

  auto function_start_time = std::chrono::high_resolution_clock::now();

  // Initializing a struct that can store results for each
  // pivot index combination:
  // Note that each value will be initialized as 0.0 (or, for
  // long integers, 0) in order
  // to help ensure that our final output is correct. (I'm not sure
  // that these initial values are strictly necessary to specify,
  // but it shouldn't hurt to do so.)

  struct Pivot_Vals {
    double pivot_sum{0.0};
    long pivot_count{0};
    double pivot_mean{0.0};
  };

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

  rows_to_scan = -1;


  if (rows_to_scan == -1)
  // In this case, the value will get updated to encompass all rows
  // within the dataset.
  rows_to_scan = table_rows.size();


  for (int i = 0; i < rows_to_scan; i++) // This loop will iterate
  // through each entry within table_rows.
  {
    std::map<std::string, std::variant<std::string, double>> row =
        table_rows[i];
  bool include_row = true;

    // Checking, based on include_map and exclude_map,
    // whether this particular row should be included in our output:
      // Iterating through our map of values to include for each field:
      // This code was based in part on P0W's response at
      // https://stackoverflow.com/a/26282004/13097194 .

      // NOTE: Consider adding double-typed include and exclude maps
      // rather than just string-based ones in order to allow for a 
      // broader range of field-based inclusions/exclusions.
      for (auto const &[field, field_vals] : include_map) {
        // Checking to see whether the row's value is present within
        // our list of field values:
        if (std::ranges::contains(field_vals, 
          std::get<std::string>(row[field])) == false) {
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
        if (std::ranges::contains(field_vals, 
          std::get<std::string>(row[field]))) {
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
          std::get<double>(row[value_fields[vfi]]);
      pivot_map[pivot_index_vals][vfi].pivot_count++;
    }
  }
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
  // std::ofstream ofs_pivot(pivot_file_path);
  // auto row_writer = make_csv_writer(ofs_pivot);

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

  // row_writer << header_row;

  for (const std::string& header_field: header_row)
{
  std::cout << header_field << "\t";
}
std::cout << "\n";
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
      for (const std::string& value_field: pivot_row_vector)
{
  std::cout << value_field << "\t";
}
std::cout << "\n";
    //row_writer << pivot_row_vector;
  }

  auto function_end_time = std::chrono::high_resolution_clock::now();
  auto function_run_time =
      std::chrono::duration<double>(function_end_time - function_start_time)
          .count();
  std::cout << "Finished processing the " << rows_to_scan << "-row dataset in "
            << function_run_time << " seconds.\n";


  auto program_end_time = std::chrono::high_resolution_clock::now();
  auto program_run_time =
      std::chrono::duration<double>(program_end_time - program_start_time)
          .count();
  std::cout << "The program finished running after " << program_run_time
            << " seconds.\n";
}
