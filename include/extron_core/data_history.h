#pragma once


#include <tinyxml2.h>


#include <ctime>
#include <map>
#include <vector>
#include <string>


namespace extron::core {


    class data_history {
      public:
        class workout_time;
        class workout;

        data_history();
        ~data_history();

        void clear();
        void clear_and_load(tinyxml2::XMLNode const *);
        void deserialise_from_xml_workout(tinyxml2::XMLNode const *);

        void save(tinyxml2::XMLNode *) const;
        void serialise_workout_to_xml(const workout_time&, const workout&, tinyxml2::XMLNode *) const;
    
        bool get_workout_data(data_history::workout_time const &time, data_history::workout *out_workout) const;
        void save_workout_data(data_history::workout_time const &previous_time, data_history::workout_time &inout_new_time, data_history::workout const&);

        const std::map<workout_time, workout>& get_workout_data() const;
      protected:
        char const *clean_string(char const*) const;

        std::map<workout_time, workout> workout_data;
    };


    class data_history::workout_time {
      public:
        workout_time(std::time_t time, uint32_t uid);

        bool operator<(workout_time const& other) const;
        bool operator==(workout_time const& other) const;
        bool operator!=(workout_time const& other) const;

        std::time_t time;
        uint32_t uid;
    };


    class data_history::workout {
      public:
        std::string type;

        struct counts {
          uint32_t count, target;
        };

        std::map<std::string, counts> workout_counts;
    };

}