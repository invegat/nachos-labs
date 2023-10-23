//
// Created by camar on 10/5/2023.
//
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <regex>
#include <cassert>
using namespace std;
int
main(int argc, char **argv)
{
    if (argc != 2) {
        cout << "usage testElevator <log file name>";
        exit(1);
    }
    fstream my_file;
    my_file.open(argv[1], ios::in);
    if (!my_file) {
        cout << "No such file";
    }
    else {
//        string sent;
        std::stringstream buffer;
        buffer << my_file.rdbuf();
        string sent = buffer.str();
        // Person 1 got into the elevator.
        std::regex person_regex("Person (\\d+) got into the elevator\\.");
        auto person_begin = std::sregex_iterator(sent.begin(), sent.end(), person_regex);
        auto persons_end = std::sregex_iterator();

        smatch match;
        int k = 0;
        for (std::sregex_iterator i = person_begin; i != persons_end; ++i) {
            // Person 6 got out of the elevator
            regex matcher("Person (" + i->str(1) + ") got out of the elevator");
            if (regex_search(sent, match, matcher) == true) {
                // cout << match.str(1) << " found" << endl;
                k++;
            } else {
                cout << "failure to find match " << i->str(1) << endl;
            }

        }
        cout << k << " matches found" << endl;

        regex nameMatcher("f(\\d+)\\.p(\\d+)\\.d(\\d+)\\.log");
        smatch nameMatch;
        string* fn = new string(argv[1]);
        if (regex_search(*fn, nameMatch, nameMatcher) == true) {
            const int people = (const int)stoi(nameMatch.str(2));
            // assert(people == k); // k matches the People in the file name"
            if (people == k)
                cout << "number of matches " + to_string(people) + " matches People in file name " + *fn << endl;
        }
        my_file.close();
        regex both("(?:Person (\\d+) got (into) the elevator\\.)|(?:Person (\\d+) got (out) of the elevator\\.)");
        auto occupancy_begin = std::sregex_iterator(sent.begin(), sent.end(), both);
        auto both_end = std::sregex_iterator();
        int occupancy = 0;
        // const int max_occupancy = 5;
        int my_max_occupancy = 0;
        for (std::sregex_iterator i = occupancy_begin; i != both_end; ++i) {
            if (std::strcmp(i->str(2).c_str(), "into") == 0) {
                occupancy++;
                if (occupancy > my_max_occupancy)
                    my_max_occupancy = occupancy;
            }
            else {
                assert(std::strcmp(i->str(4).c_str(), "out") == 0);
                occupancy--;
            }
        }
        cout << "max occupancy " << to_string(my_max_occupancy) << endl;
    }
    return 0;
}