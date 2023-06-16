#include "extron_core/data_description.h"

#include <iomanip>
#include <sstream>


using namespace extron::core;


data_description::data_description()
{
}


data_description::~data_description()
{
}


void data_description::clear()
{
    this->day_data.clear();
}


void data_description::clear_and_load(tinyxml2::XMLNode const *dict)
{
    this->clear();

    tinyxml2::XMLElement const *xml_meta = nullptr;
    tinyxml2::XMLElement const *xml_days = nullptr;

    for (auto entry = dict->FirstChild(); entry; entry = entry->NextSibling()) {
        if (0 == strcmp(entry->Value(), "meta"))
          xml_meta = entry->ToElement();
        else if (0 == strcmp(entry->Value(), "days"))
          xml_days = entry->ToElement();
    }

    if (xml_meta) {
        tinyxml2::XMLElement const* xml_meta_properties = nullptr;
        
        for (auto entry = xml_meta->FirstChild(); entry; entry = entry->NextSibling()) {
            if (0 == strcmp(entry->Value(), "properties"))
              xml_meta_properties = entry->ToElement();
        }

        if (xml_meta_properties)
          this->nickname = xml_meta_properties->Attribute("nickname");
    }

    if (xml_days) {
        for (auto entry = xml_days->FirstChild(); entry; entry = entry->NextSibling()) {
            if (0 == strcmp(entry->Value(), "day")) 
              this->deserialise_from_xml_day(entry);
        }
    }
}


void data_description::deserialise_from_xml_day(tinyxml2::XMLNode const *day)
{
    char const *date = day->ToElement()->Attribute("date");
    if (!date || !date[0])
      return;

    // Deserialise the date

    std::tm tm;
    memset(&tm, 0x0, sizeof(std::tm));

    std::istringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    std::time_t time = _mkgmtime(&tm);

    // Obtain day

    auto &data_day = this->day_data[time];

    // Load day data

    for (auto entry = day->FirstChild(); entry; entry = entry->NextSibling()) {
        if (0 == strcmp(entry->Value(), "cat")) {
            day::category_data category;
            category.uid          = this->clean_string(entry->ToElement()->Attribute("id"));
            category.display_name = this->clean_string(entry->ToElement()->Attribute("name"));
            category.sort_name    = this->clean_string(entry->ToElement()->Attribute("sortName"));
            category.weight       = entry->ToElement()->FloatAttribute("weight");
            data_day.categories.push_back(category);
            continue;
        }
        if (0 == strcmp(entry->Value(), "ex")) {
            day::exercise_data exercise;
            exercise.uid          = this->clean_string(entry->ToElement()->Attribute("id"));
            exercise.category     = this->clean_string(entry->ToElement()->Attribute("cat"));
            exercise.display_name = this->clean_string(entry->ToElement()->Attribute("name"));
            exercise.sort_name    = this->clean_string(entry->ToElement()->Attribute("sortName"));
            exercise.unit         = this->clean_string(entry->ToElement()->Attribute("unit"));
            exercise.weight       = entry->ToElement()->FloatAttribute("weight");
            exercise.peak_min     = entry->ToElement()->FloatAttribute("peakMin", -1.f);
        
            std::string str = this->clean_string(entry->ToElement()->Attribute("special"));
            exercise.set_suspend = 0;
            if (str.find("suspend") != std::string::npos)
              exercise.set_suspend = 1;
            else if (str.find("continue") != std::string::npos)
              exercise.set_suspend = -1;
        
            data_day.extercises.push_back(exercise);
            continue;
        }
    }
}


void data_description::described_day_init(described_day &desc) const
{
    if (day_data.size() == 0) {
        time(&desc.time);
        return;
    }

    desc.time = day_data.begin()->first - 24*3600;
    this->described_day_next(desc, day_data.begin()->first);
}


void data_description::described_day_next(described_day &desc, std::time_t const &next_time) const
{
    std::time_t target_time = next_time - (next_time % 24*3600);

    while (desc.time < target_time) {
        desc.time += 24*3600;

        auto const& day = this->day_data.find(desc.time);
        if (day == this->day_data.end())
          continue;

        for (auto const& category : day->second.categories) {
            auto &desc_category = desc.categories[category.uid];

            if (desc_category.weight == 0.f)
              desc_category.weight = 1.f;

            if (category.display_name.size() > 0)
              desc_category.display_name = category.display_name;
            if (category.sort_name.size() > 0)
              desc_category.sort_name = category.sort_name;

            if (desc_category.sort_name.size() == 0)
              desc_category.sort_name = desc_category.display_name;

            if (!std::isnan(category.weight) && category.weight != 0.f)
              desc_category.weight = category.weight;
        }

        for (auto const& exercise : day->second.extercises) {
            auto &desc_exercise = desc.extercises[exercise.uid];

            if (desc_exercise.display_name.size() == 0)
            {
                desc_exercise.first_mentioned = day->first;
                desc_exercise.peak_min = 0.f;
            }

            if (exercise.category.size() > 0)
              desc_exercise.category = exercise.category;
            if (exercise.display_name.size() > 0)
              desc_exercise.display_name = exercise.display_name;
            if (exercise.sort_name.size() > 0)
              desc_exercise.sort_name = exercise.sort_name;
            if (exercise.unit.size() > 0)
              desc_exercise.unit = exercise.unit;
            if (!std::isnan(exercise.weight))
              desc_exercise.weight = exercise.weight;
            if (exercise.peak_min >= 0.f)
              desc_exercise.peak_min = exercise.peak_min;
            if (exercise.set_suspend != 0)
              desc_exercise.is_suspended = exercise.set_suspend > 0;

            if (desc_exercise.sort_name.size() == 0)
              desc_exercise.sort_name = desc_exercise.display_name;
        }
    }
}


std::string const& data_description::get_nickname() const
{
    return this->nickname;
}


char const *data_description::clean_string(char const *string) const
{
    if (string)
      return string;
    return "";
}