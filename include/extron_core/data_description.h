#pragma once


#include <tinyxml2.h>


#include <ctime>
#include <map>
#include <vector>
#include <string>


namespace extron::core {


    class data_description {
      public:
        class day;
        class described_day;

        data_description();
        ~data_description();

        void clear();
        void clear_and_load(tinyxml2::XMLNode const *);
        void deserialise_from_xml_day(tinyxml2::XMLNode const *);
    
        void described_day_init(described_day&) const;
        void described_day_next(described_day&, std::time_t const&) const;
    
        std::string const& get_nickname() const;

      protected:
        // util
        char const *clean_string(char const*) const;

        std::string nickname;
        std::map<std::time_t, day> day_data;
    };


    class data_description::described_day {
      public:
        struct category_data {
            std::string display_name, sort_name;
            float weight;
        };
        struct exercise_data {
            std::string category;
            std::string display_name, sort_name;
            std::string unit;
            float weight;
            float peak_min;
            bool is_suspended;
            std::time_t first_mentioned;
        };
    
        std::time_t time;
        std::map<std::string, category_data>  categories;
        std::map<std::string, exercise_data>  extercises;
    };



    class data_description::day {
      public:
        struct category_data {
          std::string uid;
          std::string display_name, sort_name;
          float weight;
        };
        struct exercise_data {
          std::string uid;
          std::string category;
          std::string display_name, sort_name;
          std::string unit;
          float weight;
          float peak_min;
          int set_suspend;
        };

        std::vector<category_data> categories;
        std::vector<exercise_data> extercises;
    };

}