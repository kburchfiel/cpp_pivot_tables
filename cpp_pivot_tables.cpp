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
  long rows_to_scan{-1}; // Use -1 to scan all rows

  std::string pivot_file_path{
      "../Output/pax_seats_deps_by_carrier_origin_region_filtered.csv"};

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

  pivot_file_path = "../Output/pax_seats_deps_by_carrier_origin_region.csv";
  std::map<std::string, std::vector<std::string>> unfiltered_string_map{};

  // Calling scan_to_pivot to calculate values
  // by carrier, origin, and region:
  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get() + "|" +
                row["REGION"].get()};
      },
      unfiltered_string_map, unfiltered_string_map);

  // Updating several variables, then calling scan_to_pivot a
  // second time to calculate metrics by
  // carrier and origin only:

  pivot_file_path = "../Output/pax_seats_deps_by_carrier_origin_filtered.csv";
  index_headers = "CARRIER|ORIGIN";

  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      },
      include_map, exclude_map);

  // Producing an unfiltered equivalent:
  pivot_file_path = "../Output/pax_seats_deps_by_carrier_origin.csv";

  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      },
      unfiltered_string_map, unfiltered_string_map);

  // Testing out in-memory pivot tables (either via a function or
  // through a set of pre-defined code)
  // (Consider looking into templates as a way to allow various
  // structs to get passed to this function)

  std::vector<std::map<std::string, std::variant<std::string, double>>>
      table_rows;

  std::vector<std::string> string_fields{"CARRIER", "ORIGIN", "REGION",
                                         "DEST_COUNTRY"};
  std::vector<std::string> double_fields{"PASSENGERS", "SEATS",
                                         "DEPARTURES_PERFORMED"};

  CSVReader reader(data_file_path);

  // Reading CSV data into table_rows so that the data will be
  // available in RAM for further analyses:


auto import_start_time = std::chrono::high_resolution_clock::now();
  for (CSVRow &row : reader) {
    // Initializing a new map that will hold the string- and double-typed
    // column values that we wish to include within our in-memory table:
    std::map<std::string, std::variant<std::string, double>> table_row;
    for (std::string &string_field : string_fields) {
      table_row[string_field] = row[string_field].get();
    }
    for (std::string &double_field : double_fields) {
      table_row[double_field] = row[double_field].get<double>();
      // std::cout << row[double_field].get<double>() << "\t";
    }
    // Adding this completed map (which represents a single row of data)
    // to our vector of rows:
    table_rows.push_back(table_row);
  }


  auto import_end_time = std::chrono::high_resolution_clock::now();
  auto import_run_time =
      std::chrono::duration<double>(import_end_time - import_start_time)
          .count();
  std::cout << "The dataset got loaded into memory in " << import_run_time
            << " seconds.\n";

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
  // for (int i = 0; i < 10; i++) {
  //   std::cout << "\n"
  //             << std::get<std::string>(table_rows[i]["CARRIER"]) << "\t"
  //             << std::get<double>(table_rows[i]["PASSENGERS"]);
  // }

  // Creating a pivot table: (Port this code into a function
  // once you've finished working on it.)

  // Specifying index fields and value fields:

  std::vector<std::string> index_fields{"CARRIER", "ORIGIN"};
  value_fields = {"PASSENGERS", "SEATS", "DEPARTURES_PERFORMED"};

pivot_file_path = "../Output/pax_seats_deps_by_carrier_origin_in_memory.csv";

std::map<std::string, std::vector<double>> unfiltered_double_map {};

std::map<std::string, std::vector<double>> double_include_map {
  {"PASSENGERS",{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}};

std::map<std::string, std::vector<double>> double_exclude_map {{"PASSENGERS",{0}}};




std::cout << "Now running unfiltered in-memory pivot.\n";

// Unfiltered function:
std::map<std::string, std::map<std::string, Pivot_Vals>> output_map = (
in_memory_pivot(
  table_rows, index_fields, value_fields, true,
  pivot_file_path, unfiltered_string_map, unfiltered_string_map, 
unfiltered_double_map, unfiltered_double_map));

std::cout << "Now running filtered in-memory pivot.\n";

pivot_file_path = "../Output/pax_seats_deps_by_carrier_origin_in_memory_filtered.csv";

// Testing out double-typed filters:
output_map = (
in_memory_pivot(
  table_rows, index_fields, value_fields, true,
  pivot_file_path, unfiltered_string_map, unfiltered_string_map, 
double_include_map, double_exclude_map));

  

  auto program_end_time = std::chrono::high_resolution_clock::now();
  auto program_run_time =
      std::chrono::duration<double>(program_end_time - program_start_time)
          .count();
  std::cout << "The program finished running after " << program_run_time
            << " seconds.\n";
}
