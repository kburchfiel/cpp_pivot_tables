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

#include "csv.hpp"
#include "pivot_compressors.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>

using namespace csv;

int main() {
  auto program_start_time = std::chrono::high_resolution_clock::now();
  std::string data_file_path{"/home/kjb3/D1V1/Documents/\
Large_Datasets/BTS/T_T100_SEGMENT_ALL_CARRIER_2024.csv"};
  std::string index_headers{"CARRIER|ORIGIN|REGION"};
  std::vector<std::string> value_fields{"PASSENGERS", "SEATS", "DEPARTURES_PERFORMED"};
  long rows_to_scan{-1}; // Use -1 to scan all rows

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
std::map<std::string, std::vector<std::string>> unfiltered_map {};


  // Calling scan_to_pivot to calculate values
  // by carrier, origin, and region:
  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path, [&](CSVRow row) -> std::string {
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
      },include_map, exclude_map);

// Producing an unfiltered equivalent:
pivot_file_path = "../pax_seats_deps_by_carrier_origin.csv";

  scan_to_pivot(
      data_file_path, value_fields, index_headers, rows_to_scan,
      pivot_file_path,
      [&](CSVRow row) -> std::string {
        return {row["CARRIER"].get() + "|" + row["ORIGIN"].get()};
      },
      unfiltered_map, unfiltered_map);

  auto program_end_time = std::chrono::high_resolution_clock::now();
  auto program_run_time =
      std::chrono::duration<double>(program_end_time - program_start_time)
          .count();
  std::cout << "The program finished running after " << program_run_time
            << " seconds.\n";
}
